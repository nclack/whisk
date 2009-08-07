#include "utilities.h"

#define DEBUG_REQUEST_STORAGE
#if 0
#endif

void *request_storage( void *buffer, int *maxlen, int nbytes, int minindex, char *msg )
{ if( (nbytes*minindex) > *maxlen )
  {
#ifdef DEBUG_REQUEST_STORAGE
    printf("REQUEST %7d bytes (%7d items) above current %7d bytes by %s\n",minindex * nbytes, minindex, *maxlen,msg);
#endif
    *maxlen = (int) (1.2 * minindex * nbytes + 50);
    buffer = Guarded_Realloc( buffer, *maxlen, msg );
  }
  return buffer;
}


void maxfilt_double( double *a, int len, int support )
{ int f = support/2;
  double mxmx, mx;
  int mxmx_n, mx_n;
  int i;
  
  assert(support<len);

  //init
  for(i=0;i<f;i++)
  { // XXX
    // HERE
    // TODO
  }
}
