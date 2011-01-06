/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef _H_COMMON
#define _H_COMMON

#include <stdio.h>
#include <stdlib.h>
#include "compat.h"
#include "config.h"

// #include <vld.h> // for VC++: Visual Leak Detector (v1.9b) 
                    // - comment out if this causes problems
                    // - comment in  to check for memory leaks in an
                    //   configured Visual Studio.

#include "parameters/param.h"

//evals expression px once, y up to 2 times
#ifndef bd
#define bd(TYPE,px,y) { TYPE *X = (px); *X = *X<(y) ? *X:(y);}
#endif
#ifndef bu
#define bu(TYPE,px,y) { TYPE *X = (px); *X = *X>(y) ? *X:(y);}
#endif

#ifndef MAX
#define MAX(a,b) ( (a<b)?b:a )
#endif
#ifndef MIN
#define MIN(a,b) ( (a<b)?a:b )
#endif
#ifndef SWAP
#define SWAP(a,b) (( (a)==(b) ) || ( ((a) ^= (b)), ((b) ^= (a)), ((a) ^=(b) ))  )
#endif

#define CLAMP_UINT8(v) ( (v>255) ? 255:( (v<0)? 0:v ) )
#define CLAMP(v,low,high) ((v)>(high) ? (high): ( (v)<(low)?(low):(v)))

#define WRAP_ANGLE_HALF_PLANE(th) \
  while( (th)    < M_PI/2.0 )     \
    (th)    +=   M_PI;            \
  while( (th)    >= M_PI/2.0 )    \
    (th)    -=   M_PI;
#define WRAP_ANGLE_2PI(th)        \
  while( (th)    < -M_PI )        \
    (th)    +=   2*M_PI;          \
  while( (th)    >= M_PI )        \
    (th)    -=   2*M_PI;

#define DEPRICATED error("Function at %s:%d was marked for removal.\n",__FILE__,__LINE__)

/************************************************************************
 * Example
 *
  {
    static char *deleted;
    static int maxlen = 0;

    deleted = (char*) request_storage( deleted, &maxlen, sizeof(char), *n, "remove duplicate segments" );
    ...
  }
*/
void *request_storage( 
    void   *buffer  , 
    size_t *maxlen  , 
    size_t  nbytes  , 
    size_t  minindex, 
    char   *msg );    

void *request_storage_zeroed( 
    void   *buffer  , 
    size_t *maxlen  , 
    size_t  nbytes  , 
    size_t  minindex, 
    char   *msg );

void *request_storage_pow2items( 
    void *buffer, 
    size_t *maxlen_bytes, 
    size_t nbytes, 
    size_t minindex, 
    char *msg );

/*
 * FILE IO
 */
int fskipline(FILE* fp, size_t *nch);

//for debugging
void dump_doubles(char* filename, double* a, int n);

void linspace_d( double low, double high, int n, double **resizable, size_t *size );

float threshold_two_means( uint8_t *array, size_t size );
void maxfilt_centered_double_inplace( double *a, int len, int support );
#endif //_H_COMMON

