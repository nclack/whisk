#include <QWidget>
#include <QGraphicsView>

class QGraphicsScene;
class QGraphicsSvgItem;
class QUrl;
typedef struct _video_t video_t;

////////////////////////////////////////////////////////////////////////////////
// VIEW                                                            QGRAPHICSVIEW
////////////////////////////////////////////////////////////////////////////////

class View : public QGraphicsView
{ Q_OBJECT

  public:
    View(QGraphicsScene *scene, QWidget *parent=0);

  public slots:
    void lockTo(const QGraphicsItem *item);

  signals:
    void dropped(const QUrl& url);

  protected:

    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    void resizeEvent(QResizeEvent *event);
    void wheelEvent(QWheelEvent *event);

    const QGraphicsItem *lockitem_;
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

  signals:
    void opened(const QString& path);

  protected:
    void makeActions_();
    void wheelEvent(QWheelEvent *event);

    QAction             *nextFrame_,
                        *prevFrame_, 
                        *lastFrame_, 
                        *firstFrame_;
    View                *view_;
    QGraphicsScene      *scene_;
    QGraphicsSvgItem    *droptarget_;
    QGraphicsPixmapItem *image_;
    video_t             *video_;

    int     iframe_;
};
