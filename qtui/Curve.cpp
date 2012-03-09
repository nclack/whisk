#include "Curve.h"

#define ENDL "\n"
#define HERE         qDebug("%s(%d): HERE"ENDL,__FILE__,__LINE__); 
#define REPORT(expr) qDebug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#define TRY(expr)    if(!(expr)) {REPORT(expr); goto Error;}

////////////////////////////////////////////////////////////////////////////////
//  CURVE                                                        QGRAPHICSOBJECT
////////////////////////////////////////////////////////////////////////////////

static const QColor default_color = QColor(255,155,55,255);

Curve::Curve(QGraphicsItem* parent)
  : QGraphicsObject(parent)
  , wid_(-1)
  , is_selected_(0)
  , is_hovered_(0)
  , highlight_alpha_(0.15)
{
  setAcceptedMouseButtons(Qt::LeftButton);
  setAcceptHoverEvents(true);
  setFlags(ItemIsSelectable
          |ItemClipsToShape
          );
  setCursor(Qt::ArrowCursor);

  setPen(QPen(QBrush(default_color),
              1.0,             //width
              Qt::SolidLine, //DotLine,     //pen style
              Qt::RoundCap,    //pen cap style
              Qt::RoundJoin)); //join style
}

QRectF Curve::boundingRect() const
{ return midline_.boundingRect();
}

void Curve::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{ QPen   fillpen(QColor(0,0,0,0));
  QBrush nobrush,fillbrush(color());

  if(is_selected_)
  { QColor c = color();
    c.setAlphaF(highlight_alpha_);
    fillbrush.setColor(c);
    painter->setPen(fillpen);
    painter->setBrush(fillbrush);
    { QPainterPath p;
      p.setFillRule(Qt::WindingFill);
      p.addPolygon(outline_);
      painter->drawPath(p);
    }
  }

  painter->setPen(pen_);
  painter->setBrush(nobrush);
  { QPainterPath p;
    p.addPolygon(midline_);
    painter->drawPath(p);
  }
#if 0
  painter->setFont(QFont("Times",12,QFont::Bold));
  painter->drawText(midline_.at(midline_.size()/2),
                    QString("%1").arg(wid_));
#endif
}

QPainterPath Curve::shape() const
{ 
  return collisionBounds_;
}

void Curve::setWid(int wid)
{wid_=wid;}

void Curve::setMidline(const QPolygonF& midline)
{ midline_=midline;

  is_selected_=0;
  is_hovered_=0;
  QPainterPathStroker s;
  s.setWidth(22.0);
  s.setCapStyle(Qt::FlatCap);
  s.setJoinStyle(Qt::BevelJoin);
  QPainterPath p;
  p.addPolygon(midline_);
  collisionBounds_ = s.createStroke(p);
  s.setWidth(20.0);
  outline_ = s.createStroke(p).toFillPolygon();
  update();
}

void Curve::setPen(const QPen& pen)
{pen_=pen;}

void Curve::setColor(const QColor& color)
{pen_.setColor(color);
}

void Curve::setColorByIdentity(int ident, int nident)
{ QColor c(default_color); 
  if(-1!=ident)
    c.setHsvF(ident/(qreal)nident,1.0,1.0);
  setColor(c);
}

void Curve::setSelected(bool select)
{is_selected_=select;}

void Curve::light()
{ highlight_alpha_=0.15;
  update();
}

void Curve::dark()
{ highlight_alpha_=0.15; //0.5;
}

int Curve::wid() const
{return wid_;}

const QPolygonF& Curve::midline() const
{return midline_;}

const QPen& Curve::pen() const
{return pen_;}

const QColor Curve::color() const
{ return pen_.color();
}

bool Curve::isSelected() const
{return is_selected_;}

void Curve::select()
{ 
  dark();
  setSelected(1);
  emit selected();
  QTimer::singleShot(100/*ms*/,this,SLOT(light()));
}

void Curve::mousePressEvent(QGraphicsSceneMouseEvent* e)
{ if(e->button()!=Qt::LeftButton)
  { e->ignore();
    return;
  }
  update();
  emit clicked(wid_); // this has to come before calling select().  
  select();           // Some clicked() recievers may reset the selection state.
}

void Curve::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{ is_hovered_=1;
  update();
}
void Curve::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{ is_hovered_=0;
  update();
}

////////////////////////////////////////////////////////////////////////////////
//  CURVEGROUP                                                           QOBJECT 
////////////////////////////////////////////////////////////////////////////////

CurveGroup::CurveGroup(QGraphicsScene* scene, QObject *parent/*=NULL*/)
  : QObject(parent)
  , cursor_(0)
  , scene_(scene)
{ TRY(connect(&mapper_,SIGNAL(mapped(QObject*)),this,SLOT(selection(QObject*))));
Error:
  return;
}

void CurveGroup::beginAdding(int iframe)
{ cursor_=0;
  iframe_=iframe;
}

void CurveGroup::add(QPolygonF shape, int wid, int ident, int nident)
{ 
  if(cursor_<curves_.size())  // use existing curve items
  { scene_->removeItem(curves_[cursor_]);
    curves_[cursor_]->setMidline(shape);
    curves_[cursor_]->show();
    scene_->addItem(curves_[cursor_]);
  } else                      // or add a new one
  { Curve *pp;
    curves_.append(pp=new Curve()); 
    pp->setMidline(shape);
    scene_->addItem(pp);

    mapper_.setMapping(pp,pp);
    TRY(connect(pp,SIGNAL(selected()),&mapper_,SLOT(map())));
    TRY(connect(pp,SIGNAL(clicked(int)),this,SIGNAL(clicked(int))));
  }
  curves_[cursor_]->setColorByIdentity(ident,nident);
  curves_[cursor_]->setWid(wid);
  cursor_++;
  return;
Error:
  qFatal("Couldn't connect signal.");
}

void CurveGroup::endAdding()
{
  // hide unused curves
  for(;cursor_<curves_.size();++cursor_)
  { curves_[cursor_]->hide();
  }
}

void CurveGroup::selectByWid(int wid)
{ foreach(Curve* c, curves_)
    if(c->wid()==wid)
    { c->select();
      return;
    }
}

void CurveGroup::deselectAll()
{ foreach(Curve *c,curves_)
    c->setSelected(0);
}

void CurveGroup::removeSelected()
{ foreach(Curve *c,curves_)
    if(c->isSelected())
    { c->hide();
      emit removeRequest(iframe_,c->wid());
    }
}

void CurveGroup::selection(QObject *target)
{ foreach(Curve *c,curves_)
    if( ((void*)c)!=((void*)target) )
      c->setSelected(0);
}

