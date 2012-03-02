#include "Display.h"

#include <QtGui>
#include <QGLWidget>
#include <QGraphicsView>
#include <QGraphicsSvgItem>

#include "video.h"

#include <QDebug>

#define ENDL "\n"
#define HERE         qDebug("%s(%d): HERE"ENDL,__FILE__,__LINE__); 
#define REPORT(expr) qDebug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#define TRY(expr,lbl) if(!(expr)) {REPORT(expr); goto lbl;}

////////////////////////////////////////////////////////////////////////////////
// VIEW         QGRAPHICSVIEW
////////////////////////////////////////////////////////////////////////////////

View::View(QGraphicsScene *scene, QWidget *parent)
  : QGraphicsView(scene,parent)
  , lockitem_(0)
{ setAcceptDrops(true);
  setResizeAnchor(AnchorViewCenter);
  setDragMode(ScrollHandDrag);
  setAlignment(Qt::AlignCenter);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setRenderHints( QPainter::Antialiasing
                | QPainter::SmoothPixmapTransform
                | QPainter::HighQualityAntialiasing
                | QPainter::TextAntialiasing 
      );
  //setBackgroundBrush(QBrush(Qt::black));

  QGLWidget *viewport = new QGLWidget ;
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
      if( url.isLocalFile() && is_video(url.toLocalFile().toLocal8Bit().constData()) ) /// \todo accept a wider range of drops...folders, whiskers files, etc.
      { event->acceptProposedAction();
        return;
      }
}

void View::dragMoveEvent(QDragMoveEvent *event)
{ event->accept();
}

void View::dropEvent(QDropEvent *event)
{ if(event->mimeData()->hasFormat("text/uri-list"))
    foreach(QUrl url, event->mimeData()->urls())
      if( url.isLocalFile() && is_video(url.toLocalFile().toLocal8Bit().constData()) ) /// \todo accept a wider range of drops...folders, whiskers files, etc.
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
  , image_(0)
  , video_(0)
  , iframe_(0)
{ makeActions_();

  scene_ = new QGraphicsScene;
  // Setup the initial scene
  { scene_->setBackgroundBrush(Qt::white);

    image_ = new QGraphicsPixmapItem();
    droptarget_= new QGraphicsSvgItem(":/images/droptarget");
    scene_->addItem(droptarget_);
    scene_->addItem(image_);
  }

  // Init the viewport
  { QVBoxLayout *layout = new QVBoxLayout;
    view_ = new View(scene_,this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(view_);
    setLayout(layout);

    TRY(connect(view_,SIGNAL(dropped(const QUrl&)),
                this ,  SLOT(   open(const QUrl&))),ErrorConnect);
  }
  return;
ErrorLoadDroptarget:
ErrorConnect:
  exit(-1);
}

/**
 * Tries to read the requested frame and posts it to the screen.
 * Updates iframe_, which keeps track of where we are
 * in the movie, if the read was successful.
 */
void Display::showFrame(int index)
{
  Image *im;
  TRY(im=video_get(video_,index,1),ErrorGetFrame);
  iframe_=index;  // only updates if read was successful
  { QImage qim(im->array,im->width,im->height,QImage::Format_Indexed8);
    QVector<QRgb> grayscale;
    for(int i=0;i<256;++i)
      grayscale.append(qRgb(i,i,i));
    qim.setColorTable(grayscale);
    image_->setPixmap( QPixmap::fromImage(qim));
  }
  Free_Image(im);
ErrorGetFrame:
  return;
}

void Display::nextFrame()
{ if(video_ && iframe_<video_frame_count(video_)-1)
    showFrame(iframe_+1);
}
#define NEXTFRAME(X) \
  void Display::nextFrame##X () \
  { if(video_ && iframe_<video_frame_count(video_)-X) \
      showFrame(iframe_+X); \
  }
NEXTFRAME(10);
NEXTFRAME(100);
NEXTFRAME(1000);

void Display::prevFrame()
{ if(iframe_>0)
    showFrame(iframe_-1);
}
#define PREVFRAME(X) \
  void Display::prevFrame##X () \
  { if( (iframe_-X)>=0 ) \
      showFrame(iframe_-X); \
  }
PREVFRAME(10);
PREVFRAME(100);
PREVFRAME(1000);

void Display::firstFrame()
{ showFrame(0);
}

void Display::lastFrame()
{ if(video_)
    showFrame(video_frame_count(video_)-1);
}

void Display::open(const QUrl& url)
{ if(video_)
    video_close(&video_);
  TRY(video_=video_open(url.toLocalFile().toLocal8Bit().constData()),Error);
  showFrame(iframe_=0);
  view_->lockTo(image_);
  droptarget_->setVisible(false);
  emit opened(url.toLocalFile());
  return;
Error:
  return;
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
    for(int i=0; i<countof(mods);++i)
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
