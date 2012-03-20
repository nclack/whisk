#include <QtGui>
#include "Data.h"
#include "LockedCalls.h"

#define countof(e) (sizeof(e)/sizeof(*(e)))

#define ENDL "\n"
#define HERE            qDebug("%s(%d): HERE"ENDL,__FILE__,__LINE__);
#define REPORT(expr)    qDebug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr)
#define TRY(expr)       if(!(expr)) {REPORT(expr); goto Error;}
#define SILENTTRY(expr) if(!(expr)) {              goto Error;}
#define DIE             qFatal("%s(%d): Fatal error.  Aborting."ENDL,__FILE__,__LINE__)

QMutex GlobalDataLock(QMutex::Recursive);
#define LOCK QMutexLocker locker__(&GlobalDataLock)

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
  int face_x,face_y;
  char orientation;

  result_t()
    : kind(UNKNOWN)
    , v(0)
    , w(0), wn(0)
    , m(0), mn(0)
    , face_x(0)
    , face_y(0)
    , orientation('u')
    {}
  result_t(Data *d)
    : kind(UNKNOWN)
    , v(d->video_)
    , w(d->curves_)      , wn(d->ncurves_)
    , m(d->measurements_), mn(d->nmeasurements_)
    , whisker_path(d->lastWhiskerFile_)
    , measurement_path(d->lastMeasurementsFile_)
    , face_x(0)
    , face_y(0)
    , orientation('u')
    { if(!d->faceDefaultAnchor_.isNull())
      { face_x = d->faceDefaultAnchor_.x();
        face_y = d->faceDefaultAnchor_.y();
        orientation = d->faceDefaultOrient_?'v':'h';
      }
    }
};

////////////////////////////////////////////////////////////////////////////////
//  PATH MANIPULATION
////////////////////////////////////////////////////////////////////////////////

KIND         guessKind       (const QString & path);
KIND         guessKindForSave(const QString & path);
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

KIND guessKindForSave(const QString & path)
{ QFileInfo info(path);
  QByteArray b = info.absoluteFilePath().toLocal8Bit();
  const char* fname = b.data();
  char *fmt=0;
  if( info.suffix() == "measurements" ) return MEASUREMENTS;
  if( info.suffix() == "whiskers" )     return WHISKERS;
  // now the only way to guess is based on contents
  return guessKind(path);
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
        out.whisker_path=r.whisker_path;
        break;
      case MEASUREMENTS:
        out.m =r.m;
        out.mn=r.mn;
        out.measurement_path=r.measurement_path;
        break;
      default: // UNKNOWN
        ;
    }
  }
  return out;
}

bool isOrientationCharSame(char a, char b)
{ switch(a)
  { case 'x':
    case 'h':
      return b=='x' || b=='h';
    case 'y':
    case 'v':
      return b=='y' || b=='v';
    default:
      return false;
  }
}

void savefunc(const result_t &r)
{ QByteArray wp = r.whisker_path.toLocal8Bit(),
             mp = r.measurement_path.toLocal8Bit();
  if(r.w)
    TRY(locked::Save_Whiskers(wp.data(),NULL,r.w,r.wn));
  if(r.m && r.m)
  { if( (r.face_x!=r.m[0].face_x)           // do we need to recompute measurements?
      ||(r.face_y!=r.m[0].face_y)
      ||(!isOrientationCharSame(r.orientation,r.m[0].face_axis))
      )
    { // order measurements table by whiskers      
      LOCK;
      TRY( r.wn==r.mn );
      locked::Whisker_Seg_Sort_By_Id(r.w,r.wn);
      locked::Sort_Measurements_Table_Segment_UID(r.m,r.mn);
      locked::Whisker_Segments_Update_Measurements( // make sure measures are up to date ~ O(ncurves)
          r.m,                                      // assumes whisker segments and measurements
          r.w, r.wn,                                // are in direct correspondance
          r.face_x,
          r.face_y,
          r.orientation);
    }
    TRY(locked::Measurements_Table_To_Filename(mp.data(),NULL,r.m,r.mn));
  }
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
  , ncurves_capacity_(0)
  , measurements_(0)
  , nmeasurements_(0)
  , nmeasurements_capacity_(0)
  , minIdent_(-1)
  , maxIdent_(-1)
  , faceDefaultOrient_(UNKNOWN_ORIENTATION)
  , face_param_dirty_(0)
  , watcher_(0)
{ watcher_ = new QFutureWatcher<result_t>(this);
  TRY(connect(watcher_,SIGNAL(finished()),this,SLOT(commit())));

  save_watcher_ = new QFutureWatcher<void>(this);
  TRY(connect(save_watcher_,SIGNAL(finished()),this,SIGNAL(measurementsSaved())));
  TRY(connect(save_watcher_,SIGNAL(finished()),this,SIGNAL(curvesSaved())));
  TRY(connect(save_watcher_,SIGNAL(finished()),this,SLOT(saveDone())));
  return;
Error:
  DIE;
}

Data::~Data()
{ saving_.waitForFinished();
  if(video_)        locked::video_close(&video_);
  if(curves_)       { locked::Free_Whisker_Seg_Vec(curves_,ncurves_); curves_ = NULL; }
  if(measurements_) { locked::Free_Measurements_Table(measurements_); measurements_ = NULL; }
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
{ lastVideoFile_ = path;
  watcher_->setFuture(QtConcurrent::run(maybeLoadAll,path));
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
  { LOCK;
    // Get rid of the old stuff
    { int dirty_whiskers = ((r.v!=NULL) || (r.w!=NULL)) && (curves_!=NULL);
      if(dirty_whiskers)
      { locked::Free_Whisker_Seg_Vec(curves_,ncurves_); // this takes a huge amount of time (in Debug on Win7)
        curves_=NULL;
        ncurves_=0;
        ncurves_capacity_=0;
      }
      if(measurements_)
      { locked::Free_Measurements_Table(measurements_);
        measurements_=NULL;
        nmeasurements_=0;
        nmeasurements_capacity_=0;
      }
    }
    // Load the new stuff
    if(r.m)
    { measurements_  = r.m;
      nmeasurements_ = r.mn;
      nmeasurements_capacity_ = nmeasurements_;
      lastMeasurementsFile_ = r.measurement_path;
      faceDefaultAnchor_ = QPointF(r.m[0].face_x,r.m[0].face_y);
      switch(r.m[0].face_axis)
      { case 'x':
        case 'h':
          faceDefaultOrient_ = HORIZONTAL;
          break;
        case 'y':
        case 'v':
          faceDefaultOrient_ = VERTICAL;
          break;
        default:
          faceDefaultOrient_ = UNKNOWN_ORIENTATION;
      }
      emit facePositionChanged(facePosition(NULL));
      emit faceOrientationChanged(faceOrientation());
    }
    if(r.w)
    { curves_  = r.w;
      ncurves_ = r.wn;
      ncurves_capacity_ = ncurves_;
      lastWhiskerFile_ = r.whisker_path;
    }
    if(r.v)
    { if(video_) locked::video_close(&video_);
      video_ = r.v;
    }
  } //end lock
  buildMeasurementsIndex_();
  buildCurveIndex_();
  emit loaded();
}

void Data::saveDone()
{ buildMeasurementsIndex_();
  buildCurveIndex_();
}

/**
 * Starts an asynchronous call to save current data.
 *
 * Use this for autosaves. It's executed in the background,
 * so the main thread isn't blocked.
 */
void Data::save()
{ result_t r(this);
  saving_=QtConcurrent::run(savefunc,r);
  save_watcher_->setFuture(saving_);
}

/** saves either the whiskers or the measurements file
 *  based on file extension.
 *
 *  If the file extension doesn't match, an attempt will
 *  be made to see if the file exists and if the file type
 *  can be guessed.
 *
 *  If the filetype is a video, both the whiskers and
 *  measurements data will be saved alongside the video.
 *  If whiskers or measurements, only the corresponding
 *  data will be saved.
 *
 *  \todo add dirty flags to indicate unsaved data
 *  \todo add status bar indicators for dirty data
 *
 *  If anything goes wrong, a warning dialog
 *  will get thrown up to indicate the save failed.
 */
void Data::saveAs(const QString &path)
{ KIND k = guessKindForSave(path);
  switch(k)
  { case WHISKERS:     saveWhiskersAs(path);     break;
    case MEASUREMENTS: saveMeasurementsAs(path); break;
    case VIDEO:
                       saveWhiskersAs(whiskersFile(path).absoluteFilePath());
                       saveMeasurementsAs(measurementsFile(path).absoluteFilePath());
                       break;
    default:
      QMessageBox mb;
      mb.setText(tr("Could not determine the file type for saving."ENDL
                    "Data was not saved."ENDL));
      mb.setIcon(QMessageBox::Warning);
      mb.exec();
  }
}

void Data::saveWhiskersAs(const QString &path)
{ if(curves_)
    TRY(locked::Save_Whiskers(
          path.toLocal8Bit().data(),
          NULL,curves_,ncurves_));
  emit curvesSaved();
  return;
Error:
  QMessageBox mb;
  mb.setText("Failed to save Whiskers file.");
  mb.setIcon(QMessageBox::Warning);
}

void Data::saveMeasurementsAs(const QString &path)
{
  if(measurements_)
    TRY(locked::Measurements_Table_To_Filename(
          path.toLocal8Bit().data(),
          NULL,measurements_,nmeasurements_));
  emit measurementsSaved();
Error:
  QMessageBox mb;
  mb.setText("Failed to save Measurements file.");
  mb.setIcon(QMessageBox::Warning);
}

void Data::remove(int iframe, int wid)
{ Whisker_Seg *w;
  Measurements *m;
  TRY(w=get_curve_by_wid_(iframe,wid));  // query for the Whisker_seg first
  saving_.waitForFinished();             // make sure the saving thread is done before changing anything
  { LOCK;
    locked::Free_Whisker_Seg_Data(w);    // Do the whiskers first
    memmove(w,w+1, ((curves_+ncurves_)-(w+1))*sizeof(Whisker_Seg));
    memset(curves_+ncurves_-1,0,sizeof(Whisker_Seg)); // setting to zero here is useful for catching bugs 
    --ncurves_;
    buildCurveIndex_();
    emit curvesDirtied();

    SILENTTRY(m=get_meas_by_wid_(iframe,wid));
    { Measurements t = *m;
      memmove(m,m+1, ((measurements_+nmeasurements_)-(m+1))*sizeof(Measurements));      
      measurements_[nmeasurements_-1] = t;
      measurements_[nmeasurements_-1].fid=-1;
      measurements_[nmeasurements_-1].wid=-1;
      measurements_[nmeasurements_-1].state=-1;
    }
    --nmeasurements_;
    buildMeasurementsIndex_();
    emit measurementsDirtied();
  }
  emit success();
Error:
  return;
}

void Data::setIdentity(int iframe, int wid, int ident)
{ Measurements *mm;
  measIdMap_t ms;
  if(ident==-1 && !measurements_)
    return;
  maybePopulateMeasurements();
  SILENTTRY(mm=get_meas_by_wid_(iframe,wid));
  ms = measIndex_.value(iframe);
  saving_.waitForFinished();
  { LOCK;
    foreach(Measurements* m,ms)
      if(m->state==ident)
        m->state=-1;
    mm->state=ident;

    minIdent_ = qMin(minIdent_,ident);
    maxIdent_ = qMax(maxIdent_,ident);
  }

  emit measurementsDirtied();
  emit success();
  emit lastCurve(curveByWid(iframe,wid));
Error:
  return;
}

void Data::buildCurveIndex_()
{ LOCK;
  curveIndex_.clear();
  if(curves_)
    for(Whisker_Seg *c=curves_;c<curves_+ncurves_;++c)
    { curveIdMap_t cs = curveIndex_[c->time];
      cs.insert(c->id,c);
      curveIndex_.insert(c->time,cs);
    }
}

void Data::buildMeasurementsIndex_()
{ LOCK;
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

bool Data::isFaceSet()
{ int is_unknown=1;
  facePosition(&is_unknown);
  Orientation o = faceOrientation();
  if(is_unknown || o==UNKNOWN_ORIENTATION)
    return false;
  return true;
}

bool Data::areFaceDefaultsSet()
{ int is_unknown=1;
  if( faceDefaultAnchor_.isNull() ) return false;
  if( faceDefaultOrient_==UNKNOWN_ORIENTATION) return false;
  return true;
}
QPointF Data::facePosition(int *is_unknown)
{ if(is_unknown) *is_unknown=1;
  if(!measurements_)
    return QPointF();
  if(is_unknown) *is_unknown=0;
  return QPointF(measurements_[0].face_x,measurements_[0].face_y);
}

void Data::setFacePosition(QPointF r)
{ faceDefaultAnchor_ = r;
  if(faceDefaultOrient_==UNKNOWN_ORIENTATION)
    faceDefaultOrient_=VERTICAL;
  face_param_dirty_=true;
  //emit facePositionChanged(r);  
}

void Data::maybeCommitFacePosition_()
{ if(!measurements_ && face_param_dirty_) return;
  TRY(areFaceDefaultsSet());
  { int  x = faceDefaultAnchor_.x(),
         y = faceDefaultAnchor_.y();
    char o = (faceDefaultOrient_==HORIZONTAL)?'x':'y';
    for(int i=0;i<nmeasurements_;++i)
    { measurements_[i].face_x = x;
      measurements_[i].face_y = y;
      measurements_[i].face_axis = o;
    }    
  }
  face_param_dirty_=0;
Error:
  return;
}

int Data::get_next_wid_(int iframe)
{ curveIdMap_t ws = curveIndex_.value(iframe);
  int max=-1;
  foreach(Whisker_Seg* w,ws)
    max=(max>w->id)?max:w->id;
  return max+1; // if nothing found returns 0
}

void Data::traceAt(int iframe, QPointF r, bool autocorrect)
{ // 1. sd=trace.compute_seed(a,p,d) ... a is the raw data bufffer, p is the point, and d is the diameter to check (8px)
  Seed *sd=0;
  Whisker_Seg *w=0;
  int offset,wid;
  Image *im=0;
  
  LOCK;
  TRY(im=locked::video_get(video_,iframe,autocorrect));
  offset=im->width*(int)r.y() + (int)r.x();
  TRY(sd=locked::compute_seed_from_point(im,offset,8));
  TRY( w=locked::trace_whisker(sd,im));
  locked::Free_Image(im);
  w->time = iframe;
  w->id   = wid = get_next_wid_(iframe);

  // append whisker
  if(!curves_)
  { curves_  = w;
    ncurves_ = 1;
    ncurves_capacity_ = ncurves_;
    lastWhiskerFile_=whiskersFile(lastVideoFile_).absoluteFilePath();
  } else
  { if(measurements_)
      TRY(maybeShowFaceAnchorRequiredDialog()); // before we go any further
    if( (++ncurves_)>ncurves_capacity_) // resize
    { ncurves_capacity_ = ncurves_capacity_*1.2 + 50;
      TRY(curves_=(Whisker_Seg*)realloc(curves_,sizeof(Whisker_Seg)*ncurves_capacity_));
      memset(curves_+ncurves_-1,0,sizeof(Whisker_Seg)*(ncurves_capacity_-ncurves_));
    }
    curves_[ncurves_-1] = *w;
    free(w);
    buildCurveIndex_();
  }
  { curveIdMap_t ws = curveIndex_.value(iframe);
    ws.insert(wid,curves_+ncurves_-1);
    curveIndex_.insert(iframe,ws);
  }
  emit curvesDirtied();

  // maybe append measurement
  if(measurements_)
  { int was_resized=0;
    maybeCommitFacePosition_();
    if((++nmeasurements_)>nmeasurements_capacity_) // resize
    { nmeasurements_capacity_ = nmeasurements_capacity_*1.2+50;
      TRY(measurements_=locked::Realloc_Measurements_Table(
            measurements_,
            nmeasurements_-1,
            nmeasurements_capacity_));
      was_resized=1;
    }
    TRY(nmeasurements_<=nmeasurements_capacity_);
    locked::Whisker_Seg_Measure(
        curves_+ncurves_-1,
        measurements_[nmeasurements_-1].data,
        measurements_->face_x,
        measurements_->face_y,
        measurements_->face_axis);
    measurements_[nmeasurements_-1].fid = iframe;
    measurements_[nmeasurements_-1].wid = wid;
    if(was_resized)
      buildMeasurementsIndex_();
    else
    { measIdMap_t ms = measIndex_.value(iframe);
      ms.insert(wid,measurements_+nmeasurements_-1);
      measIndex_.insert(iframe,ms);
    }
    emit measurementsDirtied();
  }
  emit success();
  emit lastCurve(curveByWid(iframe,wid));
  return;
Error:  
  if(im) locked::Free_Image(im);
  if(w && curves_==NULL)  locked::Free_Whisker_Seg(w); // curves_ realloc failed
  else if(w)              free(w);                     // something after curves_ realloc failed

  return;
}

void Data::traceAtAndIdentify(int iframe,QPointF target,bool autocorrect_video,int ident)
{ if(ident!=-1 && !measurements_)    
    TRY(maybePopulateMeasurements());
  { int oldcount = nmeasurements_;
    traceAt(iframe,target,autocorrect_video);     //don't return success failure because it's a slot
    { LOCK;
      if(oldcount!=nmeasurements_)                    //check to see if something got traced 
      { foreach(Measurements* m,measIndex_.value(iframe).values())  // make sure identity is unique in the frame
          if(m->state==ident)
            m->state=-1;
        
        measurements_[nmeasurements_-1].state = ident;//last traced is last row of measurements table
        minIdent_ = qMin(minIdent_,ident);
        maxIdent_ = qMax(maxIdent_,ident);
      }
    }
  }
Error:
  return;
}

static void ws_vec_free_rest(Whisker_Seg **wv, int nkeep, int ntotal)
{ if(!wv) return;
  if(!*wv) return;
  for(int i=nkeep;i<ntotal;++i)
    locked::Free_Whisker_Seg_Data(*wv+i);
  free(*wv);
  *wv=NULL;
}

void Data::traceFrame(int iframe, bool autocorrect)
{ Whisker_Seg *ws=0;
  Image *im=0;
  int n,k=0;

  LOCK;
  TRY(im=locked::video_get(video_,iframe,autocorrect));  
  TRY(ws=locked::find_segments(iframe,im,NULL,&k));
  n = locked::Remove_Overlapping_Whiskers_One_Frame( ws, k, 
                                            im->width, im->height, 
                                            2.0,    // scale down by this
                                            2.0,    // distance threshold
                                            0.5 );  // significant overlap fraction
  locked::Free_Image(im);

  // append whiskers
  if(!curves_)
  { curves_  = ws;
    ncurves_ = n;
    ncurves_capacity_ = ncurves_;
    lastWhiskerFile_=whiskersFile(lastVideoFile_).absoluteFilePath();
    { curveIdMap_t wmap = curveIndex_.value(iframe);
      for(int i=0;i<n;++i)
        wmap.insert(ws[i].id,curves_+i);
      curveIndex_.insert(iframe,wmap);
    }
  } else
  { int o;
    //first need to remove curves from current frame
    QList<int> wids;
    foreach(Whisker_Seg *wc,curveIndex_.value(iframe).values())  /// \todo FIXME this is retardedly slow - should write something to remove them all at onces
      wids.append(wc->id);
    foreach(int wid,wids)
      remove(iframe,wid);
    //realloc and insert
    o = ncurves_;
    ncurves_+=n;
    if( ncurves_>ncurves_capacity_) // resize
    { ncurves_capacity_ = ncurves_*1.2 + 50;
      TRY(curves_=(Whisker_Seg*)realloc(curves_,sizeof(Whisker_Seg)*ncurves_capacity_));
      memset(curves_+o,0,sizeof(Whisker_Seg)*(ncurves_capacity_-o));
    }
    memcpy(curves_+o,ws,n*sizeof(Whisker_Seg));    
    ws_vec_free_rest(&ws,n,k);  
    buildCurveIndex_();
  }
  emit curvesDirtied();

  // update measurements
  if(measurements_)
  { int was_resized=0;
    int o = nmeasurements_;
    nmeasurements_ += n;
    if(nmeasurements_>nmeasurements_capacity_) // resize
    { nmeasurements_capacity_ = nmeasurements_*1.2+50;
      TRY(measurements_=locked::Realloc_Measurements_Table(
            measurements_,
            o,
            nmeasurements_capacity_));
      was_resized=1;
    }
    TRY(nmeasurements_<=nmeasurements_capacity_); // double checking in case of weird stuff
    for(int i=0;i<n;++i)
    {
      locked::Whisker_Seg_Measure(
          curves_+o+i,
          measurements_[o+i].data,
          measurements_->face_x,
          measurements_->face_y,
          measurements_->face_axis);
      measurements_[o+i].fid = curves_[o+i].time;
      measurements_[o+i].wid = curves_[o+i].id;      
    }
    buildMeasurementsIndex_();
    emit measurementsDirtied();
  }
  emit success();
  //emit lastCurve(curveByWid(iframe,wid));

  return;
Error: 
  if(im) locked::Free_Image(im);
  if(ws && curves_==NULL) locked::Free_Whisker_Seg(ws); // curves_ realloc failed
  else if(ws)             ws_vec_free_rest(&ws,n,k);    // something after curves_ realloc failed
  return;
}

Data::Orientation Data::faceOrientation()
{ if(measurements_)
    switch(measurements_[0].face_axis)
    { case 'x':
      case 'h':
        return HORIZONTAL;
      case 'y':
      case 'v':
        return VERTICAL;
      default:
        ;
    }
  return UNKNOWN_ORIENTATION;
}

void Data::setFaceOrientation(Data::Orientation o)
{ char s;
  switch(o)
  { case HORIZONTAL: s='x'; break;
    case VERTICAL:   s='y'; break;
    default:
      ;
  }
  face_param_dirty_=true;
  //emit faceOrientationChanged(o);
}

int Data::nextMissingFrame(int iframe, int ident)
{ 
  if(ident==-1)
    return iframe;
  for(int i=iframe+1;i<frameCount();++i)
  { int any=0;
    foreach(Measurements *m,measIndex_.value(i).values())
    { if(m->state==ident)
      { any=1;
        break;
      }
    }
    if(!any)
      return i;
  }
  return frameCount()-1;
}

int Data::prevMissingFrame(int iframe, int ident)
{ 
  if(ident==-1)
    return iframe;
  for(int i=iframe-1;i>=0;--i)
  { int any=0;
    foreach(Measurements *m,measIndex_.value(i).values())
    { if(m->state==ident)
      { any=1;
        break;
      }
    }
    if(!any)
      return i;
  }  
  return 0;
}

const QPixmap Data::frame(int iframe, bool autocorrect)
{ Image *im;
  TRY(video_);
  SILENTTRY(im=locked::video_get(video_,iframe,autocorrect));
  { //QImage qim(im->array,im->width,im->height,im->width,QImage::Format_Indexed8);
    QImage qim(im->width,im->height,QImage::Format_Indexed8);
    QVector<QRgb> grayscale;
    for(int i=0;i<256;++i)
      grayscale.append(qRgb(i,i,i));
    qim.setColorTable(grayscale);

    // copy in the data
    qim.fill(0);
    for(int i=0;i<im->height;++i)
      memcpy(qim.scanLine(i),im->array+im->width*i,im->width);

    QPixmap out = QPixmap::fromImage(qim);
    locked::Free_Image(im);
    return out;
  }
Error:
  return QPixmap();
}

int Data::frameCount()
{ if(video_)
    return locked::video_frame_count(video_);
  else
    return 0;
}

int Data::wid(int iframe, int icurve)
{ Whisker_Seg* w = get_curve_(iframe,icurve);
  return w->id;
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

Whisker_Seg* Data::get_curve_by_wid_(int iframe, int wid)
{ Whisker_Seg *w = 0;
  QList<Whisker_Seg*> cs = curveIndex_.value(iframe,curveIdMap_t()).values();
  foreach(Whisker_Seg *c,cs)
    if(c->id==wid)
      w=c;
  return w;
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

Measurements* Data::get_meas_by_wid_(int iframe, int wid)
{ measIdMap_t ms = measIndex_.value(iframe);
  return  ms.value(wid,NULL);
}

bool Data::is_ident_same_(int iframe, int wid, int ident)
{ measIdMap_t ms = measIndex_.value(iframe);
  Measurements *row; 
  if(row=ms.value(wid,NULL))
    return row->state == ident;
}

int Data::maybeShowFaceAnchorRequiredDialog()
{
  if(!areFaceDefaultsSet())
  { QMessageBox mb;
    mb.setText("Could not make curve measurements.");
    mb.setInformativeText("Need to set the face position and orientation first. "
                          "Right click on the image and select \"Place face anchor.\"");
    mb.setIcon(QMessageBox::Information);
    mb.exec();
    return 0;
  }
  return 1;
}

int Data::maybePopulateMeasurements()
{ if(!curves_)      return 0;
  if(measurements_) return 1;
  TRY(maybeShowFaceAnchorRequiredDialog());
  { LOCK;
    TRY(measurements_=locked::Whisker_Segments_Measure(
          curves_,ncurves_,
          faceDefaultAnchor_.x(),
          faceDefaultAnchor_.y(),
          faceDefaultOrient_?'y':'x'));
    nmeasurements_=ncurves_;
    for(int i=0;i<nmeasurements_;++i)// set initial identities to unknown
      measurements_[i].state=-1;
  }
  buildMeasurementsIndex_();
  lastMeasurementsFile_ = measurementsFile(lastWhiskerFile_).absoluteFilePath();
  return 1;
Error:
  return 0;
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

QPolygonF Data::curveByWid(int iframe, int wid)
{ QPolygonF c;
  Whisker_Seg *w=0;
  TRY(w=get_curve_by_wid_(iframe,wid));
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
  SILENTTRY(row= measIndex_.value(iframe,measIdMap_t())
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
