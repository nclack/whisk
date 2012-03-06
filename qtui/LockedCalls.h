namespace whisk {
extern "C"{
#include <parameters/param.h>
#include "whisker_io.h"
#include "measurements_io.h"
  }
#include "video.h"
}

#define TRANSLATE_TYPE(e) typedef whisk::e e
TRANSLATE_TYPE(t_params);
TRANSLATE_TYPE(Whisker_Seg);
TRANSLATE_TYPE(Measurements);
TRANSLATE_TYPE(video_t);
TRANSLATE_TYPE(Image);

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

} // end namespace locked
