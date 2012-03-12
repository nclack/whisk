#pragma once
#include<QtGui>
#include"Data.h"

class QSvgRenderer;

class FaceIndicator : public QGraphicsObject
{ Q_OBJECT
  
  public:

    FaceIndicator(QGraphicsItem *parent=0);
    const QPointF&    position()    const;
    const Data::Orientation orientation() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QRectF boundingRect() const;

  public slots:
    void setPosition(const QPointF &r);
    void setOrientation(Data::Orientation o);
    void toggleOrientation();

  signals:
    void changed();

  protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);
    QPointF       pos_;
    Data::Orientation   orient_;
    QSvgRenderer *shape_;
};
