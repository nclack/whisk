#ifndef H_MEASUREMENTS_IO_V0
#define H_MEASUREMENTS_IO_V0

#include <stdio.h>
#include "traj.h"

int            is_file_measurements_v0 ( const char* filename);
FILE*          open_measurements_v0    ( const char* filename, const char* mode);
void           close_measurements_v0   ( FILE* file);
void           write_measurements_v0   ( FILE* file, Measurements *table, int n);
Measurements*  read_measurements_v0    ( FILE* file, int *n);

#endif //H_MEASUREMENTS_IO_V0
