#pragma once
#include <QWidget>
#include <QGraphicsView>
#include "Data.h"

class QGraphicsScene;
class QGraphicsSvgItem;
class QGraphicsWidget;
class QUrl;
class LoadingGraphicsWidget;
class CurveGroup;
class Curve;
class Frame;
class FaceIndicator;


////////////////////////////////////////////////////////////////////////////////
// VIEW                                                            QGRAPHICSVIEW
////////////////////////////////////////////////////////////////////////////////

class View : public QGraphicsView
{ Q_OBJECT

  public:
    View(QGraphicsScene *scene, QWidget *parent=0);

    int ident();

  public slots:
    void lockTo(const QGraphicsItem *item);
    void setFrame(int iframe);
    void invalidateForeground();
    void setIdent(int ident);
    void setAdvanceIndicator(bool);
    void setAutoModeIndicator(bool);

  signals:
    void dropped(const QUrl& url);
    void identChanged(int);

  protected:
    void drawForeground(QPainter *painter,const QRectF &rect);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    void resizeEvent(QResizeEvent *event);
    void wheelEvent(QWheelEvent *event);

    const QGraphicsItem *lockitem_;                 ///< View transforms the scene to fit to this item

    int  iframe_;
    int  ident_;
    bool indicate_advance_;
    bool indicate_auto_;
};

////////////////////////////////////////////////////////////////////////////////
// DISPLAY                                                               QWIDGET
////////////////////////////////////////////////////////////////////////////////
class Editor : public QWidget
{ Q_OBJECT

  public:
    Editor(QWidget *parent=0,Qt::WindowFlags f=0);

    Data* data() {return &data_;}
    QList<QAction*> tracingActions();
    QList<QAction*> videoPlayerActions();
    QList<QAction*> identityActions();

  public slots:
    void open(const QString& path);
    void open(const QUrl&    path);
    void showFrame(int index);
    void showCurrentFrame();
    void maybeNextFrame();
    void nextFrame();
    void nextFrame10();
    void nextFrame100();
    void nextFrame1000();
    void prevFrame();
    void prevFrame10();
    void prevFrame100();
    void prevFrame1000();
    void incIdent();
    void incIdent10();
    void incIdent100();
    void incIdent1000();
    void decIdent();
    void decIdent10();
    void decIdent100();
    void decIdent1000();
    void firstFrame();
    void lastFrame();
    void deleteSelected();
    void selectByIdent(int ident);
    void setToCurrentIdentByWid(int wid);
    void traceAt(QPointF target);
    void traceAtAndIdentify(QPointF target);
    void traceAtCursor();
    void setFaceAnchor();        ///< Response from editor action.  Updates FaceIndicator and Data.
    void updateFromFaceAnchor(); ///< Commit state of FaceIndicator item to Data.
    void setAutocorrect(bool);
    void setAdvanceOnSuccessfulClick(bool);
    void setAutoMode(bool);

  signals:
    void loadStarted();
    void opened(const QString& path);
    void frameId(int iframe);
    void facePositionChanged(QPointF);
    void propigateTrace(QPointF);
    void propigateIdentity(int query_frame,int query_wid);

  protected:
    void makeActions_();
    void wheelEvent(QWheelEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

  protected slots:
    void maybeShowLastCurve(QPolygonF);
    void propigateTraceHandler(QPointF);
    void propigateIdentityHandler(int query_frame,int query_wid);

  protected:
    QMap<QString,QAction*>  actions_;
    View                   *view_;
    QGraphicsScene         *scene_;
    QGraphicsSvgItem       *droptarget_;
    QGraphicsWidget        *dataItemsRoot_;
    Frame                  *image_;
    LoadingGraphicsWidget  *loadingGraphics_;
    FaceIndicator          *face_;
    Data                    data_;
    CurveGroup             *curves_;
    Curve                  *lastEdit_;
    QPoint                  last_context_menu_point_;
    bool                    autocorrect_video_;
    bool                    advance_on_successful_left_click_;
    bool                    is_auto_mode_on_;
    int     iframe_;
};
