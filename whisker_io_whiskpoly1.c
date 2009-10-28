#include <stdio.h>
#include <assert.h>
#include "poly.h"
#include "error.h"
#include "trace.h"
#include "common.h"
#include "utilities.h"

// WARNING
// Adjusting these parameters could cause
// files to become unreadable!
//
// (these parameters are not recorded in the file).
#define WHISKER_IO_POLY_DEGREE       3
#define WHISKER_IO_POLY_END_PADDING  0

#define DEBUG_WHISKER_IO_POLYFIT_ERROR
#if 0
#define DEBUG_WHISKER_IO_POLYFIT_READ
#endif

static const char type[] = "bwhiskpoly1\0"; 

void write_whiskpoly1_header( FILE *file )
{ fwrite( type, sizeof(type), 1, file );
}

void read_whiskpoly1_header( FILE *file ) 
{ fseek( file, sizeof(type), SEEK_SET ); // just seek to end of header
}

void write_whiskpoly1_footer( FILE *file, int nwhiskers )
{ fwrite( &nwhiskers, sizeof(int), 1, file ); // append number of whiskers
  fseek( file, -sizeof(int), SEEK_CUR );
}

int is_file_whiskpoly1( const char *filename )
{ char buf[33];
  FILE *file = fopen(filename,"rb");
  long pos = ftell(file);

  if(file==NULL)
    error("Could not open file (%s) for reading.\n",filename);
  
  fread(buf, sizeof(type), 1, file);
  fclose(file);
  if( strncmp( buf, type, sizeof(type) )==0 ) // if this is a whiskpoly1 file, read position is not reset
    return 1;
  return 0;
}

FILE* open_whiskpoly1( const char* filename, const char* mode )
{ FILE *fp;
  if( strncmp(mode,"w",1)==0 )
  { fp = fopen(filename,"w+b");
    if( fp == NULL )
    { error("Could not open file (%s) for writing.\n");
      goto Err;
    }
    write_whiskpoly1_header(fp);
    write_whiskpoly1_footer(fp,0);
  } else if( strncmp(mode,"r",1)==0 )
  { fp = fopen(filename,"rb");
    read_whiskpoly1_header(fp);
  } else {
    error("Could not recognize mode (%s) for file (%s).\n",mode,filename);
    goto Err;
  }
  return fp;
Err:
  return NULL;
}

void close_whiskpoly1( FILE *fp )
{ fclose(fp);
}

int peek_whiskpoly1_footer( FILE *file )
{ int nwhiskers;
  long pos = ftell(file);
  fseek( file, -sizeof(int), SEEK_END);
  fread( &nwhiskers, sizeof(int), 1, file );
  fseek( file, pos, SEEK_SET);
  return nwhiskers;
}

static int _score_cmp( const void *a, const void *b )
{ 
  float d = *(float*)a - *(float*)b;
  if( d < 0 )
  { if( d > -1e-6 )
      return 0;
    return -1;
  } else if( d < 1e-6 )
    return 0;
  return 1;
}

void write_whiskpoly1_segment( FILE *file, Whisker_Seg *w )
{ typedef struct {int id; int time; int len;} trunc_WSeg;
  float path_length,     //               
        median_score;    //
  float *x = w->x,
        *y = w->y,
        *s = w->scores;
  int len = w->len;
  static double *workspace = NULL;
  double px[  WHISKER_IO_POLY_DEGREE+1 ],
         py[  WHISKER_IO_POLY_DEGREE+1 ];
  
  polyfit_realloc_workspace( len, WHISKER_IO_POLY_DEGREE, &workspace );

  // compute polynomial fit
  // ----------------------
  { static double *cumlen = NULL;
    static size_t  cumlen_size = 0;

    cumlen = request_storage( cumlen, &cumlen_size, sizeof(double), len, "measure: cumlen");
    cumlen[0] = 0.0;

    // path length
    { float *ax = x + 1,       *ay = y + 1,
            *bx = x,           *by = y;
      double *cl = cumlen + 1, *clm = cumlen;
      while( ax < x + len )
        *cl++ = (*clm++) + hypotf( (*ax++) - (*bx++), (*ay++) - (*by++) );
      path_length = cl[-1];
    }
    // Fit
    // ---
    { static double *t = NULL;
      static size_t  t_size = 0;
      static double *xd = NULL;
      static size_t  xd_size = 0;
      static double *yd = NULL;
      static size_t  yd_size = 0;
      int i;
      const int pad = MIN( WHISKER_IO_POLY_END_PADDING, len/4 );

      t = request_storage(t, &t_size, sizeof(double), len, "measure");
      xd = request_storage(xd, &xd_size, sizeof(double), len, "measure");
      yd = request_storage(yd, &yd_size, sizeof(double), len, "measure");
      { int i = len; // convert floats to doubles
        while(i--)
        { xd[i] = x[i];
          yd[i] = y[i];
        }
      }

      for( i=0; i<len; i++ )
        t[i] = cumlen[i] / path_length; // [0 to 1]

#ifdef DEBUG_WHISKER_IO_POLYFIT_ERROR
      assert(t[0] == 0.0 );
      assert( (t[len-1] - 1.0)<1e-6 );
#endif

      // polynomial fit
      polyfit( t+pad, xd+pad, len-2*pad, WHISKER_IO_POLY_DEGREE, px, workspace );
      polyfit_reuse(  yd+pad, len-2*pad, WHISKER_IO_POLY_DEGREE, py, workspace );
    }
  }
  
  // median score
  // ------------
  { qsort( s, len, sizeof(float), _score_cmp );
    if(len&1) // odd
      median_score = s[ (len-1)/2 ];
    else      //even
      median_score = ( s[len/2 - 1] + s[len/2] )/2.0;
  }
  
  // write a record
  // --------------
  if( w->len )
  { fwrite( (trunc_WSeg*) w, sizeof(trunc_WSeg) , 1                       , file );
    fwrite( &median_score  , sizeof(float)      , 1                       , file );
    fwrite( px             , sizeof(double)     , WHISKER_IO_POLY_DEGREE+1, file );
    fwrite( py             , sizeof(double)     , WHISKER_IO_POLY_DEGREE+1, file );
  }
}

Whisker_Seg *read_segments_whiskpoly1( FILE *file, int *n)
{ typedef struct {int id; int time; int len;} trunc_WSeg;
  Whisker_Seg *wv;
  int i;
  static double *t = NULL;
  static size_t  t_size = 0;

  *n = peek_whiskpoly1_footer(file); //read in number of whiskers
#ifdef DEBUG_WHISKER_IO_POLYFIT_READ
  debug("Number of segments: %d\n",*n);
#endif
  wv = (Whisker_Seg*) Guarded_Malloc( sizeof(Whisker_Seg)*(*n), "read whisker segments - format: whiskpoly1");

  for( i=0; i<(*n); i++ )
  { Whisker_Seg *w = wv + i;
    int j,len;
    double px[WHISKER_IO_POLY_DEGREE+1],
           py[WHISKER_IO_POLY_DEGREE+1];
    float s;
    float *x, *y, *thick, *scores;

    fread( w, sizeof( trunc_WSeg ), 1, file ); //populates id,time (a.k.a frame id),len
    len = w->len;
    linspace_d( 0.0, 1.0, len, &t, &t_size );

    x      = w->x      = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (whiskpoly1 format)" );
    y      = w->y      = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (whiskpoly1 format)" );
    thick  = w->thick  = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (whiskpoly1 format)" );
    scores = w->scores = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (whiskpoly1 format)" );

    fread( &s, sizeof(float),  1, file );
    fread( px, sizeof(double), WHISKER_IO_POLY_DEGREE+1, file );
    fread( py, sizeof(double), WHISKER_IO_POLY_DEGREE+1, file );

#ifdef DEBUG_WHISKER_IO_POLYFIT_READ
    debug("Row: %d\n"
          "   fid:%5d wid:%5d len:%5d\n"
          "   Median score: %f\n"
          "   px[0] %5.5g px[end] %5.5g\n"
          "   py[0] %5.5g py[end] %5.5g\n"
        ,i,w->time, w->id, w->len,s
        ,px[0],px[WHISKER_IO_POLY_DEGREE] 
        ,py[0],py[WHISKER_IO_POLY_DEGREE]
        );
#endif
    for( j=0; j<len; j++ )
    { x[j] = (float) polyval( px, WHISKER_IO_POLY_DEGREE, t[j] );
      y[j] = (float) polyval( py, WHISKER_IO_POLY_DEGREE, t[j] ); 
      thick[j]  = 1.0;
      scores[j] = s;
    }
  }
  return wv;
}

void append_segments_whiskpoly1( FILE *file, Whisker_Seg *wv, int n )
{ int count,i;

  count = peek_whiskpoly1_footer(file);
  for(i=0; i<n; i++)
    write_whiskpoly1_segment( file, wv + i );
  write_whiskpoly1_footer( file, count + n );
}
