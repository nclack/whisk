/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : May 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)

#include "utilities.h"
#include <assert.h>
#include <string.h>
#include "compat.h"

#if 0
#define DEBUG_REQUEST_STORAGE
#endif

//
// MEMORY
// ------
//

//for debugging
void dump_doubles(char* filename, double* a, int n)
{ FILE *fp = fopen(filename,"wb");
  fwrite(a, sizeof(double), n, fp);
  fclose(fp);
}

void *request_storage( void *buffer, size_t *maxlen, size_t nbytes, size_t minindex, char *msg )
{ if( (nbytes*minindex) > *maxlen )
  { size_t newsize = (size_t) (1.25 * minindex + 64) * nbytes;
#ifdef DEBUG_REQUEST_STORAGE
    printf("REQUEST %7d bytes (%7d items) above current %7d bytes by %s\n",minindex * nbytes, minindex, *maxlen,msg);
#endif
    buffer = Guarded_Realloc( buffer, newsize, msg );
    *maxlen = newsize; 
  }
  return buffer;
}

void *request_storage_zeroed( void *buffer, size_t *maxlen, size_t nbytes, size_t minindex, char *msg )
{ if( (nbytes*minindex) > *maxlen )
  { size_t newsize = (size_t) (1.25 * minindex + 64) * nbytes;
#ifdef DEBUG_REQUEST_STORAGE
    printf("REQUEST %7d bytes (%7d items) above current %7d bytes by %s\n",minindex * nbytes, minindex, *maxlen,msg);
#endif
    buffer = Guarded_Realloc( buffer, newsize, msg );
    memset((char*)buffer+*maxlen,0,newsize-*maxlen);
    *maxlen = newsize; 
  }
  return buffer;
}

 size_t _next_pow2_uint32(uint32_t v)
{ v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

 size_t _next_pow2_uint64(uint64_t v)
{ v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  v++;
  return v;
}

// TODO - returns buffers where maxlen_items is a power of 2 
void *request_storage_pow2items( void *buffer, size_t *maxlen_bytes, size_t nbytes, size_t minindex, char *msg )
{ if( (nbytes*minindex) > *maxlen_bytes )
  {
#ifdef DEBUG_REQUEST_STORAGE
    printf("REQUEST %7d bytes (%7d items) above current %7d bytes by %s\n",minindex * nbytes, minindex, *maxlen_bytes,msg);
    assert(sizeof(size_t)==4);
#endif
    *maxlen_bytes = (size_t) ( _next_pow2_uint32(minindex) ) * nbytes;
    buffer = Guarded_Realloc( buffer, *maxlen_bytes, msg );
  }
  return buffer;
}

//
// FILE IO
// -------
//

int fskipline(FILE* fp, size_t *nch)
{ //return fgetln(fp,nch)!=NULL; // not available on windows :(
  size_t i = 0;
  int c;
  do { c = fgetc(fp); i++; } while( c!=EOF && c!='\n' );
  *nch = i;
  return (c=='\n');
}

//
// NUMERICAL
// ---------
//

/**
 * Threshold estimation using k-means for k=2 on 8 bit images.
 */

float threshold_two_means( uint8_t *array, size_t size )
{ size_t hist[ 256 ],i;
  uint8_t *cur = array + size;
  float num = 0.0, 
        dom = 0.0,
        thresh,last,
        c[2] = {0.0,0.0};
  memset(hist,0,256*sizeof(size_t));
  while(cur-- > array) ++hist[ *cur ];
  for(i=0;i<256;i++)
  { float v = hist[i];
    num += i*v;
    dom += v;
  }
  thresh = num/dom;    // the mean - compute this way bc we need to compute the hist anyway
  
  // update means
  do
  { last = thresh;
    num = dom = 0.0;
    for(i=0;i<thresh;i++)
    { float v = hist[i];
      num += i*v;
      dom += v;
    }
    c[0] = num/dom;
    num = dom = 0.0;  
    for(;i<256;i++)
    { float v = hist[i];
      num += i*v;
      dom += v;
    }
    c[1] = num/dom;
    thresh = (c[1]+c[0])/2.0;
  } while( fabs(last - thresh) > 0.5 );
  //debug("Threshold: %f\n",thresh);
  return thresh;
}

/**
 * Resizes and fills <resizable> with <n> points from <low> to <high>.
 * <low> and <high> are inclusive bounds.
 *
 * Note the location of resizable may change.
 */
void linspace_d( double low, double high, int n, double **resizable, size_t *size )
{ double step = (high-low)/(n-1.0);
  int i;
  double *a;
  *resizable = request_storage(*resizable, size, sizeof(double), n, "linspace_d");
  a = *resizable;
  for(i=0;i<n;i++)
    a[i] = step*i + low;
}

/**
 * Based on:
 * Daniel Lemire, "Streaming Maximum-minimum filter using no more than
 * three comparisons per element', Nordic Journal of Computing, 13(4) pp. 328-339, 2006.
 *
 * Right now the modulus division for indexing the queue ring buffers is by far
 * the most expensive thing. (~97% of the time for arrays with millions of
 * elements).
 *
 * One way around this is to reset the ring buffer indices when the queue is
 * emptied.  Since the size of the queue is bounded by the support size, 
 * this might eliminate the need for a ring buffer.
 *
 */

#define pushf(x)          ( x[++i##x##f & max##x] )
#define popf(x)           ( x[i##x##f-- & max##x] )
#define popf_noassign(x)  ( i##x##f--)
#define peekf(x)          ( x[  i##x##f & max##x] )
#define pushb(x)          ( x[--i##x##b & max##x] )
#define popb(x)           ( x[i##x##b++ & max##x] )
#define popb_noassign(x)  ( i##x##b++)
#define peekb(x)          ( x[  i##x##b & max##x] )
#define isempty(x)        ( i##x##f+1 == i##x##b  )

void maxfilt_centered_double_inplace( double *a, int len, int support )
{ static double **U = NULL;
  static size_t maxUbytes = 0;
  size_t maxU,maxR;
  unsigned int    iUf,iUb,iRf,iRb;
  static double *R = NULL;
  static size_t maxRbytes = 0;
  double *e;
  int off = support;
  int c = support/2+1;
  double this,last;
  assert(support > 2);

  U = request_storage_pow2items(U,&maxUbytes,sizeof(double*), 2*support , "maxfilt_centered_double_inplace");
  maxU = maxUbytes / sizeof(double*) - 1;
  iUf = maxU/2-1;
  iUb = maxU/2;
  R = request_storage_pow2items(R,&maxRbytes,sizeof(double), 2*(off-c), "maxfilt_centered_double_inplace");
  maxR = maxRbytes / sizeof(double) - 1;
  iRf=-1;
  iRb=0;

  //The middle (support entirely in interval)
  last = *a;
  for( e=a+1; e < a+len; e++ )
  { this = *e;
    if( e-a >= c )
      pushf(R) = isempty(U) ? last : *peekf(U);
    if( e-a >= off  )                          
      e[-off] = popb(R);
    //progress("iRf:%5d\tiUf:%5d\tiUb:%5d\tsz:%5d\n",iRf,iUf,iUb,iUf-iUb+1);
    if( this > last )  
    { while( !isempty(U) ) 
      { if( this <= *peekb(U) )
        { if( e-off == peekf(U) )
            popf_noassign(U);   
          break;    
        }           
        if( !isempty(U) ) 
          popb_noassign(U);
      }                                         
    } else                                      
    { pushb(U)  = e-1;                          
      if( (e-off) == peekf(U) )                 
        popf_noassign(U);
    }
    last = this;
  }                      
  //The end              
  for( ; e <= a+len+c; e++ )    
  { pushf(R) = isempty(U) ? e[-1] : *peekf(U);
    e[-off] = popb(R);
    
    pushb(U)  = e-1;                          
    if( (e-off) == peekf(U) )                 
      popf_noassign(U);
  }
}

#ifdef TEST_MAX_FILT_1
double testdata[] = {-1.0, 2.0,-3.0, 4.0, 
                      3.0, 2.0, 1.0, 0.0,
                      1.0, 2.0, 3.0, 4.0,
                      3.0, 2.0, 1.0, 4.0};
//double expected[] = { 2.0, 4.0, 4.0, 4.0, 
//                      3.0, 2.0, 1.0, 2.0,
//                      3.0, 4.0, 4.0, 4.0,
//                      3.0, 4.0, 4.0, 4.0};
double expected[] = { 2.0, 2.0, 4.0, 4.0,  //window 3
                      4.0, 3.0, 2.0, 1.0,
                      2.0, 3.0, 4.0, 4.0,
                      4.0, 3.0, 4.0, 4.0};
//double expected[] = { 2.0, 4.0, 4.0, 4.0,  //window 5
//                      4.0, 4.0, 3.0, 2.0,
//                      3.0, 4.0, 4.0, 4.0,
//                      4.0, 4.0, 4.0, 4.0};
#endif
#ifdef TEST_MAX_FILT_2
double testdata[] = { 1.0, 0.0, 0.0, 0.0, 
                      0.0, 0.0, 1.0, 0.0,
                      0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 1.0};
//double expected[] = { 1.0, 0.0, 0.0, 0.0, 
//                      1.0, 1.0, 1.0, 0.0,
//                      0.0, 0.0, 0.0, 0.0,
//                      0.0, 1.0, 1.0, 1.0};
double expected[] = { 1.0, 1.0, 0.0, 0.0,  //window 3
                      0.0, 1.0, 1.0, 1.0,
                      0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 1.0, 1.0};
//double expected[] = { 1.0, 1.0, 0.0, 0.0,  //window 4
//                      1.0, 1.0, 1.0, 1.0,
//                      0.0, 0.0, 0.0, 0.0,
//                      0.0, 1.0, 1.0, 1.0};
//double expected[] = { 1.0, 1.0, 1.0, 0.0,    // window 5
//                      1.0, 1.0, 1.0, 1.0,    
//                      1.0, 0.0, 0.0, 0.0,
//                      0.0, 1.0, 1.0, 1.0};
#endif
                    

#if defined(TEST_MAX_FILT_1) || defined(TEST_MAX_FILT_2)
int main(int argc, char* argv[])
{ int i,j;
  double *e = expected;
  { for(i=0;i<16;i++)
      progress("%2d %f\n",i,testdata[i]);
    maxfilt_centered_double_inplace( testdata, 16, 3 );
    progress("\n"
             " #  R   E  =\n"
             "-- --- --- -\n"
        );
    for(i=0;i<16;i++)
      progress("%2d %2.1f %2.1f %d\n",i,testdata[i],e[i],testdata[i]==e[i]);
  }
  return 0;
}
#endif

#ifdef TEST_MAX_FILT_3
#include <math.h>
#include <time.h>
#define N (1000000)
int main(int argc, char* argv[])
{ int i = N;
  double k = 2*M_PI/N;
  double *t;
  double *y;
  int j = 100;
  clock_t clock0;
  double t0,t1;

  t = Guarded_Malloc(sizeof(double)*N,"t");
  y = Guarded_Malloc(sizeof(double)*N,"y");

  progress(" w  t(ms) type\n"
           "--- ----- ----\n");
  while((j-=3)>3)
  { 
    i=N;
    while(i--)
    { t[i] = i;
      y[i] = sin(k*i*4.0);
    }
    clock0 = clock();
    maxfilt_centered_double_inplace( y, N,  j  );
    t0 = (double)(clock()-clock0)/((double)(CLOCKS_PER_SEC));
    progress("%3d %4.1f  sin\n",j, 1e3*t0 );
    clock0 = clock();
    maxfilt_centered_double_inplace( t, N,  j  );
    t1 = (double)(clock()-clock0)/((double)(CLOCKS_PER_SEC));
    progress("%3d %2.1f  ramp\n",j, 1e3*t1 );
  }                             
  progress("\n"
           "NOTE\n"
           "----\n"
           "Timings should be constant with respect to window size (w).\n"
           "Each time is for an array of %g elements.\n"
           "Based on the last times...\n"
           "\tThroughput for the sin wave: %4.1f MSamples/s\n"
           "\tThroughput for the ramp    : %4.1f MSamples/s\n",(double)N, 1e-6*((double)N)/t0, 1e-6*((double)N)/t1 );
  free(t);
  free(y);
  return 0;
}
#endif
