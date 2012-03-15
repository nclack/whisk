#include <QtCore>
#include "LockedCalls.h"
namespace whisk {
extern "C"{
#include <parameters/param.h>
#include "whisker_io.h"
#include "measurements_io.h"
#include "measure.h"
#include "seed.h"
#include "merge.h"
#include "traj.h"
  }
#include "video.h"
}

namespace locked {
QMutex GlobalWhiskLock; // = NULL; //& GlobalWhiskLock_;

#define LOCK QMutexLocker locker(&GlobalWhiskLock)
#define WRAP(expr) {LOCK; return (whisk::expr);}
#define WRAPNR(expr) {LOCK; (whisk::expr);}

//  PARAMS  ////////////////////////////////////////////////////////////////////
t_params* Params()                     WRAP(Params())
int Load_Params_File(char *filename)   WRAP(Load_Params_File(filename))
int Print_Params_File(char *filename)  WRAP(Print_Params_File(filename))

//  WHISKER IO  ////////////////////////////////////////////////////////////////
int Whisker_File_Autodetect(const char * filename, char** format )
WRAP(Whisker_File_Autodetect(filename,format))

Whisker_Seg* Load_Whiskers(const char *filename, char *format, int *n )
WRAP(Load_Whiskers(filename,format,n))

int Save_Whiskers(const char *filename, char *format, Whisker_Seg *w, int n )
WRAP(Save_Whiskers(filename,format,w,n))

void Free_Whisker_Seg ( Whisker_Seg *w )
WRAPNR(Free_Whisker_Seg(w))

void Free_Whisker_Seg_Vec(Whisker_Seg *wv, int n )
WRAPNR(Free_Whisker_Seg_Vec(wv,n));

//  MEASUREMENTS IO  ///////////////////////////////////////////////////////////
int Measurements_File_Autodetect(const char* filename, char** format )
WRAP(Measurements_File_Autodetect(filename,format))

Measurements* Measurements_Table_From_Filename (const char *filename, char *format, int *n )
WRAP(Measurements_Table_From_Filename(filename,format,n))

int Measurements_Table_To_Filename   (const char *filename, char *format, Measurements *table, int n )
WRAP(Measurements_Table_To_Filename(filename,format,table,n))

void Free_Measurements_Table (Measurements *table )
WRAP(Free_Measurements_Table(table))

//  VIDEO  /////////////////////////////////////////////////////////////////////
video_t     *video_open          (const char *path) WRAP(video_open(path))
void         video_close         (video_t **self)   WRAPNR(video_close(self))
unsigned int video_frame_count   (video_t  *self)   WRAP(video_frame_count(self))
int          is_video            (const char *path) WRAP(is_video(path))

void         video_compute_stats (video_t  *self, int at_most_nframes)
WRAPNR(video_compute_stats(self,at_most_nframes))

Image       *video_get           (video_t  *self, unsigned int iframe, int apply_line_bias_correction)
WRAP(video_get(self,iframe,apply_line_bias_correction))

void   Free_Image(Image *image)
WRAPNR(Free_Image(image))

//  TRACE  /////////////////////////////////////////////////////////////////////
Whisker_Seg *find_segments( int iFrame, Image *image, Image *bg, int *pnseg )
WRAP(find_segments(iFrame,image,bg,pnseg))

Seed* compute_seed_from_point  ( Image *image, int p, int maxr )
WRAP(compute_seed_from_point(image,p,maxr))

Whisker_Seg* trace_whisker(Seed *s,Image *image)
WRAP(trace_whisker(s,image))

void Whisker_Seg_Sort_By_Id( Whisker_Seg *wv, int n )
WRAP(Whisker_Seg_Sort_By_Id(wv,n))

void Free_Whisker_Seg_Data    ( Whisker_Seg *w )
WRAPNR(Free_Whisker_Seg_Data(w))

int Remove_Overlapping_Whiskers_One_Frame( Whisker_Seg *wv,
                                           int wv_n,
                                           int w,
                                           int h,
                                           float scale,
                                           float dist_thresh,
                                           float overlap_thresh )
WRAP(Remove_Overlapping_Whiskers_One_Frame(wv,wv_n,w,h,scale,dist_thresh,overlap_thresh))

// TRAJ  ///////////////////////////////////////////////////////////////////////

void Sort_Measurements_Table_Segment_UID( Measurements *table, int nrows )
WRAP(Sort_Measurements_Table_Segment_UID(table,nrows))

void Whisker_Seg_Measure(Whisker_Seg *w,double *dest,int facex,int facey,char face_axis )
WRAPNR(Whisker_Seg_Measure(w,dest,facex,facey,face_axis))

Measurements *Whisker_Segments_Measure( 
    Whisker_Seg *wv, int wvn, 
    int facex, int facey, char face_axis )
WRAP(Whisker_Segments_Measure(wv,wvn,facex,facey,face_axis))

Measurements *Whisker_Segments_Update_Measurements(
    Measurements* table,Whisker_Seg *wv, int wvn,
    int facex, int facey, char face_axis )
WRAP(Whisker_Segments_Update_Measurements(table,wv,wvn,facex,facey,face_axis))

Measurements* Realloc_Measurements_Table( Measurements *old, int n_rows_old, int n_rows_new )
WRAP(Realloc_Measurements_Table(old,n_rows_old,n_rows_new))

}// end namespace locked
