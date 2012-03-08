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

////////////////////////////////////////////////////////////////////////////////
// VIEW                                                            QGRAPHICSVIEW
////////////////////////////////////////////////////////////////////////////////

class View : public QGraphicsView
{ Q_OBJECT

  public:
    View(QGraphicsScene *scene, QWidget *parent=0);

  public slots:
    void lockTo(const QGraphicsItem *item);
    void setFrame(int iframe);

  signals:
    void dropped(const QUrl& url);

  protected:
    void drawForeground(QPainter *painter,const QRectF &rect);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    void resizeEvent(QResizeEvent *event);
    void wheelEvent(QWheelEvent *event);

    const QGraphicsItem *lockitem_;                 ///< View transforms the scene to fit to this item

    int iframe_;
};

////////////////////////////////////////////////////////////////////////////////
// DISPLAY                                                               QWIDGET
////////////////////////////////////////////////////////////////////////////////
class Display : public QWidget
{ Q_OBJECT

  public:
    Display(QWidget *parent=0,Qt::WindowFlags f=0);

  public slots:
    void open(const QUrl& path);
    void showFrame(int index);
    void showCurrentFrame();
    void nextFrame();
    void nextFrame10();
    void nextFrame100();
    void nextFrame1000();
    void prevFrame();
    void prevFrame10();
    void prevFrame100();
    void prevFrame1000();
    void firstFrame();
    void lastFrame();
    void deleteSelected();

  signals:
    void loadStarted();
    void opened(const QString& path);
    void frameId(int iframe);

  protected:
    void makeActions_();
    void wheelEvent(QWheelEvent *event);

    QMap<QString,QAction*>  actions_;
    View                   *view_;          
    QGraphicsScene         *scene_;         
    QGraphicsSvgItem       *droptarget_;    
    QGraphicsWidget        *dataItemsRoot_; 
    QGraphicsPixmapItem    *image_;         
    LoadingGraphicsWidget  *loadingGraphics_;
    Data                    data_;
    CurveGroup             *curves_;

    int     iframe_;
};
