/* Author: Nathan Clack <clackn@janelia.hhmi.org> 
 *   Date: May 2009
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef H_WHISKER_IO_WHISKPOLY1
#define H_WHISKER_IO_WHISKPOLY1

#include <stdio.h>
#include "trace.h"

int            is_file_whiskpoly1         ( const char* filename);
FILE          *open_whiskpoly1            ( const char* filename, const char* mode);
void           close_whiskpoly1           ( FILE* fp);
void           append_segments_whiskpoly1 ( FILE *fp, Whisker_Seg *wv, int n );
Whisker_Seg   *read_segments_whiskpoly1   ( FILE *file, int *n);

#endif //H_WHISKER_IO_WHISKPOLY1
