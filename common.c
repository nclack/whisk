#include "utilities.h"

#if 0
#define DEBUG_REQUEST_STORAGE
#endif

void *request_storage( void *buffer, int *maxlen, int nbytes, int minindex, char *msg )
{ if( (nbytes*minindex) > *maxlen )
  {
#ifdef DEBUG_REQUEST_STORAGE
    printf("REQUEST %7d bytes above current %7d bytes by %s\n",minindex * nbytes,*maxlen,msg);
#endif
    *maxlen = (int) (1.2 * minindex * nbytes + 50);
    buffer = Guarded_Realloc( buffer, *maxlen, msg );
  }
  return buffer;
}
