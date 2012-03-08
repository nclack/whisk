/** \file
 *  \todo rename to CurveItem
 */
#pragma once
#include <QtGui>

class GraphicsWhiskerCurve : public QGraphicsObject
{ Q_OBJECT
  Q_PROPERTY(int           wid READ wid        WRITE setWid)
  Q_PROPERTY(QPolygonF midline READ midline    WRITE setMidline)
  Q_PROPERTY(QPen          pen READ pen        WRITE setPen)
  Q_PROPERTY(QColor      color READ color      WRITE setColor)
  Q_PROPERTY(bool     selected READ isSelected WRITE setSelected NOTIFY selected)
  public:
    GraphicsWhiskerCurve(QGraphicsItem *parent=0);

    QRectF boundingRect() const;
    void   paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QPainterPath shape() const;

    void setWid(int wid);
    void setMidline(const QPolygonF& midline);
    void setPen(const QPen& pen);
    void setColor(const QColor& color);
    void setSelected(bool select=true);

    void setColorByIdentity(int ident, int nident);

    int              wid()   const;
    const QPolygonF& midline() const;
    const QPen&      pen()   const;
    const QColor     color() const;
    bool             isSelected() const;

  public slots:
    void select();

  signals:
    void clicked(int wid);
    void selected();

  protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);

  protected:
    int       wid_;
    QPolygonF midline_;
    QPolygonF outline_;
    QPen      pen_;
    int       is_selected_;

};
