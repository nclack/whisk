#ifndef H_WHISKER_IO_WHISK_OLD
#define H_WHISKER_IO_WHISK_OLD

#include <stdio.h>
#include "trace.h"

int            is_file_whisk_old         ( const char* filename);
FILE          *open_whisk_old            ( const char* filename, const char* mode);
void           close_whisk_old           ( FILE* fp);
void           append_segments_whisk_old ( FILE *fp, Whisker_Seg *wv, int n );
Whisker_Seg   *read_segments_whisker_old ( FILE *file, int *n);

#endif //H_WHISKER_IO_WHISK_OLD
