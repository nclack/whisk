#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void error(char *str, ... )
{
  va_list argList;
  va_start( argList, str );
  fprintf(stderr, "*** ERROR: ");
  vfprintf(stderr, str, argList);
  va_end( argList );
  fflush(NULL);
  exit(-1);
}

void warning(char *str, ... )
{
  va_list argList;
  va_start( argList, str );
  fprintf(stderr, "--- Warning: ");
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

void progress_meter(double cur, double min, double max, int len, char *str, ...)
{ va_list argList;
  char buf[1024];
  int n=0;

  va_start( argList, str );
  if( SHOW_PROGRESS_MESSAGES )
  { n = sprintf(buf,"\r");
    n += vsprintf(buf+n, str, argList);
  }
  va_end( argList );

  if( SHOW_PROGRESS_MESSAGES )
  { n += sprintf(buf+n,"[");
    { int nc = len*(cur-min)/(max-min);
      len -= nc;
      while(nc--) 
        n+=sprintf(buf+n,"|");
      while(len--)
        n+=sprintf(buf+n,"-");
    }
    n+=sprintf(buf+n,"]\r");
    buf[n] = '\0';
    
    fprintf(stderr,buf);
    fflush(NULL);
  }
}
