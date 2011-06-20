/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef H_MEASUREMENTS_IO_V2
#define H_MEASUREMENTS_IO_V2

#include <stdio.h>
#include "traj.h"

int            is_file_measurements_v2 ( const char* filename);
FILE*          open_measurements_v2    ( const char* filename, const char* mode);
void           close_measurements_v2   ( FILE* file);
void           write_measurements_v2   ( FILE* file, Measurements *table, int n);
Measurements*  read_measurements_v2    ( FILE* file, int *n);

#endif //H_MEASUREMENTS_IO_V1
