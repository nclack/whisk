/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef _H_ERROR_REPORT
#define _H_ERROR_REPORT

//#define SHOW_DEBUG_MESSAGES     1
//#define SHOW_PROGRESS_MESSAGES  1

#include <parameters/param.h>
#include <compat.h>

SHARED_EXPORT void error(char *str, ... );
SHARED_EXPORT void warning(char *str, ... );
SHARED_EXPORT void help(int show, char *str, ... );
SHARED_EXPORT void debug(char *str, ... );
SHARED_EXPORT void progress(char *str, ... );
SHARED_EXPORT void progress_meter(double cur, double min, double max, int len, char *str, ...);

#endif//_H_ERROR_REPORT
