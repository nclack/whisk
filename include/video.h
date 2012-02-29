#ifdef __cplusplus
extern "C" {
#endif

#include "image_lib.h"

typedef struct _video_t video_t;

video_t     *video_open          (const char *path);
void         video_close         (video_t **self);
unsigned int video_frame_count   (video_t  *self);
void         video_compute_stats (video_t  *self, int at_most_nframes);
Image       *video_get           (video_t  *self, unsigned int iframe, int apply_line_bias_correction);

int          is_video            (const char *path); 


#ifdef __cplusplus
} 
#endif
