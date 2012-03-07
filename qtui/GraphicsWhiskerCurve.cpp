#include "GraphicsWhiskerCurve.h"

#define ENDL "\n"
#define HERE         qDebug("%s(%d): HERE"ENDL,__FILE__,__LINE__); 
#define REPORT(expr) qDebug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#define TRY(expr,lbl) if(!(expr)) {REPORT(expr); goto lbl;}

GraphicsWhiskerCurve::GraphicsWhiskerCurve(QGraphicsItem* parent)
  : QGraphicsObject(parent)
  , wid_(-1)
  , is_selected_(0)
{ setAcceptedMouseButtons(Qt::LeftButton);
  setFlags(ItemIsSelectable
          |ItemClipsToShape
          );
}

QRectF GraphicsWhiskerCurve::boundingRect() const
{ return midline_.boundingRect();
}

void GraphicsWhiskerCurve::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{ 
  QPainterPath p;
  p.addPolygon(midline_);
  painter->setPen(pen_);
  painter->drawPath(p);
}

QPainterPath GraphicsWhiskerCurve::shape() const
{
  QPainterPath p;
  p.addRect(midline_.boundingRect());
  return p;
}

void GraphicsWhiskerCurve::setWid(int wid)
{wid_=wid;}

void GraphicsWhiskerCurve::setMidline(const QPolygonF& midline)
{ midline_=midline;
}

void GraphicsWhiskerCurve::setPen(const QPen& pen)
{pen_=pen;}

void GraphicsWhiskerCurve::setSelected(bool select)
{is_selected_=select;}

int GraphicsWhiskerCurve::wid() const
{return wid_;}

const QPolygonF& GraphicsWhiskerCurve::midline() const
{return midline_;}

const QPen& GraphicsWhiskerCurve::pen() const
{return pen_;}

bool GraphicsWhiskerCurve::isSelected() const
{return is_selected_;}

void GraphicsWhiskerCurve::select()
{ setSelected();
  emit selected();
}

void GraphicsWhiskerCurve::mousePressEvent(QGraphicsSceneMouseEvent* e)
{ if(e->button()!=Qt::LeftButton)
  { e->ignore();
    return;
  }
  HERE;
  emit clicked(wid_);
}
