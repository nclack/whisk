#pragma warning(disable : 4996)
#pragma warning(disable : 4244) //type conversion

#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
 
SHARED_EXPORT
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

SHARED_EXPORT
void warning(char *str, ... )
{
  va_list argList;
  va_start( argList, str );
  fprintf(stderr, "--- Warning: ");
  vfprintf(stderr, str, argList);
  va_end( argList );
  fflush(NULL);
}

SHARED_EXPORT
void debug(char *str, ... )
{
  va_list argList;
  va_start( argList, str );
  if( SHOW_DEBUG_MESSAGES )
    vfprintf(stderr, str, argList);
  va_end( argList );
  fflush(NULL);
}

SHARED_EXPORT
void help(int show, char *str, ... )
{ if(show)
  { va_list argList;
    va_start( argList, str );
    vfprintf(stderr, str, argList);
    va_end( argList );
    fflush(NULL);
	exit(0);
  }
}

SHARED_EXPORT
void progress(char *str, ... )
{ va_list argList;
  va_start( argList, str );
  if( SHOW_PROGRESS_MESSAGES )
    vfprintf( stderr, str, argList);
  va_end( argList );
  fflush(NULL);
}

SHARED_EXPORT
void progress_meter(double cur, double min, double max, int len, char *str, ...)
{ if( SHOW_PROGRESS_MESSAGES )
  { va_list argList;
    char buf[1024];
    int n=0;

    {
      va_start( argList, str );
      n = sprintf(buf,"\r");
      n += vsprintf(buf+n, str, argList);
      va_end( argList );
    }


    n += sprintf(buf+n,"[");
    len-=(n-1);
    { 
      int nc = (len)*(cur-min)/(max-min);
      len -= (nc+1);
      while(nc--  > 0) 
        n+=sprintf(buf+n,"|");
      while(len-- > 0)
        n+=sprintf(buf+n,"-");
    }
    n+=sprintf(buf+n,"]\r");
    buf[n] = '\0';

    fprintf(stderr,buf);
    fflush(NULL);
  }
}
