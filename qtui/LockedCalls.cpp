#include <QtCore>
#include "LockedCalls.h"

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

}// end namespace locked
