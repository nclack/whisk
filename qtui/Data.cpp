#include <QtGui>
#include "Data.h"


#define countof(e) (sizeof(e)/sizeof(*(e)))

#define ENDL "\n"
#define HERE            qDebug("%s(%d): HERE"ENDL,__FILE__,__LINE__); 
#define REPORT(expr)    qDebug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#define TRY(expr)       if(!(expr)) {REPORT(expr); goto Error;}
#define SILENTTRY(expr) if(!(expr)) {              goto Error;}
#define DIE             qFatal("%s(%d): Fatal error.  Aborting."ENDL,__FILE__,__LINE__)


enum KIND       ///< Order is important. See maybeLoadAll()
{ VIDEO = 0,    ///< must be 0
  WHISKERS,
  MEASUREMENTS, 
  MAX_KIND,
  UNKNOWN
};

/** This data structure is kind of stupid?
 */
struct result_t { 
  KIND kind;              ///< indicates the Loader type (when relevant)
  video_t *v; 
  Whisker_Seg *w; int wn; 
  Measurements *m; int mn;
  QString whisker_path,
          measurement_path;

  result_t()
    : kind(UNKNOWN)
    , v(0)
    , w(0), wn(0)
    , m(0), mn(0)
    {}
  result_t(Data *d)
    : kind(UNKNOWN)
    , v(d->video_)
    , w(d->curves_)      , wn(d->ncurves_)
    , m(d->measurements_), mn(d->nmeasurements_)
    , whisker_path(d->lastWhiskerFile_)
    , measurement_path(d->lastMeasurementsFile_)
    {}
};

////////////////////////////////////////////////////////////////////////////////
//  PATH MANIPULATION
////////////////////////////////////////////////////////////////////////////////

KIND         guessKind       (const QString & path);    
QFileInfo    measurementsFile(const QString & keypath); ///< The keypath will be used to guess the location.  Doesn't necessarily exist.
QFileInfo    whiskersFile    (const QString & keypath); ///< The keypath will be used to guess the location.  Doesn't necessarily exist.  
QFileInfo    videoFile       (const QString & keypath); ///< The keypath will be used to guess the location.  Doesn't necessarily exist. 
QFileInfo    getInfoForKind  (const QString & keypath, KIND k);

//static const char* fname(const QFileInfo & info)
//{ return info.absoluteFilePath().toLocal8Bit().data(); }

KIND guessKind(const QString & path)
{ QFileInfo info(path);  
  QByteArray b = info.absoluteFilePath().toLocal8Bit();
  const char* fname = b.data();
  char *fmt=0;
  if(!info.isFile()) return UNKNOWN;
  if(!info.exists()) return UNKNOWN;
  if( info.suffix() == "measurements" )
  { TRY(locked::Measurements_File_Autodetect(fname,&fmt));
    return MEASUREMENTS;
  }
  if( info.suffix() == "whiskers" )
  { TRY(locked::Whisker_File_Autodetect(fname,&fmt));
    return WHISKERS;
  }
  if(locked::is_video(fname))
    return VIDEO;
Error:
  return UNKNOWN;
}

/** Assume keypath is a valid path as determined by isValidPath()
 *  \returns QFileInfo.  The file doesn't necessarily exist.
 */
QFileInfo measurementsFile(const QString &keypath)
{ QFileInfo info(keypath);
  QString newname = info.completeBaseName()+ ".measurements"; 
  info.setFile(info.absoluteDir(),newname);
  return info;
}

/** Assume keypath is a valid path as determined by isValidPath()
 *  \returns QFileInfo.  The file doesn't necessarily exist.
 */
QFileInfo whiskersFile(const QString &keypath)
{ QFileInfo info(keypath);
  QString newname = info.completeBaseName()+ ".whiskers"; 
  info.setFile(info.absoluteDir(),newname);
  return info;
}

/** Assume keypath is a valid path as determined by isValidPath().
 *  Will try a few possible file extensions.  If none of them are
 *  present, will return a guess.
 *  \returns QFileInfo.  The file may not exist.
 */
QFileInfo videoFile(const QString &keypath)
{ QFileInfo info(keypath), proposed;
  static const char *tries[] = {".tif",".seq",NULL};
  const char **t = tries;
  do 
  { proposed.setFile( info.completeBaseName()+t[0] );
    if(proposed.exists())
      return proposed;
  } while (++t);
  proposed.setFile(info.completeBaseName()+".mp4"); 
  return proposed;
}

QFileInfo getInfoForKind(const QString & keypath, KIND k)
{ switch(k)
  { case VIDEO:         return videoFile(keypath);
    case WHISKERS:      return whiskersFile(keypath);
    case MEASUREMENTS:  return measurementsFile(keypath);
    default:
      return QFileInfo();
  }
}

////////////////////////////////////////////////////////////////////////////////
//  LOADER
////////////////////////////////////////////////////////////////////////////////

result_t loadOne(const QString &path)
{ result_t  r;
  QFileInfo cur(path);
  QByteArray b = cur.absoluteFilePath().toLocal8Bit();
  const char *fname = b.data();
  switch(r.kind=guessKind(path))
  { case VIDEO:
      TRY(r.v=locked::video_open(fname));
    break;
    case MEASUREMENTS:
      TRY(r.m=locked::Measurements_Table_From_Filename(fname,NULL,&r.mn));
      r.measurement_path = cur.absoluteFilePath();
    break;
    case WHISKERS:
      TRY(r.w=locked::Load_Whiskers(fname,NULL,&r.wn));
      r.whisker_path = cur.absoluteFilePath();
    break;
    break;
    default: // UNKNOWN
      ;
  }
Error:
  return r; // if there's an error the corresponding field(s) in r will be NULL.
}

result_t maybeLoadAll(const QString &path)
{ QFutureSynchronizer<result_t> sync;
  QString cur = path;
  result_t out;
  int k = (int)guessKind(path);
  while(k<MAX_KIND)
  {
    sync.addFuture(QtConcurrent::run(loadOne,cur));

    cur = getInfoForKind(cur,(KIND)++k).absoluteFilePath(); // Move to the next file type
    if(guessKind(cur)!=k)                                   // double check the type
    { qDebug() << "maybeLoadAll - last try: " << cur;
      break;
    }
  }

  //assemble results
  sync.waitForFinished();
  foreach(QFuture<result_t> f,sync.futures())
  { result_t r = f.result();
    switch(r.kind)
    { case VIDEO:
        out.v=r.v;
        break;
      case WHISKERS:
        out.w =r.w;
        out.wn=r.wn;
        break;
      case MEASUREMENTS:
        out.m =r.m;
        out.mn=r.mn;
        break;
      default: // UNKNOWN
        ;
    }
  }
  return out;
}

void savefunc(const result_t &r)
{ 
  if(r.w)
    TRY(Save_Whiskers(
          r.whisker_path.toLocal8Bit().data(),
          NULL,r.w,r.wn));
  if(r.m)
    TRY(Measurements_Table_To_Filename(
          r.measurement_path.toLocal8Bit().data(),
          NULL,r.m,r.mn));
Error:
  return;
}

////////////////////////////////////////////////////////////////////////////////
// DATA                                                                  QOBJECT
////////////////////////////////////////////////////////////////////////////////

Data::Data(QObject *parent)
  : QObject(parent)
  , video_(0)
  , curves_(0)
  , ncurves_(0)
  , measurements_(0)
  , nmeasurements_(0)
  , minIdent_(-1)
  , maxIdent_(-1)
  , watcher_(0)
{ watcher_ = new QFutureWatcher<result_t>(this);
  TRY(connect(watcher_,SIGNAL(finished()),this,SLOT(commit())));
  return;
Error:
  DIE;
}

Data::~Data()
{ saving_.waitForFinished();
  if(video_)        video_close(&video_);
  if(curves_)       { Free_Whisker_Seg_Vec(curves_,ncurves_); curves_ = NULL; }
  if(measurements_) { Free_Measurements_Table(measurements_); measurements_ = NULL; }
}

// DATA    Load and Save ///////////////////////////////////////////////////////


/**
 * Starts loading of various files depending on the type of the file pointed to
 * by \a path.
 *
 * Measurements depend on Whiskers which, in turn, depend on a Video.
 * So, if a Video is pointed to by \path, an attempt will be made to load
 * corresponding whiskers and measurements files.
 */
void Data::open(const QString& path)
{ watcher_->setFuture(QtConcurrent::run(maybeLoadAll,path));
}

void Data::open(const QUrl& path)
{ open(path.toLocalFile());
}

/**
 * Called when an asynchronous load completes.
 * Commits the loaded data and
 * emits loaded() when fiished.
 */
void Data::commit()
{ result_t r = watcher_->future().result();

  // commit in measurements -> whiskers -> video order
  // to respect dependencies
  saving_.waitForFinished();
  // Get rid of the old stuff
  { int dirty_whiskers = ((r.v!=NULL) || (r.w!=NULL)) && (curves_!=NULL);
    if(dirty_whiskers)
    { Free_Whisker_Seg_Vec(curves_,ncurves_); // this takes a huge amount of time (in Debug on Win7)
      curves_=NULL;
    }
    if(measurements_)
    { Free_Measurements_Table(measurements_);
      measurements_=NULL;
    }
  }
  // Load the new stuff
  if(r.m)
  { measurements_  = r.m;
    nmeasurements_ = r.mn;
    lastMeasurementsFile_ = r.measurement_path;    
  }
  if(r.w)
  { curves_  = r.w;
    ncurves_ = r.wn;
    lastWhiskerFile_ = r.whisker_path;    
  }
  if(r.v)
  { if(video_) video_close(&video_);
    video_ = r.v;
  }
  buildMeasurementsIndex_();
  buildCurveIndex_();
  emit loaded();
}

/**
 * Starts an asynchronous call to save current data.
 */
void Data::save()
{ result_t r(this);
  saving_=QtConcurrent::run(savefunc,r);
}

void Data::buildCurveIndex_()
{ 
  curveIndex_.clear();
  if(curves_)
    for(Whisker_Seg *c=curves_;c<curves_+ncurves_;++c)
    { curveIdMap_t cs = curveIndex_[c->time];
      cs.insert(c->id,c);
      curveIndex_.insert(c->time,cs);
    }
}

void Data::buildMeasurementsIndex_()
{ 
  minIdent_=maxIdent_=-1;
  measIndex_.clear();
  if(measurements_)
  { maxIdent_=measurements_->state;
    for(Measurements *m=measurements_;m<measurements_+nmeasurements_;++m)
    { measIdMap_t ms = measIndex_[m->fid];                // ... maybe make a new map for frame
      ms.insert(m->wid,m);
      measIndex_.insert(m->fid,ms);
      minIdent_ = qMin(minIdent_,(int)m->state);
      maxIdent_ = qMax(maxIdent_,(int)m->state);
    }
  }
}

// DATA    Accessing the data //////////////////////////////////////////////////

const QPixmap Data::frame(int iframe, bool autocorrect)
{ Image *im;
  TRY(video_);
  SILENTTRY(im=video_get(video_,iframe,autocorrect));
  { QImage qim(im->array,im->width,im->height,QImage::Format_Indexed8);
    QVector<QRgb> grayscale;
    for(int i=0;i<256;++i)
      grayscale.append(qRgb(i,i,i));
    qim.setColorTable(grayscale);
    QPixmap out = QPixmap::fromImage(qim); 
    Free_Image(im);
    return out;
  }
Error:
  return QPixmap();
}

int Data::frameCount()
{ if(video_)
    return video_frame_count(video_);
  else
    return 0;
}

Whisker_Seg* Data::get_curve_(int iframe, int icurve)
{ QList<Whisker_Seg*> cs;
  cs = curveIndex_.value(iframe,curveIdMap_t())
                  .values();
  TRY(0<=icurve && icurve<cs.size());
  return cs.at(icurve);
Error:
  return NULL;
}

Measurements* Data::get_meas_(int iframe, int icurve)
{ QList<Measurements*> ms;
  ms = measIndex_.value(iframe,measIdMap_t())
                 .values();
  TRY(0<icurve && icurve<ms.size());
  return ms.at(icurve);
Error:
  return NULL;
}

QPolygonF Data::curve(int iframe, int icurve)
{ QPolygonF c;
  Whisker_Seg *w=0;
  TRY(w=get_curve_(iframe,icurve));
  for(int i=0;i<w->len;++i)
    c << QPointF(w->x[i],w->y[i]);
Error:
  return c;
}

int Data::curveCount(int iframe)
{ return curveIndex_.value(iframe,curveIdMap_t()).size();
}

int Data::identity(int iframe, int icurve)
{ Measurements *row;
  Whisker_Seg  *w;
  TRY(  w=get_curve_(iframe,icurve))  // icurve is not necessarily the whisker id, so look up the whisker first
  TRY(row= measIndex_.value(iframe,measIdMap_t())
                     .value(w->id,NULL));
  return row->state;
Error:
  return -1; //unknown
}

int Data::minIdentity()
{ return minIdent_; }

int Data::maxIdentity()
{ return maxIdent_; }

bool Data::isValidPath(const QString & path)
{ return guessKind(path)!=UNKNOWN;
}
