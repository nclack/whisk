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
#include <QtGui>
#include "LockedCalls.h"

struct result_t;

class Data : public QObject
{ Q_OBJECT
  public:
    Data(QObject *parent=NULL);
    virtual ~Data();

    const QPixmap  frame       (int iframe, bool autocorrect=true);
              int  frameCount  ();
        QPolygonF  curve       (int iframe, int icurve); ///< \returns the curve as an ordered set of points
              int  curveCount  (int iframe);
              int  identity    (int iframe, int icurve); ///< \returns the identity (typ. an int >=0) of the curve or -1 if unknown.
              int  minIdentity ();                       ///< \returns the minimum identity label across the entire data set
              int  maxIdentity ();                       ///< \returns the minimum identity label across the entire data set 

    static bool isValidPath(const QString& path);        ///< \returns true if, at first glance, the path seems to point to something relevant.

  protected:
    Whisker_Seg   *get_curve_(int iframe,int icurve);    ///< icurve is NOT the "whisker id"
    Measurements  *get_meas_(int iframe,int icurve);     ///< icurve is NOT the "whisker id" 
  public slots:
    void open(const QString& path);
    void open(const QUrl& path);                         ///< Currently only handle local paths
    void save();

  signals:
    void loaded();

  protected slots:
    void commit();                                         ///< recieves a finished QFutureWatcher*
                                                         
  public: //pseudo-private
    typedef QMap<int,Whisker_Seg*>           curveIdMap_t;///<        id->Whisker_Seg* 
    typedef QMap<int,curveIdMap_t>           curveMap_t;  ///< frame->id->Whisker_Seg*
    typedef QMap<int,Measurements*>          measIdMap_t; ///<        id->Measurements*
    typedef QMap<int,measIdMap_t>            measMap_t;   ///< frame->id->Measurements* 

    video_t          *video_;

    Whisker_Seg      *curves_;        
    int               ncurves_;       
    curveMap_t        curveIndex_;

    Measurements     *measurements_;  
    int               nmeasurements_;
    measMap_t         measIndex_;
    int               minIdent_;
    int               maxIdent_;

    QFutureWatcher<result_t> *watcher_;
    QFuture<void>     saving_;
    QString           lastWhiskerFile_;
    QString           lastMeasurementsFile_;

    void buildCurveIndex_();
    void buildMeasurementsIndex_();
};
