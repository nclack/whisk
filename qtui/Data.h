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
#pragma once

#include <QtCore>
#include <QtGui>
#include "LockedCalls.h"

struct result_t;

class Data : public QObject
{ Q_OBJECT
  public:
    enum Orientation
    { HORIZONTAL=0,
      VERTICAL,
      UNKNOWN_ORIENTATION
    };


    Data(QObject *parent=NULL);
    virtual ~Data();

    const QPixmap  frame       (int iframe, bool autocorrect=true);
              int  frameCount  ();
        QPolygonF  curve       (int iframe, int icurve); ///< \returns the curve as an ordered set of points
        QPolygonF  curveByWid  (int iframe, int wid);
              int  curveCount  (int iframe);
              int  wid         (int iframe, int icurve);
              int  identity    (int iframe, int icurve); ///< \returns the identity (typ. an int >=0) of the curve or -1 if unknown.
              int  minIdentity ();                       ///< \returns the minimum identity label across the entire data set
              int  maxIdentity ();                       ///< \returns the minimum identity label across the entire data set
             bool  isFaceSet();
             bool  areFaceDefaultsSet();
          QPointF  facePosition(int *is_unknown);
      Orientation  faceOrientation();
              int  nextMissingFrame(int iframe, int ident); ///< \returns the next frame number lacking a curve with the given identity
              int  prevMissingFrame(int iframe, int ident); ///< \returns the prev frame number lacking a curve with the given identity

    static bool isValidPath(const QString& path);        ///< \returns true if, at first glance, the path seems to point to something relevant.

  protected:
    Whisker_Seg   *get_curve_(int iframe,int icurve);    ///< icurve is NOT the "whisker id"
    Whisker_Seg   *get_curve_by_wid_(int iframe,int wid);///< \returns NULL if not found
    Measurements  *get_meas_(int iframe,int icurve);     ///< icurve is NOT the "whisker id"
    Measurements  *get_meas_by_wid_(int iframe,int wid); ///< \returns NULL if not found
    bool           is_ident_same_(int iframe, int wid, int ident);
    int            maybePopulateMeasurements();          ///< \returns 1 if measurements table is populated, 0 otherwise
    int            maybeShowFaceAnchorRequiredDialog();  ///< \returns 0 if face anchor defaults are not set, 0 otherwise
    int            get_next_wid_(int iframe);            ///> \returns a good wid for iframe.  Used for appending new curves.

  public slots:
    void open(const QString& path);
    void open(const QUrl& path);                         ///< Currently only handle local paths
    void save();
    void saveAs(const QString &path);                    ///< will try to guess what data to save
    void saveWhiskersAs(const QString &path);
    void saveMeasurementsAs(const QString &path);

    void remove(int iframe, int wid);
    void setIdentity(int iframe, int wid, int ident);
    void setFacePosition(QPointF r);
    void setFaceOrientation(Orientation o);

    void traceAt(int iframe, QPointF r, bool autocorrect=true);
    void traceAtAndIdentify(int iframe,QPointF target,bool autocorrect_video,int ident);
    void traceFrame(int iframe, bool autocorrect);

  signals:
    void loaded();                                        ///< emited when commit of new data is finished
    void curvesSaved();
    void curvesDirtied();
    void measurementsSaved();
    void measurementsDirtied();
    void success();                                       ///< emited after a sucessful edit
    void facePositionChanged(QPointF r);                  ///< only emitted after load
    void faceOrientationChanged(Data::Orientation o);     ///< only emitted after load
    void lastCurve(QPolygonF);                            ///< emits shape of the last edited curve

  protected slots:
    void commit();                                        ///< called once a load succesfully completes to merge loaded data
    void saveDone();                                      ///< called after save.  Save may update data before writing to disk. This commits changes.

  public: //pseudo-private
    typedef QMap<int,Whisker_Seg*>           curveIdMap_t;///<        id->Whisker_Seg*
    typedef QMap<int,curveIdMap_t>           curveMap_t;  ///< frame->id->Whisker_Seg*
    typedef QMap<int,Measurements*>          measIdMap_t; ///<        id->Measurements*
    typedef QMap<int,measIdMap_t>            measMap_t;   ///< frame->id->Measurements*

    video_t          *video_;
    Image            *lastImage_;

    Whisker_Seg      *curves_;
    int               ncurves_;
    int               ncurves_capacity_;
    curveMap_t        curveIndex_;

    Measurements     *measurements_;
    int               nmeasurements_;
    int               nmeasurements_capacity_;
    measMap_t         measIndex_;
    int               minIdent_;
    int               maxIdent_;
    QPointF           faceDefaultAnchor_;
    Orientation       faceDefaultOrient_;

    QFutureWatcher<result_t> *watcher_;
    QFutureWatcher<void>     *save_watcher_;
    QFuture<void>     saving_;
    QString           lastVideoFile_;         ///< gets set whether or not video loads
    QString           lastWhiskerFile_;
    QString           lastMeasurementsFile_;

    void buildCurveIndex_();
    void buildMeasurementsIndex_();
};
