#include <QtGui>
#include <QGLWidget>
#include <QGraphicsView>
#include <QGraphicsSvgItem>
#include <QDebug>

#include "Display.h"

#define ENDL "\n"
#define HERE         qDebug("%s(%d): HERE"ENDL,__FILE__,__LINE__); 
#define REPORT(expr) qDebug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#define TRY(expr,lbl) if(!(expr)) {REPORT(expr); goto lbl;}

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
  , nextFrame_(0)
  , prevFrame_(0)
  , lastFrame_(0)
  , firstFrame_(0)
  , view_(0)
  , scene_(0)
  , droptarget_(0)
  , dataItemsRoot_(0)
  , image_(0)
  , loadingGraphics_(0)
  , data_(this)
  , iframe_(0)
  , framePositionDisplay_(0)
{ makeActions_();

  ///// Setup the initial scene
  scene_ = new QGraphicsScene;
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
    framePositionDisplay_ = new QGraphicsTextItem("Frame: 0");
    scene_->addItem(droptarget_);
    image_->setParentItem(dataItemsRoot_);
    framePositionDisplay_->setParentItem(dataItemsRoot_);
    framePositionDisplay_->setDefaultTextColor(Qt::yellow);
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
    image_->setPixmap(p);
    
    framePositionDisplay_->setPlainText(QString("Frame: %1").arg(iframe_,5));

    // add whisker curves
    { int i;
      for(i=0;i<data_.curveCount(iframe_);++i)
      { QPainterPath p;
        p.addPolygon(data_.curve(iframe_,i));
        if(i<curves_.size())
          curves_[i]->setPath(p);                      // use existing curves
        else
        { QGraphicsPathItem *pp;
          curves_.append(pp=new QGraphicsPathItem(p)); // or add a new one
          scene_->addItem(pp);
        }
        // determine color based on identity
        QColor color(255,155,55,200); // default
        int ident,
            nidents = data_.maxIdentity()-data_.minIdentity(); // should add one, but one identity (-1) doesn't count
                      //e.g. 5 - (-1) = 6 ... 0,1,2,3,4,5
        if(-1!=(ident=data_.identity(iframe_,i)))
          color.setHsvF(ident/(qreal)nidents,1.0,1.0);
        curves_[i]->setPen(QPen(
              QBrush(color),
              2,               //width
              Qt::DotLine,     //pen style
              Qt::RoundCap,    //pen cap style
              Qt::RoundJoin)); //join style
        curves_[i]->show();
      }
      // hide unused curves
      for(;i<curves_.size();++i)
        curves_[i]->hide();
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

void Display::makeActions_()
{ 
  nextFrame_  = new QAction(tr("Next frame")    ,this);
  prevFrame_  = new QAction(tr("Previous frame"),this); 
  lastFrame_  = new QAction(tr("Jump to end")   ,this); 
  firstFrame_ = new QAction(tr("Jump to start") ,this); 

  nextFrame_->setShortcut( QKeySequence( Qt::Key_Right));
  prevFrame_->setShortcut( QKeySequence( Qt::Key_Left));
  lastFrame_->setShortcut( QKeySequence( "]"));
  firstFrame_->setShortcut(QKeySequence( "["));

  nextFrame_->setShortcutContext(Qt::ApplicationShortcut);
  prevFrame_->setShortcutContext(Qt::ApplicationShortcut);
  lastFrame_->setShortcutContext(Qt::ApplicationShortcut);
  firstFrame_->setShortcutContext(Qt::ApplicationShortcut);

  addAction(nextFrame_);
  addAction(prevFrame_);
  addAction(lastFrame_);
  addAction(firstFrame_);

  TRY(connect(nextFrame_ ,SIGNAL(triggered()),
              this       ,  SLOT(nextFrame())),Error);
  TRY(connect(prevFrame_ ,SIGNAL(triggered()),
              this       ,  SLOT(prevFrame())),Error);
  TRY(connect(lastFrame_ ,SIGNAL(triggered()),
              this       ,  SLOT(lastFrame())),Error);
  TRY(connect(firstFrame_,SIGNAL(triggered()),
              this       ,  SLOT(firstFrame())),Error);
Error:
  return;
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
