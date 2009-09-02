#include "compat.h"
#include "common.h"
#include "utilities.h"
#include "trace.h"
#include "whisker_io.h"
#include "traj.h"
#include <math.h>
#include <string.h>
#include <assert.h>
#include "poly.h"
#include "mat.h"

#if 0  // Tests
#define TEST_MEASURE_1
#endif

//
// The degree was chosen such that over the data set
// the mean residual after fitting was less than one
// pixel (see asserts flipped on by defining 
// DEBUG_MEASURE_POLYFIT_ERROR).
//
#define MEASURE_POLY_FIT_DEGREE  3
#define MEASURE_POLY_END_PADDING 16

#if 0
#define DEBUG_MEASURE_POLYFIT_ERROR 
#define DEBUG_MEASURE_FACE_POINT_FROM_HINT
#endif

int _score_cmp( const void *a, const void *b )
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
       
// Given the face center (cx,cy) compute the index
// corresponding to the follicle and return dI/di
// (I is the index as it proceeds outward from the
// follicle; dI/di is either 1 or -1).
static inline float _side_dist_to_face( Whisker_Seg* w, int cx, int cy, int idx )
{ float dx = w->x[idx] - cx,
        dy = w->y[idx] - cy;
  return dx*dx + dy*dy;
}
static float _side( Whisker_Seg *w, int cx, int cy, int *idx_follicle, int* idx_tip )
{ int beg = 0,
      end = w->len-1;
  float dbeg = _side_dist_to_face(w,cx,cy,beg),
        dend = _side_dist_to_face(w,cx,cy,end);
  if( dbeg < dend ) //test which side is closest to face
  { *idx_follicle = beg;
    *idx_tip      = end;
    return 1.0f;
  } else 
  { *idx_follicle = end;
    *idx_tip      = beg;
    return -1.0f;
  }
}

//
// Measure Whisker Segment Features
// --------------------------------
//
void Whisker_Seg_Measure( Whisker_Seg *w, double *dest, int facex, int facey )
{ float path_length,     //               
        median_score,    //
        root_angle_deg,  // side  poly
        mean_curvature,  //(side) poly quad?  (depends on side for sign)
        follicle_x,      // side
        follicle_y,      // side
        tip_x,           // side
        tip_y;           // side
  float *x = w->x,
        *y = w->y,
        *s = w->scores;
  int len = w->len,
      idx_follicle,
      idx_tip;
  float dx;
  static double *cumlen = NULL;
  static size_t  cumlen_size = 0;

  cumlen = request_storage( cumlen, &cumlen_size, sizeof(double), len, "measure: cumlen");
  cumlen[0] = 0.0;

  // path length
  // -----------
  // XXX: an alternate approach would be to compute the polynomial fit
  //      and do quadrature on that.  Might be more precise.
  //      Although, need cumlen
  { float *ax = x + 1,       *ay = y + 1,
          *bx = x,           *by = y;
    double *cl = cumlen + 1, *clm = cumlen;
    while( ax < x + len )
      *cl++ = (*clm++) + hypotf( (*ax++) - (*bx++), (*ay++) - (*by++) );
    path_length = cl[-1];
  }

  // median score
  // ------------
  { qsort( s, len, sizeof(float), _score_cmp );
    if(len&1) // odd
      median_score = s[ (len-1)/2 ];
    else      //even
      median_score = ( s[len/2 - 1] + s[len/2] )/2.0;
  }

  // Follicle and root positions
  // ---------------------------
  dx = _side( w, facex, facey, &idx_follicle, &idx_tip );

  follicle_x = x[ idx_follicle ];
  follicle_y = y[ idx_follicle ];
  tip_x = x[ idx_tip ];
  tip_y = y[ idx_tip ];

  // Polynomial based measurements
  // (Curvature and angle)
  // -----------------------------
  { double px[  MEASURE_POLY_FIT_DEGREE+1 ],
           py[  MEASURE_POLY_FIT_DEGREE+1 ],
           xp[  MEASURE_POLY_FIT_DEGREE+1 ],
           yp[  MEASURE_POLY_FIT_DEGREE+1 ],
           xpp[ MEASURE_POLY_FIT_DEGREE+1 ],
           ypp[ MEASURE_POLY_FIT_DEGREE+1 ],
           mul1[ 2*MEASURE_POLY_FIT_DEGREE ],
           mul2[ 2*MEASURE_POLY_FIT_DEGREE ],
           num[  2*MEASURE_POLY_FIT_DEGREE ],
           den[  2*MEASURE_POLY_FIT_DEGREE ]; 
    static double *t = NULL;
    static size_t  t_size = 0;
    static double *xd = NULL;
    static size_t  xd_size = 0;
    static double *yd = NULL;
    static size_t  yd_size = 0;
    static double *workspace = NULL;
    static size_t  workspace_size = 0;
    int i;
    const int pad = MIN( MEASURE_POLY_END_PADDING, len/4 );

    // parameter for parametric polynomial representation
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
#ifdef DEBUG_MEASURE_POLYFIT_ERROR
    assert(t[0] == 0.0 );
    assert( (t[len-1] - 1.0)<1e-6 );
#endif

    // polynomial fit
    workspace = request_storage( workspace, 
                                &workspace_size, 
                                 sizeof(double), 
                                 polyfit_size_workspace( len, 2*MEASURE_POLY_FIT_DEGREE ), //need 2*degree for curvature eval later
                                 "measure: polyfit workspace" );
    polyfit( t+pad, xd+pad, len-2*pad, MEASURE_POLY_FIT_DEGREE, px, workspace );
    polyfit_reuse(  yd+pad, len-2*pad, MEASURE_POLY_FIT_DEGREE, py, workspace );

#ifdef DEBUG_MEASURE_POLYFIT_ERROR
    { double err = 0.0;
      int i;
      for( i=0; i<len; i++ )
        err += hypot( xd[i] - polyval( px, MEASURE_POLY_FIT_DEGREE, t[i] ),
                      yd[i] - polyval( py, MEASURE_POLY_FIT_DEGREE, t[i] ) );
      err /= ((float)len);
      debug("Polyfit root mean squared residual: %f\n", err );
      assert( err < 1.0 );
    }
#endif

    // first derivative
    memcpy( xp, px, sizeof(double) * ( MEASURE_POLY_FIT_DEGREE+1 ) );
    memcpy( yp, py, sizeof(double) * ( MEASURE_POLY_FIT_DEGREE+1 ) );
    polyder_ip( xp, MEASURE_POLY_FIT_DEGREE+1, 1 );
    polyder_ip( yp, MEASURE_POLY_FIT_DEGREE+1, 1 );

    // second derivative
    memcpy( xpp, xp, sizeof(double) * ( MEASURE_POLY_FIT_DEGREE+1 ) );
    memcpy( ypp, yp, sizeof(double) * ( MEASURE_POLY_FIT_DEGREE+1 ) );
    polyder_ip( xpp, MEASURE_POLY_FIT_DEGREE+1, 1 );
    polyder_ip( ypp, MEASURE_POLY_FIT_DEGREE+1, 1 );

    // Root angle
    // ----------
    { double teval = (idx_follicle == 0) ? t[pad] : t[len-pad-1];
      static const double rad2deg = 180.0/M_PI;
      root_angle_deg = atan2( dx*polyval(yp, MEASURE_POLY_FIT_DEGREE, teval ),
                              dx*polyval(xp, MEASURE_POLY_FIT_DEGREE, teval ) ) * rad2deg;
    }

    // Mean curvature
    // --------------
    // Use the most naive of integration schemes
    { double  *V = workspace; // done with workspace, so reuse it for vandermonde matrix (just alias it here)
      static double *evalnum = NULL,
                    *evalden = NULL;
      static size_t evalnum_size = 0,
                    evalden_size = 0;
      size_t npoints = len-2*pad;
  
      evalnum = request_storage( evalnum, &evalnum_size, sizeof(double), npoints, "numerator" );
      evalden = request_storage( evalden, &evalden_size, sizeof(double), npoints, "denominator" );
  
      Vandermonde_Build( t+pad, npoints, 2*MEASURE_POLY_FIT_DEGREE, V ); // used for polynomial evaluation
  
      // numerator
      memset( mul1, 0, 2*MEASURE_POLY_FIT_DEGREE*sizeof(double) );
      memset( mul2, 0, 2*MEASURE_POLY_FIT_DEGREE*sizeof(double) );
      polymul( xp, MEASURE_POLY_FIT_DEGREE+1,
              ypp, MEASURE_POLY_FIT_DEGREE+1,
              mul1 );
      polymul( yp, MEASURE_POLY_FIT_DEGREE+1,
              xpp, MEASURE_POLY_FIT_DEGREE+1,
              mul2 );
      polyadd( mul1, 2*MEASURE_POLY_FIT_DEGREE,
               mul2, 2*MEASURE_POLY_FIT_DEGREE,
               num );
  
      // denominator
      memset( mul1, 0, 2*MEASURE_POLY_FIT_DEGREE*sizeof(double) );
      memset( mul2, 0, 2*MEASURE_POLY_FIT_DEGREE*sizeof(double) );
      polymul( xp, MEASURE_POLY_FIT_DEGREE+1,
               xp, MEASURE_POLY_FIT_DEGREE+1,
              mul1 );
      polymul( yp, MEASURE_POLY_FIT_DEGREE+1,
               yp, MEASURE_POLY_FIT_DEGREE+1,
              mul2 );
      polyadd( mul1, 2*MEASURE_POLY_FIT_DEGREE,
               mul2, 2*MEASURE_POLY_FIT_DEGREE,
               den );
  
      // Eval
      matmul(   V, npoints,                   MEASURE_POLY_FIT_DEGREE*2,
              num, MEASURE_POLY_FIT_DEGREE*2, 1,
              evalnum );
      matmul(   V, npoints,                   MEASURE_POLY_FIT_DEGREE*2,
              den, MEASURE_POLY_FIT_DEGREE*2, 1,
              evalden );
      // compute kappa at each t
      { int i;
        for(i=0; i<npoints; i++ )
          evalnum[i] /= pow( evalden[i], 3.0/2.0 )*dx; //dx is 1 or -1 so dx = 1/dx;
        mean_curvature = evalnum[0] * (t[1]-t[0]);
        for(i=1; i<npoints; i++ )
          mean_curvature += evalnum[i] * ( t[i]-t[i-1] );
      }
    }
  }

  // fill in fields
  dest[0] = path_length;
  dest[1] = median_score;
  dest[2] = root_angle_deg;
  dest[3] = mean_curvature;
  dest[4] = follicle_x;
  dest[5] = follicle_y;
  dest[6] = tip_x;
  dest[7] = tip_y;
}

Measurements *Whisker_Segments_Measure( Whisker_Seg *wv, int wvn, int facex, int facey )
{ Measurements *table = Alloc_Measurements_Table( wvn /* #rows */, 8/* #measurments */ ); 
  while(wvn--)
  { Measurements *row = table + wvn; 
    row->row   = wvn;
    row->fid   = wv[wvn].time;
    row->wid   = wv[wvn].id;
    row->state =  0;
    row->face_x = facex;
    row->face_y = facey;
    row->col_follicle_x = 4;
    row->col_follicle_y = 5;
    row->valid_velocity = 0;
    row->n = 8;
    Whisker_Seg_Measure( wv+wvn, row->data, facex, facey );  
  }
  return table;
}

void face_point_from_hint( Whisker_Seg *wv, int wvn, char* hint, int *x, int *y )
{ float maxx = 0.0, 
        maxy = 0.0;

  // Find maximum extent of whiskers in x and y
  // Assume min is zero
  while(wvn--)
  { Whisker_Seg *c = wv + wvn;
    int n = c->len;
    float *xp = c->x,
          *yp = c->y;
    while(n--)
    { float x = *xp,
            y = *yp;
      maxx = MAX( maxx, x );
      maxy = MAX( maxy, y );
    }
  }

  // Use hint to determine approximate center of face
  // hint may be "top", "left", "bottom", or "right"
  switch( hint[0] ) //just check the first character
  { case 'r':
    case 'R':
      *x = 3*maxx/2;
      *y =   maxy/2;
      break;
    case 'l':
    case 'L':
      *x =  -maxx/2;
      *y =   maxy/2;
      break;
    case 't':
    case 'T':
      *x =   maxx/2;
      *y =  -maxy/2;
      break;
    case 'b':
    case 'B':
      *x =   maxx/2;
      *y = 3*maxy/2;
      break;
    default:
      error("Did not recognize face hint (%s)\n"
            "\n"
            "Which side of the image is the face's center nearest?\n"
            "Options:"
            "\tright\n"
            "\tleft \n"
            "\ttop  \n"
            "\tbottom\n", hint);
  }
#ifdef DEBUG_MEASURE_FACE_POINT_FROM_HINT
  debug("Got hint: %s\n"
        "\tCenter at (%4d, %4d)\n", hint, *x, *y );
#endif
}

#ifdef TEST_MEASURE_1
char *Spec[] = {"<whiskers:string> <dest:string> --face (<x:int> <y:int>|<hint:string>)",NULL};
int main( int argc, char* argv[] )
{ Whisker_Seg *wv;
  int wvn;
  Measurements *table;
  int facex, facey;
  
  Process_Arguments( argc, argv, Spec, 0 );
  wv = Load_Whiskers( Get_String_Arg("whiskers"), NULL, &wvn);

  if( Is_Arg_Matched("hint") )
  { face_point_from_hint( wv, wvn,  Get_String_Arg("hint"), &facex, &facey );
  } else {
    facex = Get_Int_Arg("x");
    facey = Get_Int_Arg("y");
  }

  table = Whisker_Segments_Measure( wv, wvn, facex, facey );

  Measurements_Table_To_Filename( Get_String_Arg("dest"), table, wvn );

  Free_Measurements_Table( table );
  Free_Whisker_Seg_Vec(wv,wvn);

  return 0;
}
#endif
