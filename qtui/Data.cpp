#include "Data.h"
#include <QtGui>

#define countof(e) (sizeof(e)/sizeof(*(e)))

#define ENDL "\n"
#define HERE         qDebug("%s(%d): HERE"ENDL,__FILE__,__LINE__); 
#define REPORT(expr) qDebug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#define TRY(expr)    if(!(expr)) {REPORT(expr); goto Error;}
#define DIE          qFatal("%s(%d): Fatal error.  Aborting."ENDL,__FILE__,__LINE__)


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

static const char* fname(const QFileInfo & info)
{ return info.absoluteFilePath().toLocal8Bit().data(); }

KIND guessKind(const QString & path)
{ QFileInfo info(path);
  char *fmt=0;
  if(!info.isFile()) return UNKNOWN;
  if(!info.exists()) return UNKNOWN;
  if( info.suffix() == "measurements" )
  { TRY(locked::Measurements_File_Autodetect(fname(info),&fmt));
    return MEASUREMENTS;
  }
  if( info.suffix() == "whiskers" )
  { TRY(locked::Whisker_File_Autodetect(fname(info),&fmt));
    return WHISKERS;
  }
  if(locked::is_video(fname(info)))
    return VIDEO;
Error:
  return UNKNOWN;
}

/** Assume keypath is a valid path as determined by isValidPath()
 *  \returns QFileInfo.  The file doesn't necessarily exist.
 */
QFileInfo measurementsFile(const QString &keypath)
{ QFileInfo info(keypath);
  QString newname = info.completeBaseName()+".measurements"; 
  return QFileInfo(newname);
}

/** Assume keypath is a valid path as determined by isValidPath()
 *  \returns QFileInfo.  The file doesn't necessarily exist.
 */
QFileInfo whiskersFile(const QString &keypath)
{ QFileInfo info(keypath);
  QString newname = info.completeBaseName()+".whiskers"; 
  return QFileInfo(newname);
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
  switch(r.kind=guessKind(path))
  { case VIDEO:
      TRY(r.v=locked::video_open(fname(cur)));
    break;
    case MEASUREMENTS:
      TRY(r.m=locked::Measurements_Table_From_Filename(fname(cur),NULL,&r.mn));
      r.measurement_path = cur.absoluteFilePath();
    break;
    case WHISKERS:
      TRY(r.w=locked::Load_Whiskers(fname(cur),NULL,&r.wn));
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
Error:
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
{ TRY(path.isLocalFile());
  open(path.toLocalFile());
Error:
  ;
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
  if(r.m)
  { if(measurements_) Free_Measurements_Table(measurements_);
    measurements_  = r.m;
    nmeasurements_ = r.mn;
    lastMeasurementsFile_ = r.measurement_path;
    buildMeasurementsIndex_();
  }
  if(r.w)
  { if(curves_) Free_Whisker_Seg_Vec(curves_,ncurves_); 
    curves_  = r.w;
    ncurves_ = r.wn;
    lastWhiskerFile_ = r.whisker_path;
    buildCurveIndex_();
  }
  if(r.v)
  { if(video_) video_close(&video_);
    video_ = r.v;
  }
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
  curveCounts_.clear();
  if(curves_)
    for(Whisker_Seg *c=curves_;c<curves_+ncurves_;++c)
    { curveIndex_[qMakePair(c->time,c->id)] = c;
      curveCounts_.insert(  // i.e. a safe curveCounts[time]++
          c->time,
          curveCounts_.value(c->time,0)+1);
    }
}

void Data::buildMeasurementsIndex_()
{ minIdent_=maxIdent_=-1;
  measIndex_.clear();
  if(measurements_)
  { maxIdent_=measurements_->data[0];
    for(Measurements *m=measurements_;m<measurements_+nmeasurements_;++m)
    { measIndex_[qMakePair(m->fid,m->wid)] = m;
      minIdent_ = qMin(minIdent_,(int)m->data[0]);
      maxIdent_ = qMax(maxIdent_,(int)m->data[0]);
    }
  }
}

// DATA    Accessing the data //////////////////////////////////////////////////

const QPixmap Data::frame(int iframe, bool autocorrect)
{ Image *im;
  TRY(video_);
  TRY(im=video_get(video_,iframe,autocorrect));
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

QList<QPointF> Data::curve(int iframe, int icurve)
{ QList<QPointF> c;
  Whisker_Seg *w=0;
  TRY(w=curveIndex_.value(qMakePair(iframe,icurve),NULL));
  for(int i=0;i<w->len;++i)
    c.append(QPointF(w->x[i],w->y[i]));
Error:
  return c;
}

int Data::curveCount(int iframe)
{ return curveCounts_.value(iframe,0);
}

int Data::identity(int iframe, int icurve)
{ Measurements *row; 
  TRY(row=measIndex_.value(qMakePair(iframe,icurve),NULL));
  return row->data[0];
Error:
  return -1;
}

int Data::minIdentity()
{ return minIdent_; }

int Data::maxIdentity()
{ return maxIdent_; }

bool Data::isValidPath(const QString & path)
{ return guessKind(path)!=UNKNOWN;
}
