/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef H_MEASUREMENTS_IO
#define H_MEASUREMENTS_IO

#include "traj.h"

typedef void* MeasurementsFile;

SHARED_EXPORT int              Measurements_File_Autodetect (const char* filename, char** format );
SHARED_EXPORT MeasurementsFile Measurements_File_Open       (const char* filename, char* format, const char* mode );
SHARED_EXPORT void             Measurements_File_Close      (MeasurementsFile fp);
SHARED_EXPORT void             Measurements_File_Write      (MeasurementsFile fp, Measurements *table, int n);
SHARED_EXPORT Measurements*    Measurements_File_Read       (MeasurementsFile fp, int *n);

SHARED_EXPORT Measurements*    Measurements_Table_From_Filename (const char *filename, char *format, int *n );
SHARED_EXPORT void             Measurements_Table_To_Filename   (const char *filename, char *format, Measurements *table, int n );

#endif //H_WHISKER_IO
