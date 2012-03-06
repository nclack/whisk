/**
 * \todo load measurements, whiskers, and video explicitly
 *       \todo add slots
 *       \todo from a file menu
 *       \todo via drops (drop routes to correct slot via extension)
 *
 * \notes
 * - negative one (-1) is a special curve identity.  It represents an unknown identity.
 * - If the whiskers or measurements arrays get reordered or changed, their
 *   respective indexes need to be rebuilt. 
 */
#include <QtCore>
#include "LockedCalls.h"

struct result_t;

class Data : public QObject
{ Q_OBJECT
  public:
    Data(QObject *parent=NULL);
    virtual ~Data();

    const QPixmap  frame       (int iframe, bool autocorrect=true);
              int  frameCount  ();
    QList<QPointF> curve       (int iframe, int icurve); ///< \returns the curve as an ordered set of points
              int  curveCount  (int iframe);
              int  identity    (int iframe, int icurve);
              int  minIdentity ();                       ///< \returns the minimum identity label across the entire data set
              int  maxIdentity ();                       ///< \returns the minimum identity label across the entire data set 

    static bool isValidPath(const QString& path);        ///< \returns true if, at first glance, the path seems to point to something relevant.

  public slots:
    void open(const QString& path);
    void open(const QUrl& path);                         ///< Currently only handle local paths
    void save();

  signals:
    void loaded();

  protected slots:
    void commit();                                         ///< recieves a finished QFutureWatcher*
                                                         
  public: //pseudo-private
    typedef QPair<int,int>                   curveKey_t;   ///< (frameid,wid)
    typedef QMap<curveKey_t,Whisker_Seg*>    curveIndex_t;
    typedef QMap<int,int>                    curveCount_t;
    typedef QMap<curveKey_t,Measurements*>   measIndex_t;

    video_t          *video_;

    Whisker_Seg      *curves_;        
    int               ncurves_;       
    curveIndex_t      curveIndex_;
    curveCount_t      curveCounts_;

    Measurements     *measurements_;  
    int               nmeasurements_;
    measIndex_t       measIndex_;
    int               minIdent_;
    int               maxIdent_;

    QFutureWatcher<result_t> *watcher_;
    QFuture<void>     saving_;
    QString           lastWhiskerFile_;
    QString           lastMeasurementsFile_;

    void buildCurveIndex_();
    void buildMeasurementsIndex_();
};
