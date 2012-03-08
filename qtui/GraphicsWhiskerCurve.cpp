#include "GraphicsWhiskerCurve.h"

#define ENDL "\n"
#define HERE         qDebug("%s(%d): HERE"ENDL,__FILE__,__LINE__); 
#define REPORT(expr) qDebug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#define TRY(expr,lbl) if(!(expr)) {REPORT(expr); goto lbl;}

static const QColor default_color = QColor(255,155,55,255);

GraphicsWhiskerCurve::GraphicsWhiskerCurve(QGraphicsItem* parent)
  : QGraphicsObject(parent)
  , wid_(-1)
  , is_selected_(0)
{ setAcceptedMouseButtons(Qt::LeftButton);
  setFlags(ItemIsSelectable
          |ItemClipsToShape
          );

  setPen(QPen(QBrush(default_color),
              1.0,             //width
              Qt::DotLine,     //pen style
              Qt::RoundCap,    //pen cap style
              Qt::RoundJoin)); //join style
}

QRectF GraphicsWhiskerCurve::boundingRect() const
{ return midline_.boundingRect();
}

void GraphicsWhiskerCurve::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{ 
  painter->setPen(pen_);
#if 0
  QPainterPath p;
  //p.addPolygon(midline_);
  p.addPolygon(outline_);
  painter->drawPath(p);
#endif
  painter->drawPath(shape());
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

  QPointF shift(0.0,5.0);
  outline_.clear();
  if(!midline.empty())
  {
    for(int i=0;i<midline_.size();++i)
    { outline_ << (midline_.at(i)+shift);
    }
    for(int i=midline_.size()-1;i>=0;--i)
    { outline_ << (midline_.at(i)-shift);
    }
    outline_<<outline_.at(0); //close the contour
  }
}

void GraphicsWhiskerCurve::setPen(const QPen& pen)
{pen_=pen;}

void GraphicsWhiskerCurve::setColor(const QColor& color)
{pen_.setColor(color);
}

void GraphicsWhiskerCurve::setColorByIdentity(int ident, int nident)
{ QColor c(default_color); 
  if(-1!=ident)
    c.setHsvF(ident/(qreal)nident,1.0,1.0);
  setColor(c);
}

void GraphicsWhiskerCurve::setSelected(bool select)
{is_selected_=select;}

int GraphicsWhiskerCurve::wid() const
{return wid_;}

const QPolygonF& GraphicsWhiskerCurve::midline() const
{return midline_;}

const QPen& GraphicsWhiskerCurve::pen() const
{return pen_;}

const QColor GraphicsWhiskerCurve::color() const
{ return pen_.color();
}

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
  qDebug("Clicked %5d"ENDL,wid_);
  emit clicked(wid_);
}
