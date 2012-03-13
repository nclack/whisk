#pragma once
namespace whisk {
extern "C"{
#include "trace.h"
#include "traj.h"
#include "seed.h"
}
}
#define TRANSLATE_TYPE(e) typedef whisk::e e
#define TRANSLATE_ABSTRACT_TYPE(e) typedef struct _##e e
TRANSLATE_TYPE(t_params);
TRANSLATE_TYPE(Whisker_Seg);
TRANSLATE_TYPE(Measurements);
TRANSLATE_TYPE(Image);
TRANSLATE_TYPE(Seed);
TRANSLATE_ABSTRACT_TYPE(video_t);

namespace locked {

//  PARAMS  ////////////////////////////////////////////////////////////////////
t_params* Params();
int       Load_Params_File(char *filename);
int       Print_Params_File(char *filename);

//  WHISKER IO  ////////////////////////////////////////////////////////////////
int          Whisker_File_Autodetect      (const char * filename, char** format );
Whisker_Seg* Load_Whiskers                (const char *filename, char *format, int *n );
int          Save_Whiskers                (const char *filename, char *format, Whisker_Seg *w, int n );
void         Free_Whisker_Seg             (Whisker_Seg *w );
void         Free_Whisker_Seg_Vec         (Whisker_Seg *wv, int n );

//  MEASUREMENTS IO  ///////////////////////////////////////////////////////////
int           Measurements_File_Autodetect     (const char* filename, char** format );
Measurements* Measurements_Table_From_Filename (const char *filename, char *format, int *n );
int           Measurements_Table_To_Filename   (const char *filename, char *format, Measurements *table, int n );
void          Free_Measurements_Table          (Measurements *table );

//  VIDEO  /////////////////////////////////////////////////////////////////////
video_t     *video_open          (const char *path);
void         video_close         (video_t **self);
unsigned int video_frame_count   (video_t  *self);
void         video_compute_stats (video_t  *self, int at_most_nframes);
Image       *video_get           (video_t  *self, unsigned int iframe, int apply_line_bias_correction);
int          is_video            (const char *path);

void   Free_Image(Image *image);
//  TRACE  /////////////////////////////////////////////////////////////////////

Seed*        compute_seed_from_point  ( Image *image, int p, int maxr );
Whisker_Seg* trace_whisker            ( Seed *s, Image *image);
void         Whisker_Seg_Sort_By_Id   ( Whisker_Seg *wv, int n );
void         Free_Whisker_Seg_Data    ( Whisker_Seg *w );

// TRAJ  ///////////////////////////////////////////////////////////////////////
void Sort_Measurements_Table_Segment_UID( Measurements *table, int nrows );
void Whisker_Seg_Measure( Whisker_Seg *w, double *dest, int facex, int facey, char face_axis );
Measurements *Whisker_Segments_Measure( 
    Whisker_Seg *wv, int wvn, 
    int facex, int facey, char face_axis );
Measurements *Whisker_Segments_Update_Measurements(
    Measurements* table,
    Whisker_Seg *wv, int wvn,
    int facex, int facey, char face_axis );
Measurements  *Realloc_Measurements_Table( Measurements *old, int n_rows_old, int n_rows_new );
} // end namespace locked
