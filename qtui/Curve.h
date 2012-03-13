/** \file
 *  \todo rename to Curve
 */
#pragma once
#include <QtGui>

class Curve : public QGraphicsObject
{ Q_OBJECT
  Q_PROPERTY(int           wid READ wid        WRITE setWid)
  Q_PROPERTY(QPolygonF midline READ midline    WRITE setMidline)
  Q_PROPERTY(QPen          pen READ pen        WRITE setPen)
  Q_PROPERTY(QColor      color READ color      WRITE setColor)
  Q_PROPERTY(bool     selected READ isSelected WRITE setSelected NOTIFY selected)
  public:
    Curve(QGraphicsItem *parent=0);

    QRectF boundingRect() const;
    void   paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QPainterPath shape() const;

    void setColorByIdentity(int ident, int nident);

    int              wid()   const;
    const QPolygonF& midline() const;
    const QPen&      pen()   const;
    const QColor     color() const;
    bool             isSelected() const;

    QPointF nearest(QPointF query) const;
    double distance(QPointF query) const;

  public slots:
    void select();
    void setWid(int wid);
    void setMidline(const QPolygonF& midline);
    void setPen(const QPen& pen);
    void setColor(const QColor& color);
    void setSelected(bool select=true);
    void setSelectable(bool);

  protected slots:
    void light();
    void dark();

  signals:
    void clicked(int wid);
    void selected();

  protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void hoverEnterEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*); 

  protected:
    int       wid_;
    QPolygonF midline_;
    QPolygonF outline_;
    QPainterPath collisionBounds_;
    QPen      pen_;
    int       is_selected_;
    int       is_selectable_;
    int       is_hovered_;
    float     highlight_alpha_;
};

class CurveGroup : public QObject
{ Q_OBJECT
  public:
    CurveGroup(QGraphicsScene* scene, QObject *parent=NULL);

    Curve* nearest(QPolygonF midline); ///> returns nearest curve

  public slots:
    void beginAdding(int iframe);
    void add(QPolygonF shape,int wid,int ident,int nident);
    void endAdding();
    void selectByWid(int wid);
    void deselectAll();
    void removeSelected();

  signals:
    void removeRequest(int fid, int wid);
    void clicked(int wid);

  protected slots:
    void selection(QObject* target);    ///< ensures only one curve is selected

  protected:
    QSignalMapper   mapper_;
    int             cursor_;
    QGraphicsScene *scene_;
    QList<Curve*>   curves_;
    int             iframe_;
};
