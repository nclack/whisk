#include "FaceIndicator.h"
#include <QSvgRenderer>

#define ENDL "\n"
#define HERE         qDebug("%s(%d): HERE"ENDL,__FILE__,__LINE__)
#define REPORT(expr) qDebug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr)
#define TRY(expr,lbl) if(!(expr)) {REPORT(expr); goto lbl;}
#define DIE          qFatal("%s(%d): Aborting."ENDL,__FILE__,__LINE__)

FaceIndicator::FaceIndicator(QGraphicsItem *parent)
  : QGraphicsObject(parent)
  , orient_(Data::VERTICAL)
{ shape_=new QSvgRenderer(QString(":/images/faceindicator"),this);
  setFlags(ItemIsMovable);
  setCursor(Qt::ArrowCursor);
  setZValue(5); // put it on the top layer so it's always selectable
}

void FaceIndicator::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{ 
  QRectF r = shape_->viewBoxF();
  r.moveCenter(QPointF(0,0));
  shape_->render(painter,r);
}

QRectF FaceIndicator::boundingRect() const
{ 
  QRectF r = shape_->viewBoxF();
  r.moveCenter(QPointF(0,0));
  return r;
}

const QPointF& FaceIndicator::position() const
{ return pos_;}

const Data::Orientation FaceIndicator::orientation() const
{ return orient_; }

void FaceIndicator::setPosition(const QPointF& r)
{ pos_=r;
  setPos(r);
  emit changed();
}

void FaceIndicator::setOrientation(Data::Orientation orient)
{ if(orient==Data::UNKNOWN_ORIENTATION)
    return;
  orient_=orient; 
  emit changed();
}

void FaceIndicator::toggleOrientation()
{ orient_ = (orient_)?Data::HORIZONTAL:Data::VERTICAL;
  emit changed();
}

void FaceIndicator::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{ if(event->button()==Qt::LeftButton)
  { toggleOrientation();
    if(orient_==Data::HORIZONTAL)
      setRotation(90);
    else
      setRotation(0);
  }
}
