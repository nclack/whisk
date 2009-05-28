#include "error.h"
#include <stdarg.h>
#include <stdio.h>

void error(char *str, ... )
{
  va_list argList;
  va_start( argList, str );
  fprintf(stderr, "*** ERROR: ");
  vfprintf(stderr, str, argList);
  va_end( argList );
  fflush(NULL);
}

void debug(char *str, ... )
{
  va_list argList;
  va_start( argList, str );
  if( SHOW_DEBUG_MESSAGES )
    vfprintf(stderr, str, argList);
  va_end( argList );
  fflush(NULL);
}

void progress(char *str, ... )
{ va_list argList;
  va_start( argList, str );
  if( SHOW_PROGRESS_MESSAGES )
    vfprintf( stderr, str, argList);
  va_end( argList );
  fflush(NULL);
}
