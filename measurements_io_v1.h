#ifndef H_MEASUREMENTS_IO_V1
#define H_MEASUREMENTS_IO_V1

#include <stdio.h>
#include "traj.h"

int            is_file_measurements_v1 ( const char* filename);
FILE*          open_measurements_v1    ( const char* filename, const char* mode);
void           close_measurements_v1   ( FILE* file);
void           write_measurements_v1   ( FILE* file, Measurements *table, int n);
Measurements*  read_measurements_v1    ( FILE* file, int *n);

#endif //H_MEASUREMENTS_IO_V1
