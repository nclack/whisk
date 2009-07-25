#ifndef H_WHISKER_IO_WHISKER1
#define H_WHISKER_IO_WHISKER1

#include <stdio.h>
#include "trace.h"

int            is_file_whisk1         ( const char* filename);
FILE          *open_whisk1            ( const char* filename, const char* mode);
void           close_whisk1           ( FILE* fp);
void           append_segments_whisk1 ( FILE *fp, Whisker_Seg *wv, int n );
Whisker_Seg   *read_segments_whisker1 ( FILE *file, int *n);

#endif //H_WHISKER_IO_WHISKER1
