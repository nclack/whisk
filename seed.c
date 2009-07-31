#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include "seed.h"
#include "image_lib.h"
#include "contour_lib.h"
#include "draw_lib.h"
#include "common.h"

#undef  DEBUG_COMPUTE_SEED_FROM_POINT

#undef  SHOW_BRANCHES
#undef  SHOW_TRACE
#undef  DEBUG_COMPUTE_SEED

/*
 * SEED - DECOMPOSITION OF CONTOURS
 *
 *
 */

//  TRASHES "image" !!
/*
**  Uses a levelset and size threshold to segment an image constrained by the
**  zone mask.
*/
SHARED_EXPORT
Object_Map *find_objects(Image *image, int vthresh, int sthresh)
{ static Object_Map mymap;
  static int        obj_max = 0;
  static Contour  **objects = NULL;

  static Paint_Brush zero = { 0., 0., 0. };

  uint8 *array  = image->array;

  int carea   = image->width*image->height;

  { int p, n;

    n = 0;
    for (p = 0; p < carea; p++)
      if (array[p] >= vthresh)                             // start at a non-masked pixel
        { Contour *c = Trace_Region(image,p,GE,vthresh,1); // trace the contour around that pixel
          Draw_Contour_Interior(c,&zero,image);            // mask out that countour
          if (Contour_Area(c) >= sthresh)                  // apply a size threshold to the region
            { if (n >= obj_max)
                { obj_max = 1.2*n+500;
                  objects = (Contour **)
                              Guarded_Realloc(objects,sizeof(Contour *)*obj_max,Program_Name());
                }
              objects[n++] = c;
            } else {
              Free_Contour( c );
            }
        }
    mymap.num_objects = n;
    mymap.objects     = objects;
  }

  return (&mymap);
}

SHARED_EXPORT
Seed *compute_seed(Raster *raster, int n, int x0, int width, uint8 *value)
{ static Seed myseed;

  int nb, ne, xc;
  int xfar, xctr;

  //if (raster[n].major < MIN_LENPRJ) 
  //  return (NULL);
  nb = raster[n].minor;    // beginning of y-interval
  ne = raster[n+1].minor;  // end       of y-interval

#ifdef DEBUG_COMPUTE_SEED
  printf("  Back (%d,%d):%d",nb,ne-1,raster[n].major);
#endif

  // compute location of minima on right-most y interval
  { int y, p;
    int xmin;

    xmin = 256;
    for (y = nb; y < ne; y++)
      { p = x0 + y*width;
        if (value[p] < xmin)
          { xfar = p;
            xmin = value[p];
          }
      }
  }

  // back up
  { int mb, me;
    int x;

    xc = x0 - raster[n].major/2;
    for (x = x0-1; x >= xc; x--)
      { while (raster[n+1].major > x)
          n -= 2;
        while (raster[n+1].major == x)
          { mb = raster[n].minor;
            me = raster[n+1].minor;
            if (me > nb && ne > mb)
              { nb = mb;
                ne = me;
                break;
              }
            n -= 2;
          }
      }
#ifdef DEBUG_COMPUTE_SEED
   printf(" to (%d,%d) @ %d\n",nb,ne,xc);
#endif
  }

  // find location of min along center
  { int y, p;
    int xmin;

    xmin = 256;
    for (y = nb; y < ne; y++)
      { p = xc + y*width;
        if (value[p] < xmin)
          { xctr = p;
            xmin = value[p];
          }
      }
  }

  //compute location and slope of seed
  { int xpnt, ypnt;
    int xdir, ydir;

    myseed.xpnt = xpnt = xctr%width;
    myseed.ypnt = ypnt = xctr/width;
    myseed.xdir = xdir = xfar%width - xpnt;
    myseed.ydir = ydir = xfar/width - ypnt;

#ifdef DEBUG_COMPUTE_SEED
    if (xdir*xdir + ydir*ydir < MIN_LENSQR)
      printf("\tREJECT: Seed at ( %4d, %4d )\n", myseed.xpnt, myseed.ypnt );
#endif

    if (xdir*xdir + ydir*ydir < MIN_LENSQR)
      return (NULL);
    else
      return (&myseed);
  }
}


SHARED_EXPORT
Seed_Vector *decompose_trace_x(Contour *trace, int width, int height, uint8 *value)
{ static Seed       *seedlist = NULL;
  static int         seedmax  = 0;
  static Seed_Vector myseeds;

  static Seed *boo;

  int    *yaster, ypairs;
  Raster *raster;

  int nseeds;
  int ntrk, mtrk, etrk;
  int r, x, x0;

  nseeds = 0;

  yaster = Yaster_Scan(trace,&ypairs,height);
  
  
  // reencode yaster as (x,y) points - in place
  raster = (Raster *) yaster; 
  for (r = 0; r < ypairs; r++)
  { int mj = yaster[r]/height; // x
    int mn = yaster[r]%height; // y
    raster[r].major = mj;      // x
    raster[r].minor = mn;      // y
  }

#ifdef SHOW_BRANCHES
  printf("\nTRACE:\n"); fflush(stdout);
#endif

  ntrk = mtrk = etrk = 0;
  r = 0;
  while (r < ypairs)
  { x0 = x = raster[r].major;  // the x value for this (vertical) raster
#ifdef SHOW_TRACE
    printf("  %d:",x0); fflush(stdout);
#endif

    ntrk = mtrk;   
    mtrk = etrk;
    // advance over vertically aligned intervals in the raster  
    while (x == x0)
    { raster[r].major = 0;     // mark it
#ifdef SHOW_TRACE
      //                      y0              y1
      printf(" (%d,%d)",raster[r].minor,raster[r+1].minor-1); fflush(stdout);
#endif
      r += 2;                  // move to next pair
      if (r >= ypairs) break;
      x = raster[r].major;
    }
    etrk = r;   // remember the end of this sequence of intervals
#ifdef SHOW_TRACE
    printf("\n"); fflush(stdout);
#endif

    { int  m, n;
      int  mb, me;
      int  nb, ne;
      int   c, o;
      Seed *s;

      m = mtrk;
      n = ntrk;
      c = 0;
      while (n < mtrk)
      { if (m >= etrk)            //if the end of this sequence of intervals has been passed...
          mb = me = ne;           //...empty m interval located at end of n
        else                      
        { mb = raster[m].minor;   //beginning of the m'th interval
          me = raster[m+1].minor; //end       of the m'th interval
        }
        nb = raster[n].minor;     //beginning of the n'th interval
        ne = raster[n+1].minor;   //end       of the n'th interval

        // init
        if (me > nb && ne > mb)   //if the intervals have an intersection...
        { raster[m].major += 1;   //...increment m's id
          c += 1;                 //...count
        }

        if (me < ne)              //if m's end is before n's ...
          m += 2;                 //...increment to interval following m and move on
        else                      //else m and n intersect or m is past n
        { raster[n+1].major = -1; //...mark next
          if (c > 1)                  //if there was more than one intersection event...
          { // move o back till all intersection counts are accounted for
            //   and add counts into interval labels
            o = m;
            while (1)
            { if (me > nb && ne > mb) // if m and n intersect ...
              { raster[o].major += 1; // ...increment label
                if (--c <= 0) break;  // ...clear counts
              }
              o -= 2;                 // back up an interval
              mb = raster[o].minor;
              me = raster[o+1].minor;
            }
          }
          else if (c == 1)            // if only one intersection count
          { if (ne > mb)
            { if ((n+2 >= mtrk || raster[n+2].minor >= me) && raster[m].major <= 1)
              raster[n+1].major = m;
            }
            else
            { if (raster[m-2].major <= 1)
              raster[n+1].major = m-2;
            }
          }
#ifdef SHOW_BRANCHES
          if (raster[n+1].major < 0)
            printf("Track (%d,%d) %d expires\n",
                raster[n].minor,raster[n+1].minor-1,raster[n].major);
#endif
          n += 2;
          c  = 0;
        }
      }

      for (m = mtrk; m < etrk; m += 2) //iterate over sequence of vertically aligned intervals
        if (raster[m].major != 1)      //if unlabelled
        {
#ifdef SHOW_BRANCHES
          printf("Track (%d,%d) = %d starts\n",
              raster[m].minor,raster[m+1].minor-1,raster[m].major);
#endif
          raster[m].major = 1;         //...init label
        }

      for (n = ntrk; n < mtrk; n += 2)
      { c = raster[n+1].major;
        raster[n+1].major = x0-1;      // set back to x0
        if (c < 0)                     // if termination...compute seed on n'th
        { if (nseeds >= seedmax)
          { seedmax  = 1.2*nseeds + 10;
            seedlist = (Seed *)
              Guarded_Realloc(seedlist,sizeof(Seed)*seedmax,Program_Name());
          }
          s = compute_seed(raster,n,x0-1,width,value);
          if (s != NULL)
            seedlist[nseeds++] = *s;
        }
        else
          raster[c].major = raster[n].major+1;
      }
    } //end context
  } //end while

  { int   m;
    Seed *s;

    for (m = mtrk; m < etrk; m += 2)
    { if (nseeds >= seedmax)
      { seedmax  = 1.2*nseeds + 10;
        seedlist = (Seed *)
          Guarded_Realloc(seedlist,sizeof(Seed)*seedmax,Program_Name());
      }
      s = compute_seed(raster,m,x0,width,value);
      if (s != NULL)
        seedlist[nseeds++] = *s;
#ifdef SHOW_BRANCHES
      printf("Track (%d,%d) %d = 0 expires\n",
          raster[m].minor,raster[m+1].minor-1,raster[m].major);
#endif
    }
  }

  myseeds.nseeds = nseeds;
  myseeds.seeds  = seedlist;
  return (&myseeds);
}

SHARED_EXPORT
Seed_Vector *find_seeds(Contour *trace, Image *image)
{ //Contour_Extent *extent;
  //extent = Contour_Get_Extent(trace);
  return (decompose_trace_x(trace,image->width,image->height,image->array));
}

/***                                            ***
 ***                                            ***
 *** SEED - LOCAL LINEAR CORRELATION OF MINIMA  ***
 ***                                            ***
 ***                                            ***
 ***/

// Warning: no bounds checking
#if 0
inline void _compute_seed_from_point_helper(
              Image* image, int x, int y, int cx, int cy,
              uint8 *best, int *bp )
{ int tp = x+cx + image->width * (y+cy);
  uint8 *val = image->array + tp;
  if(  *val <= *best )
  { *bp = tp;
    *best = *val;
  }
  return;
}
#endif

#define _COMPUTE_SEED_FROM_POINT_HELPER(BEST,BP)                    \
{ int tp = x+cx + image->width * (y+cy);                            \
  uint8 val = *( ((uint8*) image->array) + tp);                     \
  if(   val <= BEST )                                               \
  { BP = tp;                                                        \
    BEST =  val;                                                    \
  }                                                                 \
}

inline float _compute_seed_from_point_eigennorm( float th )
{ //float th = atanf( slope );
  float cs = fabsf(cos(th)),
        ss = fabsf(sin(th));
  return 1.0f/MAX(ss,cs);
}

SHARED_EXPORT
Seed *compute_seed_from_point( Image *image, int p, int maxr )
{ float m, stat;
  return compute_seed_from_point_ex( image, p, maxr, &m, &stat);
}

SHARED_EXPORT
Seed *compute_seed_from_point_ex( Image *image, int p, int maxr, float *out_m, float *out_stat)
  /* Specific for uint8 */
{ static Seed myseed;
  static const float eps = 1e-3;
  int i = -1, rnpoints = 0, lnpoints = 0;
  int stride = image->width;
  float lsx   = 0.0, /* statistics for left corner cut: (ab,cd) grouping */
        lsy   = 0.0,
        lsxy  = 0.0,
        lsxx  = 0.0,
        lsyy  = 0.0;
  float rsx   = 0.0, /* statistics for right corner cut: (ad,bc) grouping */
        rsy   = 0.0,
        rsxy  = 0.0,
        rsxx  = 0.0,
        rsyy  = 0.0;
  /* Spiral out from center
   * Collect pixels minimal over set of pixels with same L0 distance from p
   * Form seed by computing center and slope of collected pixels
   * Use linear regression
   */
  int tp, cx,cy,x,y;
  cx=cy=0;
  x = p%stride;
  y = p/stride;
#ifdef DEBUG_COMPUTE_SEED_FROM_POINT
  printf("Test point (%d,%d). stride = %d\n",x,y,stride);
#endif
  if( (x < maxr)                   || // Computation isn't valid for boundary
      (x >=(image->width  - maxr)) ||
      (y < maxr)                   ||
      (y >=(image->height - maxr)) )
  { *out_m = 0.0;
    *out_stat = 0.0;
    return NULL;
  }

  while( i++ < maxr)
  { int abp,bbp,cbp,dbp, bp = -1;              //best points
    uint8 abest,bbest,cbest,dbest, best = 255; //best mins
    int j,maxj;
#ifdef DEBUG_COMPUTE_SEED_FROM_POINT
    printf("  i: %d\n",i);
#endif
    abp = -1;
    abest = 255;
    j = maxj = 2*i;
    // do one loop of the spiral
    //for(j=0;j<maxj;j++)
    while( j-- )
    { --cy;
      _COMPUTE_SEED_FROM_POINT_HELPER(abest,abp);
    }
    bbp = -1;
    bbest  = 255;
    //for(j=0;j<maxj;j++)
    j = maxj;
    while( j-- )
    { --cx;
      _COMPUTE_SEED_FROM_POINT_HELPER(bbest,bbp);
    }
    cbest = 255;
    cbp = -1;
    j = maxj;
    while( j-- )
    //for(j=0;j<maxj;j++)
    { ++cy;
      _COMPUTE_SEED_FROM_POINT_HELPER(cbest,cbp);
    }
    dbest = 255;
    dbp = -1;
    //for(j=0;j<maxj;j++)
    j = maxj;
    while( j-- )
    { ++cx;
      _COMPUTE_SEED_FROM_POINT_HELPER(dbest,dbp);
    }
    cx++; cy++;

#ifdef DEBUG_COMPUTE_SEED_FROM_POINT
    printf( "\t\tabp: %7d abest %7d\n", abp, abest );
    printf( "\t\tbbp: %7d bbest %7d\n", bbp, bbest );
    printf( "\t\tcbp: %7d cbest %7d\n", cbp, cbest );
    printf( "\t\tdbp: %7d dbest %7d\n", dbp, dbest );
#endif

    /*
     * a:     top  edge
     * b:     left edge
     * c:     bottom edge
     * d:     right edge
     */
    /* Integrate statistics for (ab,cd) grouping */
    bp = (abest < bbest) ? abp : bbp;  // (ab)
    if( bp >= 0 )
    { int tx = bp%stride,
          ty = bp/stride;
#ifdef DEBUG_COMPUTE_SEED_FROM_POINT
      best = (abest < bbest) ? abest : bbest;
      printf("\t(%5d,%5d): %d\n",tx,ty,best);
#endif
      lsx += tx;
      lsy += ty;
      lsxy += tx*ty;
      lsxx += tx*tx;
      lsyy += ty*ty;
      lnpoints++;
    }
    bp = (cbest < dbest) ? cbp : dbp; // (cd)
    if( bp > 0 )
    { int tx = bp%stride,
          ty = bp/stride;
#ifdef DEBUG_COMPUTE_SEED_FROM_POINT
      best = (cbest < dbest) ? cbest : dbest;
      printf("\t(%5d,%5d): %d\n",tx,ty,best);
#endif
      lsx += tx;
      lsy += ty;
      lsxy += tx*ty;
      lsxx += tx*tx;
      lsyy += ty*ty;
      lnpoints++;
    }
    /* Integrate statistics for (ad,cb) grouping */
    bp = (abest < dbest) ? abp : dbp; //(ad)
    if( bp >= 0 )
    { int tx = bp%stride,
          ty = bp/stride;
#ifdef DEBUG_COMPUTE_SEED_FROM_POINT
      best = (abest < dbest) ? abest : dbest;
      printf("\t(%5d,%5d): %d\n",tx,ty,best);
#endif
      rsx += tx;
      rsy += ty;
      rsxy += tx*ty;
      rsxx += tx*tx;
      rsyy += ty*ty;
      rnpoints++;
    }
    bp = (cbest < bbest) ? cbp : bbp; //(cb)
    if( bp > 0 )
    { int tx = bp%stride,
          ty = bp/stride;
#ifdef DEBUG_COMPUTE_SEED_FROM_POINT
          best = (cbest < bbest) ? cbest : bbest;
          printf("\t(%5d,%5d): %d\n",tx,ty,best);
#endif
          rsx += tx;
          rsy += ty;
          rsxy += tx*ty;
          rsxx += tx*tx;
          rsyy += ty*ty;
          rnpoints++;
    }
  } // end search

  /* How well do the collected points distribute in a line?
   * Measure the slope
   * */
  { float rm,rnum,lm,lnum; //,rr2,lr2;
    float lstat,rstat;
    //lnum = lnpoints*lsxy - lsx*lsy;          //numerator - for linear regression
    //lm = atan2( lnum,  lnpoints*lsxx - lsx*lsx); //slope angle  - by linear regression
    //lr2 = lnum * lm / ( lnpoints*lsyy - lsy*lsy + eps); // pierson's correlation coefficient
    if( lnpoints <= 3 )
    { lstat = 0.0f;
      lm = 0.0f;
    } else
    { //principle components
      float cxx,cxy,cyy,trace,det,desc,eig0,eig1,stat;
      float n = lnpoints, n2 = lnpoints*lnpoints;
      cxx = lsxx/n - lsx*lsx/n2;
      cxy = lsxy/n - lsx*lsy/n2;
      cyy = lsyy/n - lsy*lsy/n2;
      trace = cxx + cyy;
      det   = cxx*cyy - cxy*cxy;
      desc  = trace*trace - 4.0f*det; // when is this negative? Both eig's imaginary?
#ifdef DEBUG_COMPUTE_SEED_FROM_POINT
      if( desc < 0.0)
        printf("Warning(L): Imaginary eigenvalues. desc: %g\n\t trace: %g, det: %g\n\tcxx: %g, cxy: %g, cyy: %g\n\n", desc, trace, det,cxx,cxy,cyy);

#endif
      desc = sqrtf(desc);
      eig0  = 0.5f*( trace + desc );
      eig1  = 0.5f*( trace - desc );
      lstat  = eig0 - eig1; //1.0f - eig1/eig0;
      lm   = atan2( cxx - eig0, -cxy );
#ifdef DEBUG_COMPUTE_SEED_FROM_POINT
      printf(" (%7.5g, %7.5g) (%7.5g, %7.5g) L m = %g\n", lsx/lnpoints, lsy/lnpoints, eig0, eig1, lm );
#endif
    }

    //rnum = rnpoints*rsxy - rsx*rsy;          //numerator - for linear regression
    //rm = atan2(rnum , rnpoints*rsxx - rsx*rsx); //slope angle    - by linear regression
    //rr2 = rnum * rm / ( rnpoints*rsyy - rsy*rsy + eps);  // pierson's correlation coefficient
    if( rnpoints <= 3 )
    { rstat = 0.0f;
    } else
    { //principle components
      float cxx,cxy,cyy,trace,det,desc,eig0,eig1,stat;
      float n = rnpoints, n2 = rnpoints*rnpoints;
      cxx = rsxx/n - rsx*rsx/n2;
      cxy = rsxy/n - rsx*rsy/n2;
      cyy = rsyy/n - rsy*rsy/n2;
      trace = cxx + cyy;
      det   = cxx*cyy - cxy*cxy;
      desc  = trace*trace - 4.0f*det; // when is this negative? Both eig's imaginary?
#ifdef DEBUG_COMPUTE_SEED_FROM_POINT
      if( desc < 0.0)
        printf("Warning(R): Imaginary eigenvalues. desc: %g\n\t trace: %g, det: %g\n\tcxx: %g, cxy: %g, cyy: %g\n\n", desc, trace, det,cxx,cxy,cyy);

#endif
      desc = sqrtf(desc);
      eig0  = 0.5f*( trace + desc );
      eig1  = 0.5f*( trace - desc );
      rstat  = eig0 - eig1;  //1.0f - eig1/eig0;
      rm   = atan2( cxx - eig0, -cxy );
#ifdef DEBUG_COMPUTE_SEED_FROM_POINT
      printf(" (%7.5g, %7.5g) (%7.5g, %7.5g) R m = %g\n", rsx/rnpoints, rsy/rnpoints, eig0, eig1, rm );
#endif
    }

    // choose the set that collected the most line-like distribution
    { float norm;
      if( lstat > rstat )
      { myseed.xpnt = (int) lsx/lnpoints;
        myseed.ypnt = (int) lsy/lnpoints;
        myseed.xdir = 100*cos(lm);
        myseed.ydir = (int) (100*sin(lm));

        norm = maxr * _compute_seed_from_point_eigennorm( lm ); // weights by length of line in square
        *out_m = lm;
        *out_stat = lstat/( norm*norm ); // normalize by length squared since the eigenvalue is variance
      } else
      { myseed.xpnt = (int) rsx/rnpoints;
        myseed.ypnt = (int) rsy/rnpoints;
        myseed.xdir = 100*cos(rm);
        myseed.ydir = (int) (100*sin(rm));

        norm = maxr * _compute_seed_from_point_eigennorm( rm );
        *out_m = rm;
        *out_stat = rstat/( norm * norm );
      }
    }
  }

  return &myseed;
}

SHARED_EXPORT
void compute_seed_from_point_histogram( Image *image, int maxr, Image *hist)
  // Assumes `image` and 'hist' are 8bit grayscale
{ int a = image->width * image->height;
  int stride = image->width;
  uint8 *h = hist->array;
  float m,stat;
  memset( hist->array, 0, a );
  while(a--)
  { Seed *s;
    int newp=a, p=a;
    int i;
    for( i=0; i< maxr/*maxiter*/; i++ )
    { p = newp;
      s = compute_seed_from_point_ex(image, p, maxr, &m, &stat); //return NULL on boundary
      if( !s ) break;
      newp = s->xpnt + stride * s->ypnt;
      if ( newp == p || stat < 0.1f)
        break;
    }
    if( s && stat > 0.1f )
      h[p]++;
  }
}


SHARED_EXPORT
void compute_seed_from_point_field_windowed( Image *image, int maxr, float statlow, float stathigh,
                                    Image *hist, Image *slopes, Image *stats)
  // Assumes `image` and 'hist' are 8bit grayscale
  //         `slopes` and `stats` should be float
{ int a = image->width * image->height;
  int stride = image->width;
  uint8 *h = hist->array;
  float *sl = (float*) slopes->array;
  float *st = (float*) stats->array;
  float m,stat;
  //int *index = pixels_ordered_by_isolines( image );
  memset( h, 0, a );
  memset( sl, 0, a * sizeof(float) );
  memset( st, 0, a * sizeof(float) );
  while(a--)
  { Seed *s;
    //int newp=index[a], p=index[a];
    int newp=a, p=a;
    int i;
    for( i=0; i< maxr/*maxiter*/; i++ ) // iterate - detector attracts to nearest line
    { p = newp;
      s = compute_seed_from_point_ex(image, p, maxr, &m, &stat); //return NULL on boundary
      if( !s ) break;
      newp = s->xpnt + stride * s->ypnt;
      if ( newp == p || stat < statlow)
        break;
    }
    if( s && stat > stathigh )
    { h[p]++;              // integrate on predicted point
      sl[p] += m;
      st[p]  = MAX( st[p], stat ); // += stat
    }
  }
  a = image->width * image->height;
  while( a-- )
  { uint8 n = h[a];
    if(n)
    { sl[a] /= n;
      //st[a] /= n;
    }
  }
}                                     

SHARED_EXPORT
void compute_seed_from_point_field( Image *image, int maxr,
                                    Image *hist, Image *slopes, Image *stats)
{ compute_seed_from_point_field_windowed( image, maxr, 0.1, 0.4, hist, slopes, stats );
}

SHARED_EXPORT
void compute_seed_from_point_field_windowed_on_contour( Image *image, Contour *trace,
                                                        int maxr, float statlow, float stathigh,
                                                        Image *hist, Image *slopes, Image *stats )
  // Assumes `image` and 'hist' are 8bit grayscale
  //         `slopes` and `stats` should be float
{ int     a       = image->width * image->height;
  int     idx     = trace->length;
  int     stride  = image->width;
  uint8  *h       = hist->array;
  float  *sl      = (float*) slopes->array;
  float  *st      = (float*) stats->array;
  float   m,stat;
  while(idx--)
  { Seed *s;
    int newp, p;
    int i;
    newp = p = trace->tour[idx];
    for( i=0; i< maxr/*maxiter*/; i++ ) // iterate - detector attracts to nearest line
    { p = newp;
      s = compute_seed_from_point_ex(image, p, maxr, &m, &stat); //return NULL on boundary
      if( !s ) break;
      newp = s->xpnt + stride * s->ypnt;
      if ( newp == p || stat < statlow)
        break;
    }
    if( s && stat > stathigh )
    { h[p]++;              // integrate on predicted point
      sl[p] += m;
      st[p] += stat; // = MAX( st[p], stat ); // += stat
    }
  }
}

SHARED_EXPORT
void compute_seed_from_point_field_on_grid( Image *image, int spacing,
                                            int maxr, float statlow, float stathigh,
                                            Image *hist, Image *slopes, Image *stats )
{ int     a       = image->width * image->height;
  int     stride  = image->width;
  uint8  *h       = hist->array;
  float  *sl      = (float*) slopes->array;
  float  *st      = (float*) stats->array;
  float   m,stat;

  //printf("On grid\n");

  // horizontal lines
  { int x,y;
    int p,newp,i;
    Seed *s;
    for( x=0; x<stride; x++ )
    { for( y=0; y<image->height; y += spacing )
      { newp = x+y*stride;
        p = newp;
        for( i=0; i < maxr; i++ )
        { p = newp;
          s = compute_seed_from_point_ex(image, x+y*stride, maxr, &m, &stat);
          if( !s ) break;
          newp = s->xpnt + stride * s->ypnt;
          if ( newp == p || stat < statlow )
            break;
        }
        if( s && stat > stathigh )
        { h[p] ++;
          sl[p] += m;
          st[p] += stat;
        }
      }
    }
  }
  // Vertical lines
  { int x,y;
    int p,newp,i;
    Seed *s;
    for( x=0; x<stride; x+=spacing )
    { for( y=0; y<image->height; y ++ )
      { newp = x+y*stride;
        p = newp;
        for( i=0; i < maxr; i++ )
        { p = newp;
          s = compute_seed_from_point_ex(image, x+y*stride, maxr, &m, &stat);
          if( !s ) break;
          newp = s->xpnt + stride * s->ypnt;
          if ( newp == p || stat < statlow )
            break;
        }
        if( s && stat > stathigh )
        { h[p] ++;
          sl[p] += m;
          st[p] += stat;
        }
      }
    }
  }
}

SHARED_EXPORT
Seed_Vector *find_seeds2( Contour *trace, Image *image )
{ static Seed_Vector sv;
  static Seed *seeds = NULL;
  static int   maxseeds = 0;

  int   maxr      = 4;
  int   width     = image->width;
  int   height    = image->height;
  int   stepsize  = MIN_LENGTH*4; 
  int   nn        = trace->length; //MAX( trace->length / stepsize, 1);
  int   i         = 0;
  int   count     = 0;
  
  seeds = (Seed*) request_storage( seeds, &maxseeds, sizeof(Seed), 50, "find_seeds2");   

  if( stepsize > nn )
    stepsize = nn/2 + 1;
  
  // Iterate to first pixel away from border
  while( i++ < nn )
  { int p = trace->tour[i];
    int x = p%width,
        y = p/width;
    if( (x > maxr)         &&
        (x < width - maxr) &&
        (y > maxr)         &&
        (y < height - maxr) )
      break;
  }
  if( i == nn ) // if entire contour lies on borded, exclude
    return (NULL);
  
  do
  { float th, stat;
    Seed *s;
    int p = trace->tour[ i % (trace->length) ];
    int newp, j;                         
   
    // iterate - detector attracts to nearest line
    newp = p;
    for( j=0; j< maxr/*maxiter*/; j++ ) 
    { p = newp;
      s = compute_seed_from_point_ex(image, p, maxr, &th, &stat); //return NULL on boundary
      if( !s ) break;
      newp = s->xpnt + width * s->ypnt;
      if ( newp == p || stat < 0.1f)
        break;
    }

    // collect
    if(s)
    { //printf("Seed: (%5d,%5d) (%5d,%5d) th = %7.5g, stat = %7.5g\n",
      //              s->xpnt, s->ypnt, s->xdir, s->ydir, th*180.0f/M_PI, stat );
      if( stat > 0.4 ) // accept - 2*s is in [0,1], 2s above 0.80 is significant
      { seeds = (Seed*) request_storage( seeds, &maxseeds, sizeof(Seed), count+1, "find_seeds2");
        seeds[count++] = *s;
      }
    }

  } while( (i += stepsize) < nn );
  sv.nseeds = count;
  sv.seeds  = seeds;
  return &sv;
}
