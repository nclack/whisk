#include <QtGui>
#include <QGLWidget>
#include <QGraphicsView>
#include <QGraphicsSvgItem>
#include <QDebug>

#include "Editor.h"
#include "Curve.h"
#include "Frame.h"
#include "FaceIndicator.h"

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
  , ident_(-1)
  , indicate_advance_(false)
{ setAcceptDrops(true);
  setResizeAnchor(AnchorViewCenter);
  setDragMode(NoDrag);
  setAlignment(Qt::AlignCenter);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  //setViewportUpdateMode(FullViewportUpdate);
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

  { QTimer *t=new QTimer(this);
    connect(t,SIGNAL(timeout()),this,SLOT(invalidateForeground()));
    t->start(1000.0/30.0);
  }
ErrorViewport: //ignore
  return;
}

void View::invalidateForeground()
{
  scene()->invalidate(QRectF(),QGraphicsScene::ForegroundLayer);
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
{
  if(iframe_>=0)
  {
    painter->resetTransform();
    { QFont font = QFont("Helvetica",20,QFont::Bold);
      QString msg;
      painter->setPen(QPen(Qt::white));
      painter->setFont(font);

      msg=QString("Frame: %1").arg(iframe_,4);
      painter->drawText(2,22,msg);
      msg=QString("Whisker: %1").arg(ident_);
      painter->drawText(2,44,msg);
      
      if(indicate_advance_)
      { painter->setPen(Qt::green);
        painter->drawText(2,66,"Advance");
      }
    }
    { QFont font = QFont("Helvetica",10);
      QPointF p  = QPointF(   mapFromGlobal(QCursor::pos())),
              o  = QPointF(10,10),
              ps = mapToScene(mapFromGlobal(QCursor::pos()));
      QStaticText txt(QString("(%1,%2)").arg((int)ps.x()).arg((int)ps.y()));
      txt.prepare();
      painter->setFont(font);
      painter->setPen(Qt::white);
      painter->setBrush(Qt::white);
      painter->drawText(p+o,QString("(%1,%2)").arg((int)ps.x()).arg((int)ps.y()));
      //painter->drawStaticText(p,txt);
      QRectF r(0.0f,0.0f,1.0f,1.0f);
      r.moveCenter(p);
      painter->drawEllipse(r);
    }
  }
}

int View::ident()
{return ident_;}

void View::setIdent(int ident)
{ ident_=ident;
  invalidateForeground();
  emit identChanged(ident);
}

void View::setAdvanceIndicator(bool b)
{ indicate_advance_=b;
  invalidateForeground();
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
// EDITOR                                                                QWIDGET
////////////////////////////////////////////////////////////////////////////////
Editor::Editor(QWidget *parent,Qt::WindowFlags f)
  : QWidget(parent,f)
  , view_(0)
  , scene_(0)
  , droptarget_(0)
  , dataItemsRoot_(0)
  , image_(0)
  , loadingGraphics_(0)
  , face_(0)
  , data_(this)
  , curves_(NULL)
  , lastEdit_(NULL)
  , autocorrect_video_(true)
  , advance_on_successful_left_click_(false)
  , iframe_(0)
{ 
  //setContextMenuPolicy(Qt::ActionsContextMenu);

  ///// Setup the initial scene
  scene_ = new QGraphicsScene;
  curves_ = new CurveGroup(scene_,this);  

  TRY(connect(curves_,SIGNAL(removeRequest(int,int)),&data_,SLOT(remove(int,int))),ErrorConnect);
  TRY(connect(curves_,SIGNAL(clicked(int)),this,SLOT(setToCurrentIdentByWid(int))),ErrorConnect);

  // scene
  //      \-- drop target
  //      \-- data items
  //            \-- video (pixmap) item
  //            \-- whisker curves
  //      \-- loading item (BROKEN - DOES NOTHING)
  { scene_->setBackgroundBrush(Qt::white);

    image_           = new Frame(this);
    image_->setCursor(Qt::CrossCursor);
    droptarget_      = new QGraphicsSvgItem(":/images/droptarget");
    dataItemsRoot_   = new QGraphicsWidget;
    loadingGraphics_ = new LoadingGraphicsWidget;
    face_            = new FaceIndicator;
    lastEdit_        = new Curve;
    face_->setPos(droptarget_->boundingRect().center());
    scene_->addItem(droptarget_);
    image_->setParentItem(dataItemsRoot_);
    face_->setParentItem(dataItemsRoot_);

    lastEdit_->setParentItem(dataItemsRoot_);    
    lastEdit_->setPen(QPen(Qt::black));
    lastEdit_->setSelectable(false);

    scene_->addItem(dataItemsRoot_);
    scene_->addItem(loadingGraphics_);

    // face position item - editor - data communication
    // XXX: This is a hacky mess.
    TRY(connect( this ,SIGNAL(facePositionChanged(QPointF)),
                &data_,SIGNAL(facePositionChanged(QPointF))),ErrorConnect);

    TRY(connect(&data_,SIGNAL(facePositionChanged(QPointF)),
                 face_,SLOT(setPosition(QPointF))),ErrorConnect);
    TRY(connect(&data_,SIGNAL(faceOrientationChanged(Data::Orientation)),
                 face_,SLOT(setOrientation(Data::Orientation))),ErrorConnect);

    TRY(connect(face_,SIGNAL(rotationChanged()),this,SLOT(updateFromFaceAnchor())),ErrorConnect);
    TRY(connect(face_,SIGNAL(xChanged()),this,SLOT(updateFromFaceAnchor())),ErrorConnect);
    TRY(connect(face_,SIGNAL(yChanged()),this,SLOT(updateFromFaceAnchor())),ErrorConnect);
  }

  ///// responses to data signals
  { TRY(connect(&data_,SIGNAL(success()),this,SLOT(maybeNextFrame())),ErrorConnect);
    TRY(connect(&data_,SIGNAL(lastCurve(QPolygonF)),this,SLOT(maybeShowLastCurve(QPolygonF))),ErrorConnect);
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
    TRY(connect(view_,SIGNAL(identChanged(int)),
                this ,  SLOT(selectByIdent(int))),ErrorConnect);
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

  makeActions_();
  return;
ErrorConnect:
  exit(-1);
}

/**
 * Tries to read the requested frame and posts it to the screen.
 * Updates iframe_, which keeps track of where we are
 * in the movie, if the read was successful.
 */
void Editor::showFrame(int index)
{ QPixmap p = data_.frame(index,autocorrect_video_);
  if(!p.isNull())
  { if(iframe_!=index)
      lastEdit_->hide();
    iframe_=index;  // only updates if read was successful    
    emit frameId(iframe_);
    image_->setPixmap(p);

    // add whisker curves
    { int i;
      int nident = data_.maxIdentity()-data_.minIdentity(); //don't add 1 bc interval always has -1 and we don't count -1.
      curves_->beginAdding(iframe_);
      for(i=0;i<data_.curveCount(iframe_);++i)
      {
        QPolygonF shape = data_.curve(iframe_,i);
        int         wid = data_.wid(iframe_,i),
                  ident = data_.identity(iframe_,i);
        curves_->add(shape,wid,ident,nident);
      }
      curves_->endAdding();
      if(view_->ident()!=-1)
        selectByIdent(view_->ident());
    }
  }
  return;
}

void Editor::setToCurrentIdentByWid(int wid)
{ data_.setIdentity(iframe_,wid,view_->ident());
  showFrame(iframe_);
}

void Editor::traceAtAndIdentify(QPointF target)
{
  data_.traceAtAndIdentify(iframe_,target,autocorrect_video_,view_->ident());
  showFrame(iframe_);
}

void Editor::traceAt(QPointF target)
{
  data_.traceAt(iframe_,target,autocorrect_video_);
  showFrame(iframe_);
}

void Editor::traceAtCursor()
{ QPointF target;
  if(last_context_menu_point_.isNull()) // keyboard accelerator activated.  Use current mouse pos.
    target = view_->mapToScene(mapFromGlobal(QCursor::pos()));
  else                                  // use position where context menu was activated
    target = view_->mapToScene(last_context_menu_point_);
  data_.traceAt(iframe_,target,autocorrect_video_);
  showFrame(iframe_);
}

void Editor::setFaceAnchor()
{ QPointF target;
  if(last_context_menu_point_.isNull()) // keyboard accelerator activated.  Use current mouse pos.
    target = view_->mapToScene(mapFromGlobal(QCursor::pos()));
  else                                  // use position where context menu was activated
    target = view_->mapToScene(last_context_menu_point_);
  data_.setFacePosition(target);
  data_.setFaceOrientation(face_->orientation());
  emit facePositionChanged(target);
}

void Editor::updateFromFaceAnchor()
{ QPointF target = face_->pos();
  //qDebug() << target;
  data_.setFacePosition(target);
  data_.setFaceOrientation(face_->orientation());
}

void Editor::setAutocorrect(bool ison)
{ autocorrect_video_ = ison;
  showFrame(iframe_);
}

void Editor::setAdvanceOnSuccessfulClick(bool b)
{ advance_on_successful_left_click_ = b;
}

/**
 * Usually this function is called to referesh the view for a new video.
 * Also calls View::lockTo for the image.
 */
void Editor::showCurrentFrame()
{ 
  if(iframe_>=0 && iframe_<data_.frameCount())
    showFrame(iframe_);
  else
    showFrame(0);
  view_->lockTo(image_);
}

void Editor::maybeNextFrame()
{ if(advance_on_successful_left_click_)
    showFrame(iframe_+1);
}

void Editor::nextFrame()
{ showFrame(iframe_+1);
}
#define NEXTFRAME(X) \
  void Editor::nextFrame##X () \
  { int i = qMin(iframe_+X,data_.frameCount()-1); \
    showFrame(i); \
  }
NEXTFRAME(10);
NEXTFRAME(100);
NEXTFRAME(1000);

void Editor::prevFrame()
{ showFrame(iframe_-1);
}
#define PREVFRAME(X) \
  void Editor::prevFrame##X () \
  { int i = qMax(iframe_-X,0); \
    showFrame(i); \
  }
PREVFRAME(10);
PREVFRAME(100);
PREVFRAME(1000);

void Editor::firstFrame()
{ showFrame(0);
}

void Editor::lastFrame()
{ showFrame(data_.frameCount()-1);
}

void Editor::incIdent()
{ view_->setIdent(view_->ident()+1);
}
#define INCIDENT(X) \
  void Editor::incIdent##X () \
  { view_->setIdent(view_->ident()+X); \
  }
INCIDENT(10);
INCIDENT(100);
INCIDENT(1000);

void Editor::decIdent()
{ view_->setIdent(view_->ident()-1);
}
#define DECIDENT(X) \
  void Editor::decIdent##X () \
  { view_->setIdent(view_->ident()-X); \
  }
DECIDENT(10);
DECIDENT(100);
DECIDENT(1000);

void Editor::open(const QString& fname)
{ data_.open(fname);
  emit loadStarted();
}

void Editor::open(const QUrl& url)
{ data_.open(url);
  emit loadStarted();
}

void Editor::deleteSelected()
{ curves_->removeSelected();
}

/** O(ncurves) search.
 */
void Editor::selectByIdent(int ident)
{ for(int i=0;i<data_.curveCount(iframe_);++i)
    if(ident==data_.identity(iframe_,i))
    { curves_->selectByWid(data_.wid(iframe_,i)); // O(ncurves) search
      return;
    }
  // none found
  curves_->deselectAll();
}

void Editor::makeActions_()
{
  actions_["next"    ]= new QAction(tr("Next frame")             ,this);
  actions_["next10"  ]= new QAction(tr("Jump ahead 10 frames")   ,this);
  actions_["next100" ]= new QAction(tr("Jump ahead 100 frames")  ,this);
  actions_["next1000"]= new QAction(tr("Jump ahead 1000 frames") ,this);
  actions_["prev"    ]= new QAction(tr("Previous frame")         ,this);
  actions_["prev10"  ]= new QAction(tr("Jump back 10 frames")    ,this);
  actions_["prev100" ]= new QAction(tr("Jump back 100 frames")   ,this);
  actions_["prev1000"]= new QAction(tr("Jump back 1000 frames")  ,this);
  actions_["last"    ]= new QAction(tr("Jump to last frame")     ,this);
  actions_["first"   ]= new QAction(tr("Jump to first frame")    ,this);
  actions_["delete"  ]= new QAction(tr("Delete selected curve")  ,this);
  actions_["incIdent"    ]= new QAction(tr("Increment active identity")  ,this);
  actions_["incIdent10"  ]= new QAction(tr("Increment active identity by 10")    ,this);
  actions_["incIdent100" ]= new QAction(tr("Increment active identity by 100")   ,this);
  actions_["incIdent1000"]= new QAction(tr("Increment active identity by 1000")  ,this);
  actions_["decIdent"    ]= new QAction(tr("Decrement active identity")  ,this);
  actions_["decIdent10"  ]= new QAction(tr("Decrement active identity by 10")    ,this);
  actions_["decIdent100" ]= new QAction(tr("Decrement active identity by 100")   ,this);
  actions_["decIdent1000"]= new QAction(tr("Decrement active identity by 1000")  ,this);
  actions_["setFaceAnchor"] = new QAction(QIcon(":/icons/faceindicator"),tr("Place &face anchor")  ,this);
  actions_["traceAt"     ]= new QAction(QIcon(":/icons/traceAt"),tr("&Trace a new curve"),this);
  actions_["autocorrect" ] = new QAction(tr("&Stripe correction"),this);
  actions_["advance"     ] = new QAction(tr("&Advance after edit"),this);

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
  actions_["delete"  ]->setShortcut( QKeySequence( Qt::Key_Backspace));
  actions_["incIdent"        ]->setShortcut( QKeySequence( Qt::Key_Up));
  actions_["incIdent10"      ]->setShortcut( QKeySequence( Qt::Key_Up + Qt::SHIFT));
  actions_["incIdent100"     ]->setShortcut( QKeySequence( Qt::Key_Up + Qt::SHIFT + Qt::CTRL));
  actions_["incIdent1000"    ]->setShortcut( QKeySequence( Qt::Key_Up + Qt::SHIFT + Qt::CTRL + Qt::ALT));
  actions_["decIdent"        ]->setShortcut( QKeySequence( Qt::Key_Down));
  actions_["decIdent10"      ]->setShortcut( QKeySequence( Qt::Key_Down  + Qt::SHIFT));
  actions_["decIdent100"     ]->setShortcut( QKeySequence( Qt::Key_Down  + Qt::SHIFT + Qt::CTRL));
  actions_["decIdent1000"    ]->setShortcut( QKeySequence( Qt::Key_Down  + Qt::SHIFT + Qt::CTRL + Qt::ALT));
  actions_["setFaceAnchor"   ]->setShortcut( QKeySequence( "f"));
  actions_["traceAt"         ]->setShortcut( QKeySequence( "t"));
  actions_["autocorrect"     ]->setShortcut( QKeySequence( "s"));
  actions_["advance"         ]->setShortcut( QKeySequence( "a"));

  TRY(connect(actions_["next"         ],SIGNAL(triggered()),this,SLOT(nextFrame())     ),Error);
  TRY(connect(actions_["next10"       ],SIGNAL(triggered()),this,SLOT(nextFrame10  ()) ),Error);
  TRY(connect(actions_["next100"      ],SIGNAL(triggered()),this,SLOT(nextFrame100 ()) ),Error);
  TRY(connect(actions_["next1000"     ],SIGNAL(triggered()),this,SLOT(nextFrame1000()) ),Error);
  TRY(connect(actions_["prev"         ],SIGNAL(triggered()),this,SLOT(prevFrame())     ),Error);
  TRY(connect(actions_["prev10"       ],SIGNAL(triggered()),this,SLOT(prevFrame10  ()) ),Error);
  TRY(connect(actions_["prev100"      ],SIGNAL(triggered()),this,SLOT(prevFrame100 ()) ),Error);
  TRY(connect(actions_["prev1000"     ],SIGNAL(triggered()),this,SLOT(prevFrame1000()) ),Error);
  TRY(connect(actions_["last"         ],SIGNAL(triggered()),this,SLOT(lastFrame())     ),Error);
  TRY(connect(actions_["first"        ],SIGNAL(triggered()),this,SLOT(firstFrame())    ),Error);
  TRY(connect(actions_["delete"       ],SIGNAL(triggered()),this,SLOT(deleteSelected())),Error);
  TRY(connect(actions_["incIdent"     ],SIGNAL(triggered()),this,SLOT(incIdent())      ),Error);
  TRY(connect(actions_["incIdent10"   ],SIGNAL(triggered()),this,SLOT(incIdent10  ())  ),Error);
  TRY(connect(actions_["incIdent100"  ],SIGNAL(triggered()),this,SLOT(incIdent100 ())  ),Error);
  TRY(connect(actions_["incIdent1000" ],SIGNAL(triggered()),this,SLOT(incIdent1000())  ),Error);
  TRY(connect(actions_["decIdent"     ],SIGNAL(triggered()),this,SLOT(decIdent())      ),Error);
  TRY(connect(actions_["decIdent10"   ],SIGNAL(triggered()),this,SLOT(decIdent10  ())  ),Error);
  TRY(connect(actions_["decIdent100"  ],SIGNAL(triggered()),this,SLOT(decIdent100 ())  ),Error);
  TRY(connect(actions_["decIdent1000" ],SIGNAL(triggered()),this,SLOT(decIdent1000())  ),Error);
  TRY(connect(actions_["setFaceAnchor"],SIGNAL(triggered()),this,SLOT(setFaceAnchor()) ),Error);
  TRY(connect(actions_["traceAt"      ],SIGNAL(triggered()),this,SLOT(traceAtCursor()) ),Error); 
  actions_["autocorrect" ]->setCheckable(true);
  actions_["autocorrect" ]->setChecked(autocorrect_video_);
  TRY(connect(actions_["autocorrect"],SIGNAL(toggled(bool)),this,SLOT(setAutocorrect(bool))),Error);
  actions_["advance" ]->setCheckable(true);
  actions_["advance" ]->setChecked(advance_on_successful_left_click_);
  TRY(connect(actions_["advance"],SIGNAL(toggled(bool)),this,SLOT(setAdvanceOnSuccessfulClick(bool))),Error);
  TRY(connect(actions_["advance"],SIGNAL(toggled(bool)),view_,SLOT(setAdvanceIndicator(bool))),Error);

  foreach(QAction *a,actions_)
  { a->setShortcutContext(Qt::ApplicationShortcut);
    addAction(a);
  }
  return;
Error:
  DIE;
}

QList<QAction*> Editor::videoPlayerActions()
{ static const char* names[] = {
                                "autocorrect",
                                "advance",
                                "next",
                                "next10",
                                "next100",
                                "next1000",
                                "prev",
                                "prev10",
                                "prev100",
                                "prev1000",
                                "last",
                                "first",
                                 NULL
                               };
  const char **c=names;
  QList<QAction*> out;
  while(*c)
    out.append(actions_[*c++]);
  return out;
}


QList<QAction*> Editor::editorActions()
{ static const char* names[] = { "delete",
                                 "incIdent",
                                 "incIdent10",
                                 "incIdent100",
                                 "incIdent1000",
                                 "decIdent",
                                 "decIdent10",
                                 "decIdent100",
                                 "decIdent1000",
                                 NULL
                               };
  const char **c=names;
  QList<QAction*> out;
  while(*c)
    out.append(actions_[*c++]);
  return out;
}

#define countof(a) (sizeof(a)/sizeof(*a))
void Editor::wheelEvent(QWheelEvent *event)
{ void (Editor::*next)(void);
  void (Editor::*prev)(void);
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
  { case 0: next = &Editor::nextFrame;     prev = &Editor::prevFrame;     break;
    case 1: next = &Editor::nextFrame10;   prev = &Editor::prevFrame10;   break;
    case 2: next = &Editor::nextFrame100;  prev = &Editor::prevFrame100;  break;
    case 3:
    case 4:
            next = &Editor::nextFrame1000; prev = &Editor::prevFrame1000; break;
    default:
      HERE;
      exit(-1);
  }

  if(event->delta()>0)
    (this->*next)();
  else if(event->delta()<0)
    (this->*prev)();
}

void Editor::contextMenuEvent(QContextMenuEvent *event)
{ QMenu *m = new QMenu(this);
  const char* as[] = {"setFaceAnchor"
                     ,"traceAt"
                     ,NULL
  };
  const char **a = as;
  last_context_menu_point_=event->pos();
  while(*a)
    m->addAction(actions_[*a++]);
  m->exec(event->globalPos());
  last_context_menu_point_=QPoint();
}


void Editor::maybeShowLastCurve(QPolygonF midline)
{ if(advance_on_successful_left_click_)
  { lastEdit_->setMidline(midline);
    lastEdit_->show();
  }
}