#include <QtGui>
#include <QGLWidget>
#include <QGraphicsView>
#include <QGraphicsSvgItem>
#include <QDebug>

#include "Display.h"
#include "Curve.h"

#define ENDL "\n"
#define HERE         qDebug("%s(%d): HERE"ENDL,__FILE__,__LINE__)
#define REPORT(expr) qDebug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#define TRY(expr,lbl) if(!(expr)) {REPORT(expr); goto lbl;}
#define DIE          qFatal("%s(%d): Aborting."ENDL,__FILE__,__LINE__)

////////////////////////////////////////////////////////////////////////////////
// LoadingGraphicsWidget                                         QGRAPHICSWIDGET
// (doesn't work)
////////////////////////////////////////////////////////////////////////////////

class LoadingGraphicsWidget : public QGraphicsWidget
{ public:
    LoadingGraphicsWidget(QGraphicsItem *parent=0)
    :QGraphicsWidget(parent)
    { QGraphicsTextItem *item = new QGraphicsTextItem("LOADING");
      item->setParent(this);
      setGraphicsItem(item);
    }
};

////////////////////////////////////////////////////////////////////////////////
// VIEW         QGRAPHICSVIEW
////////////////////////////////////////////////////////////////////////////////

View::View(QGraphicsScene *scene, QWidget *parent)
  : QGraphicsView(scene,parent)
  , lockitem_(0)
  , iframe_(-1)
{ setAcceptDrops(true);
  setResizeAnchor(AnchorViewCenter);
  setDragMode(NoDrag);
  setAlignment(Qt::AlignCenter);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setRenderHints( QPainter::Antialiasing
                | QPainter::SmoothPixmapTransform
                | QPainter::HighQualityAntialiasing
                | QPainter::TextAntialiasing 
      );
  //setBackgroundBrush(QBrush(Qt::black));

  QGLWidget *viewport = new QGLWidget(QGLFormat(QGL::SampleBuffers));
  setViewport(viewport);
  viewport->makeCurrent();
  TRY( viewport->context()->isValid(),ErrorViewport);
  TRY( viewport->isValid()           ,ErrorViewport);
ErrorViewport: //ignore
  return;
}

void View::lockTo(const QGraphicsItem *item)
{ lockitem_ = item;
  QColor c = Qt::white;
  if(lockitem_)
  { scene()->setSceneRect(lockitem_->boundingRect());
    fitInView(lockitem_,Qt::KeepAspectRatio);
    c = Qt::black;
  }
  setBackgroundBrush(QBrush(c));
}

void View::setFrame(int iframe)
{ iframe_=iframe; }

void View::drawForeground(QPainter *painter, const QRectF &rect)
{ if(iframe_>=0)
  { painter->resetTransform();
    painter->setFont(QFont("Helvetica",12,QFont::Bold));
    painter->setPen(QPen(Qt::yellow));
    painter->drawText(2,14,QString("Frame: %1").arg(iframe_,4));
  }
}

void View::dragEnterEvent(QDragEnterEvent *event)
{
  if(event->mimeData()->hasFormat("text/uri-list"))
    foreach(QUrl url, event->mimeData()->urls())
    { if( Data::isValidPath(url.toLocalFile()))
      { event->acceptProposedAction();
        return;
      }
    }
}

void View::dragMoveEvent(QDragMoveEvent *event)
{ event->accept();
}

void View::dropEvent(QDropEvent *event)
{ if(event->mimeData()->hasFormat("text/uri-list"))
    foreach(QUrl url, event->mimeData()->urls())
      if( Data::isValidPath(url.toLocalFile()))
      { emit dropped(url);
        event->acceptProposedAction();
        return;
      }
}

void View::resizeEvent(QResizeEvent *event)
{ 
  if(lockitem_)
    fitInView(lockitem_,Qt::KeepAspectRatio);
  QGraphicsView::resizeEvent(event);
}

void View::wheelEvent(QWheelEvent *event) 
{ event->ignore(); // pass the event up 
}

////////////////////////////////////////////////////////////////////////////////
// DISPLAY      QWIDGET
////////////////////////////////////////////////////////////////////////////////
Display::Display(QWidget *parent,Qt::WindowFlags f)
  : QWidget(parent,f)
  , view_(0)
  , scene_(0)
  , droptarget_(0)
  , dataItemsRoot_(0)
  , image_(0)
  , loadingGraphics_(0)
  , data_(this)
  , iframe_(0)
  , curves_(NULL)
{ makeActions_();

  ///// Setup the initial scene
  scene_ = new QGraphicsScene;
  curves_ = new CurveGroup(scene_,this);
  // scene
  //      \-- drop target
  //      \-- data items
  //            \-- video (pixmap) item
  //            \-- whisker curves
  //      \-- loading item
  { scene_->setBackgroundBrush(Qt::white);

    image_           = new QGraphicsPixmapItem();
    droptarget_      = new QGraphicsSvgItem(":/images/droptarget");
    dataItemsRoot_   = new QGraphicsWidget;
    loadingGraphics_ = new LoadingGraphicsWidget;
    scene_->addItem(droptarget_);
    image_->setParentItem(dataItemsRoot_);
    scene_->addItem(dataItemsRoot_);
    scene_->addItem(loadingGraphics_);
  }

  ///// Init the graphicsview
  { QVBoxLayout *layout = new QVBoxLayout;
    view_ = new View(scene_,this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(view_);
    setLayout(layout);

    TRY(connect(view_,SIGNAL(dropped(const QUrl&)),
                this ,  SLOT(   open(const QUrl&))),ErrorConnect);
    TRY(connect(this ,SIGNAL( frameId(int)),
                view_,  SLOT(setFrame(int))),ErrorConnect);
  }

  ///// State machine to handle item visiblity
  { QStateMachine *sm = new QStateMachine(this);
    QState *empty   = new QState,
          *loading = new QState,
          *loaded  = new QState;
    // states and properties
    empty->assignProperty(view_,"backgroundBrush",QBrush(Qt::white));
    empty->assignProperty(droptarget_,"visible",true);
    empty->assignProperty(dataItemsRoot_,"visible",false);
    empty->assignProperty(loadingGraphics_,"visible",false);

    loading->assignProperty(view_,"backgroundBrush",QBrush(Qt::red));
    loading->assignProperty(view_->viewport(),"cursor",Qt::WaitCursor);
    loading->assignProperty(droptarget_,"visible",false);
    loading->assignProperty(loadingGraphics_,"visible",true);

    loaded->assignProperty(view_,"backgroundBrush",QBrush(Qt::black));
    loaded->assignProperty(view_->viewport(),"cursor",Qt::CrossCursor);
    loaded->assignProperty(dataItemsRoot_,"visible",true);
    loaded->assignProperty(loadingGraphics_,"visible",false);

    // transitions
    empty->addTransition(this,SIGNAL(loadStarted()),loading);
    loading->addTransition(&data_,SIGNAL(loaded()),loaded);
    loaded->addTransition(this,SIGNAL(loadStarted()),loading);
    sm->addState(empty);
    sm->addState(loading);
    sm->addState(loaded);
    sm->setInitialState(empty);
    sm->start();

    // connect to state transitions
    TRY(connect(loaded,SIGNAL(entered()),this,SLOT(showCurrentFrame())),ErrorConnect);
  }
  return;
ErrorConnect:
  exit(-1);
}

/**
 * Tries to read the requested frame and posts it to the screen.
 * Updates iframe_, which keeps track of where we are
 * in the movie, if the read was successful.
 */
void Display::showFrame(int index)
{ QPixmap p = data_.frame(index);
  if(!p.isNull())
  { iframe_=index;  // only updates if read was successful
    emit frameId(iframe_);
    image_->setPixmap(p);
    
    // add whisker curves
    { int i;
      int nident = data_.maxIdentity()-data_.minIdentity();
      curves_->beginAdding();
      for(i=0;i<data_.curveCount(iframe_);++i)
      { 
        QPolygonF shape = data_.curve(iframe_,i);
        int         wid = data_.wid(iframe_,i),
                  ident = data_.identity(iframe_,i);
        curves_->add(shape,wid,ident,nident);
      }
      curves_->endAdding();
    }
  }
  return;
}

/**
 * Usually this function is called to referesh the view for a new video.
 * Also calls View::lockTo for the image.
 */
void Display::showCurrentFrame()
{ if(iframe_>=0 && iframe_<data_.frameCount())
    showFrame(iframe_);
  else
    showFrame(0);
  view_->lockTo(image_);
}

void Display::nextFrame()
{ showFrame(iframe_+1);
}
#define NEXTFRAME(X) \
  void Display::nextFrame##X () \
  { showFrame(iframe_+X); \
  }
NEXTFRAME(10);
NEXTFRAME(100);
NEXTFRAME(1000);

void Display::prevFrame()
{ showFrame(iframe_-1);
}
#define PREVFRAME(X) \
  void Display::prevFrame##X () \
  { showFrame(iframe_-X); \
  }
PREVFRAME(10);
PREVFRAME(100);
PREVFRAME(1000);

void Display::firstFrame()
{ showFrame(0);
}

void Display::lastFrame()
{ showFrame(data_.frameCount()-1);
}

void Display::open(const QUrl& url)
{ data_.open(url);
  emit loadStarted();
}

void Display::deleteSelected()
{ curves_->removeSelected();
}

void Display::makeActions_()
{ 
  actions_["next"    ]= new QAction(tr("Next frame")             ,this);  
  actions_["next10"  ]= new QAction(tr("Jump ahead 10 frames")   ,this);  
  actions_["next100" ]= new QAction(tr("Jump ahead 100 frames")  ,this);  
  actions_["next1000"]= new QAction(tr("Jump ahead 1000 frames") ,this);  
  actions_["prev"    ]= new QAction(tr("Previous frame")         ,this);  
  actions_["prev10"  ]= new QAction(tr("Jump back 10 frames")    ,this);  
  actions_["prev100" ]= new QAction(tr("Jump back 100 frames")   ,this);  
  actions_["prev1000"]= new QAction(tr("Jump back 1000 frames")  ,this);  
  actions_["last"    ]= new QAction(tr("Jump to end")            ,this);  
  actions_["first"   ]= new QAction(tr("Jump to start")          ,this);
  actions_["delete"  ]= new QAction(tr("Delete selected curve.") ,this);

  actions_["next"    ]->setShortcut( QKeySequence( Qt::Key_Right));  
  actions_["next10"  ]->setShortcut( QKeySequence( Qt::Key_Right + Qt::SHIFT));  
  actions_["next100" ]->setShortcut( QKeySequence( Qt::Key_Right + Qt::SHIFT + Qt::CTRL));  
  actions_["next1000"]->setShortcut( QKeySequence( Qt::Key_Right + Qt::SHIFT + Qt::CTRL + Qt::ALT));  
  actions_["prev"    ]->setShortcut( QKeySequence( Qt::Key_Left));   
  actions_["prev10"  ]->setShortcut( QKeySequence( Qt::Key_Left  + Qt::SHIFT));  
  actions_["prev100" ]->setShortcut( QKeySequence( Qt::Key_Left  + Qt::SHIFT + Qt::CTRL));  
  actions_["prev1000"]->setShortcut( QKeySequence( Qt::Key_Left  + Qt::SHIFT + Qt::CTRL + Qt::ALT));  
  actions_["last"    ]->setShortcut( QKeySequence( "]"));            
  actions_["first"   ]->setShortcut( QKeySequence( "["));            
  actions_["delete"  ]->setShortcut( QKeySequence( QKeySequence::Delete));

  TRY(connect(actions_["next"     ],SIGNAL(triggered()),this,SLOT(nextFrame())     ),Error);  
  TRY(connect(actions_["next10"   ],SIGNAL(triggered()),this,SLOT(nextFrame10  ()) ),Error);  
  TRY(connect(actions_["next100"  ],SIGNAL(triggered()),this,SLOT(nextFrame100 ()) ),Error);  
  TRY(connect(actions_["next1000" ],SIGNAL(triggered()),this,SLOT(nextFrame1000()) ),Error);  
  TRY(connect(actions_["prev"     ],SIGNAL(triggered()),this,SLOT(prevFrame())     ),Error);    
  TRY(connect(actions_["prev10"   ],SIGNAL(triggered()),this,SLOT(prevFrame10  ()) ),Error);  
  TRY(connect(actions_["prev100"  ],SIGNAL(triggered()),this,SLOT(prevFrame100 ()) ),Error);  
  TRY(connect(actions_["prev1000" ],SIGNAL(triggered()),this,SLOT(prevFrame1000()) ),Error);  
  TRY(connect(actions_["last"     ],SIGNAL(triggered()),this,SLOT(lastFrame())     ),Error);    
  TRY(connect(actions_["first"    ],SIGNAL(triggered()),this,SLOT(firstFrame())    ),Error);    
  TRY(connect(actions_["delete"   ],SIGNAL(triggered()),this,SLOT(deleteSelected())),Error);    

  foreach(QAction *a,actions_)
  { a->setShortcutContext(Qt::ApplicationShortcut);
    addAction(a);
  }
  return;
Error:
  DIE;
}

#define countof(a) (sizeof(a)/sizeof(*a))
void Display::wheelEvent(QWheelEvent *event)
{ void (Display::*next)(void);
  void (Display::*prev)(void);
  int nmods = 0;
  Qt::KeyboardModifiers m = event->modifiers();

  { Qt::KeyboardModifiers mods[] = { Qt::ShiftModifier,
                                     Qt::ControlModifier, 
                                     Qt::AltModifier,     
                                     Qt::MetaModifier    
                                 };
    for(unsigned int i=0; i<countof(mods);++i)
      nmods += (m & mods[i]) == mods[i];
  }

  switch(nmods)
  { case 0: next = &Display::nextFrame;     prev = &Display::prevFrame;     break;
    case 1: next = &Display::nextFrame10;   prev = &Display::prevFrame10;   break; 
    case 2: next = &Display::nextFrame100;  prev = &Display::prevFrame100;  break; 
    case 3: 
    case 4:
            next = &Display::nextFrame1000; prev = &Display::prevFrame1000; break; 
    default:
      HERE; 
      exit(-1);
  }

  if(event->delta()>0)
    (this->*next)();
  else if(event->delta()<0)
    (this->*prev)();
}
