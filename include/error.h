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

#include <config.h>
#include <parameters/param.h>
#include <compat.h>
#include <stdarg.h>

typedef void (*reporter)(char *str,va_list argList);
void set_reporter(reporter f);

void error(char *str, ... );
void warning(char *str, ... );
void help(int show, char *str, ... );
void debug(char *str, ... );
void progress(char *str, ... );
void progress_meter(double cur, double min, double max, int len, char *str, ...);

#endif//_H_ERROR_REPORT
