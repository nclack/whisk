#ifndef H_WHISKER_IO
#define H_WHISKER_IO

#include "trace.h"

typedef void*          WhiskerFile;

int           Whisker_File_Autodetect      (const char * filename, char** format );
WhiskerFile   Whisker_File_Open            (const char* filename, char* format, const char* mode );
void          Whisker_File_Close           (WhiskerFile wf);
void          Whisker_File_Append_Segments (WhiskerFile wf, Whisker_Seg *w, int n);
void          Whisker_File_Write_Segments  (WhiskerFile wf, Whisker_Seg *w, int n);
Whisker_Seg*  Whisker_File_Read_Segments   (WhiskerFile wf, int *n);

Whisker_Seg  *Load_Whiskers                (const char *filename, char *format, int *n );
void          Save_Whiskers                (const char *filename, char *format, Whisker_Seg *w, int n );

#endif //H_WHISKER_IO
