/* 
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef H_WHISKER_IO
#define H_WHISKER_IO

#include "trace.h"

typedef void*          WhiskerFile;

SHARED_EXPORT int           Whisker_File_Autodetect      (const char * filename, char** format );
SHARED_EXPORT WhiskerFile   Whisker_File_Open            (const char* filename, char* format, const char* mode );
SHARED_EXPORT void          Whisker_File_Close           (WhiskerFile wf);
SHARED_EXPORT void          Whisker_File_Append_Segments (WhiskerFile wf, Whisker_Seg *w, int n);
SHARED_EXPORT void          Whisker_File_Write_Segments  (WhiskerFile wf, Whisker_Seg *w, int n);
SHARED_EXPORT Whisker_Seg*  Whisker_File_Read_Segments   (WhiskerFile wf, int *n);

SHARED_EXPORT Whisker_Seg  *Load_Whiskers                (const char *filename, char *format, int *n );
SHARED_EXPORT int           Save_Whiskers                (const char *filename, char *format, Whisker_Seg *w, int n );

#endif //H_WHISKER_IO
