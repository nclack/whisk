/*
 * Author: Nathan Clack
 * Date  : 2008-10-15 - see subversion for history
 *
 * Most testing done through python/ctypes.  See doctests in distance.py.
 */
#include <float.h>
#include "trace.h"
#include "distance.h"
#include "math.h"
#include "string.h"
#include "common.h"
#include "compat.h"

#include <stdio.h>


#undef  DEBUG_WHISKER_HIT
#undef  DEBUG_WHISKER_NEAREST_NEIGHBORS
#undef  DEBUG_WHISKER_NEAREST_NEIGHBORS_HSB
#undef  DEBUG_WHISKER_NEAREST_NEIGHBORS_HTE
#undef  DEBUG_COMPUTE_AVERAGE_DISTANCE_BETWEEN_WHISKERS
#undef  DEBUG_WHISKER_PAIR_DISTANCE
#undef  DEBUG_COMPUTE_AREA_BETWEEN_WHISKERS
#undef  DEBUG_COMPUTE_POLYGON_AREA
#undef  DEBUG_COMPUTE_AREA_WHISKER_INTERSECTION 

void print_whisker_old( Whisker_Seg_Old *w )
{ int n = w->end - w->beg + 1;
  printf("Whisker %d:\n",w->id);
  printf("\twidth: %d\n",w->width );
  printf("\ttime : %d\n",w->time  );
  printf("\ttrack  @ %p\n",w->track);
  printf("\tscores @ %p\n",w->scores);
  printf("\tx: %d --> %d\n",w->beg, w->end );
  printf("\ty: %g --> %g\n",w->track[0], w->track[ n-1 ] );
}

void print_whisker( Whisker_Seg *w )
{ int n = w->len;;
  printf("Whisker %d:\n",w->id);
  printf("\tlen   : %d\n",w->len);
  printf("\ttime  : %d\n",w->time  );
  printf("\tx     : %5.3g --> %5.3g\n",w->x[0], w->x[ n ] );
  printf("\ty     : %5.3g --> %5.3g\n",w->y[0], w->y[ n ] );
  printf("\twidth : %5.3g --> %5.3g\n",w->thick[0], w->thick[ n ] );
  printf("\tscores: %5.3g --> %5.3g\n",w->scores[0], w->scores[ n ] );
}

float compute_dy_old( Whisker_Seg_Old *w, int side, int n )
  /* Computes average slope near one end of the whisker using n points 
   *
   * Uses two passes to try to remove outliers.
   * */
{ float dyseg, dy = 0.0, last, *t;
  int i, anchor, step, sign=0, count=0;

  t = w->track;
  
  if( n > (w->end - w->beg) )
    n = w->end - w->beg;

  if( !side ) /* test from the beginning */
  { anchor = 0;
  } else      /* test from the end       */ 
  { anchor = w->end - w->beg - n;
  }

  /* First pass, estimate sign of the slope.  Later things with opposite slope
   * will be treated as outliers
   */
  for( i = anchor + 1; i < anchor + n; i++ )
  { dyseg = t[i] - t[i-1];
    if( dyseg < 0 )
    { sign -= 1;
    } else if( dyseg > 0 )
    { sign += 1;
    }
  }

  /* Second pass, compute average slope treating segments with rare slope as
   * outliers
   */
  for( i = anchor + 1; i <= anchor + n; i++ )
  { dyseg = t[i] - t[i-1];
    if( (sign==0) || (dyseg*sign > 0) ) /* if they have the same sign */
    { dy += dyseg;
      count++;
    }
    //printf("%d, %d, %g, %g\n", sign, i, t[i], dy);
  }
  return dy/count;
}

float compute_dy( Whisker_Seg *w, int side, int n )
  /* Computes average slope near one end of the whisker using n points 
   *
   * Uses two passes to try to remove outliers.
   * */
{ int i, sign, anchor;
  float  *x = w->x,
         *y = w->y;
  if(side)               /* test from end */
    anchor = w->len - n;
  else
    anchor = 0;          /* test from beginning */

  /* First pass, Estimate sign of slope */
  for( i = anchor; i < n; i++ )
  { float   dy = y[i] - y[i-1],
            dx = x[i] - x[i-1];
    if( fabs(dx) > 1e-6 )
    { if( dx*dy > 0 ) /* slope is positive */
        sign += 1;
      else            /* slope is negative */
        sign -= 1;
    }
  }

  /* Second pass, compute average slope neglecting outliers */
  { float sdy = 0.0, sdx = 0.0;
    for( i = anchor; i < n; i++ )
    { float  dy = y[i] - y[i-1],
             dx = x[i] - x[i-1];
      if( (dx==0) || (sign == 0) || ( dy*dx*sign > 0 ) ) /* if same sign */
        { sdy += dy;
          sdx += dx;
        }
    }
    return sdy/(1e-7 + sdx);
  }
}

int hit( Whisker_Seg *q, Whisker_Seg *target, int side, int tstart, int *intersection )
/* Projects a line from one end of the whisker q.  If `side` is 0, projects
 * from the `beg` (small x) If `side` is 1, projects from the `end` (large x)
 *
 * The projection extends normal to the whisker, `q`.  This function returns
 * true of the target whisker is hit by this line.
 *
 * If there's a hit, then the location of the hit is returned as an index into
 * the target whisker's track in the `intersection` variable.  The search
 * starts at the index `tstart` into the target whisker's track.  
 *
 */
{ float dy,by;
  float ty,t,y;
  float bx,tx;
  int i;
  float sign;

  /* 
   * calculate line parameters.  dy/dx is tangent to the tips 
   */
  if( !side )               /* test from the beginning */
  { bx = q->x[0];
    by = q->y[0];
    dy = compute_dy( q, side, 30);
  } else {                  /* test from the end       */ 
    int iend = q->len - 1;
    bx = q->x[ iend ];
    by = q->y[ iend ]; 
    dy = compute_dy( q, side, 30);
  }

#ifdef DEBUG_WHISKER_HIT
  printf("bx: %5.5g  by: %5.5g  dy: %5.5g\n", bx,by,dy );
#endif
  /* First, this needs to be friendly to near misses.  Near misses happen 
   * when the line extending along the normal from one whisker comes close, but
   * misses the end of the target whisler.
   *
   * This only happens for the very ends of the whisker, so it's a single check
   * for each side.
   */
  tx = target->x[ tstart ];
  ty = target->y[ tstart ];

  { float magn, magqt, qtx, qty;
    float costheta;
    int candidate;
    if( !side )
    { qtx = tx - bx;
      qty = ty - by;
      candidate = tstart;
    } else 
    { candidate = target->len - 1; //target->end - target->beg;
      qtx = target->x[ candidate ] - bx; //target->end - bx;
      qty = target->y[ candidate ] - by;
    }
    magn = sqrt(dy*dy + 1);
    magqt = sqrt( qtx*qtx + qty*qty );
    costheta = (qtx + qty*dy) / ( magn * magqt );
#ifdef DEBUG_WHISKER_HIT
    printf("\tcostheta: %10.10g (%g,%g)\n",costheta,qtx,qty);
#endif
    if( fabs(costheta) < 0.173648177667 ) 
      /*since dx,dy describe the tangent (not the normal*/
      /* corresponds to a threshold of 80 degrees */
    { *intersection = candidate;
      return 1;
    }
  }

  /* Look at intersection */
  /* X = mx * t + bx
   * Y = my * t + by
   * Where mx and my are the slope of the normal.
   *
   * Solve for t, iterating over target x:
   *    t <-- (X - bx )/mx
   * Compute Y as: 
   *    Y <-- my * t + by
   * Compute difference between target y and Y.
   * If this difference changes sign as one iterates over the target
   * then there was a hit.
   *
   * Since mx and my are for the normal. mx = -dy and my = dx = 1.
   */

  { float txmax = -FLT_MAX, txmin = FLT_MAX;
    for(i=tstart+1; i<target->len; i++)
    { tx = target->x[i];
      txmax = MAX(txmax, tx);
      txmin = MIN(txmin, tx);
    }

    if( dy==0 ) // go vertical
    { 
      if( ( txmin <= bx             ) && 
          ( bx <= txmax           ) )
      { //*intersection = bx - target->beg;
        for( i = tstart+1; i < target->len-1; i++ )
        { if( (target->x[i]-bx) * (target->x[i+1]-bx) < 0 )
          { *intersection = i;
            return 1;
          }
        }
        *intersection = i; //?? Nothing found, just return target end point?
        return 1;
      } else {
        *intersection = -1;
        return  0;
      }
    }
    tx = target->x[ tstart ];
    ty = target->y[ tstart ];
    y  = ( bx - tx )/dy + by; 
    sign = y-ty; 
    if( sign==0 )
    { *intersection = 0;
      return 1;
    }
    //tx = target->x[ ++tstart ]; //tx++;
    for( i=tstart+1; i < target->len; i++ )
    { tx = target->x[ i ];
      ty = target->y[ i ];
      y  = ( bx - tx )/dy + by; 
      if( (y-ty) * sign <= 0 ) //when (y-ty) and sign have different signs
      { *intersection = i;
        return 1;
      }
    }
  }

  /* No hit */
  *intersection = -1;
  return 0;
}

float compute_polygon_area( float *x, float *y, int n )
{ int i;
  float area = 0.0;
#ifdef DEBUG_COMPUTE_POLYGON_AREA
//  printf("--x--    --y--\n");
  printf("-----");
#endif
  for( i=0; i<(n-1); i++ )
  { area += x[i] * y[i+1] - x[i+1] * y[i];
#ifdef DEBUG_COMPUTE_POLYGON_AREA
//    printf("%#5.5g    %#5.5g : %g\n",x[i],y[i],area/2.0);
#endif
  }
  area += x[n-1] * y[0] - x[0] * y[n-1];
#ifdef DEBUG_COMPUTE_POLYGON_AREA
//    printf("%#5.5g    %#5.5g : %g\n",x[n-1],y[n-1],area/2.0);
    printf("\nx = [");
    for(i=0;i<n-1;i++) printf("%#5.5g,", x[i]);
    printf("%#5.5g]\n",x[n-1]);
    printf("y = [");
    for(i=0;i<n-1;i++) printf("%#5.5g,", y[i]);
    printf("%#5.5g]\n",y[n-1]);
#endif
  return area / 2.0;
}

float compute_area_between_whisker_by_polygon_safe_interval( Whisker_Seg *s, 
                                                              Whisker_Seg *t, 
                                                              int sa, int sb, 
                                                              int ta, int tb )
{ int i, n = (sb-sa+1) + (tb-ta+1); 
  //int sbeg = 0;//s->beg;
  //int tbeg = 0;//t->beg;
  float area = 0.0;
  if( n > 0 ) 
  { 
    float *x, *y;
    x = (float*) malloc( n*sizeof(float) );
    y = (float*) malloc( n*sizeof(float) );
    memcpy( x, s->x+sa, (sb-sa+1) * sizeof(float) );
    memcpy( y, s->y+sa, (sb-sa+1) * sizeof(float) );
    for(i = (sb-sa+1); i<n; i++)               /* i=(sb-sa+1) to  (sb-sa+1) + (tb-ta+1) */
    { x[i] = t->x[ ta + n - i - 1 ]; //tbeg + ta + n - i - 1;            /* x[i]   from tbeg + tb     to tbeg + ta     */
      y[i] = t->y[ ta + n - i - 1 ];       /* itrack from        tb     to        ta     */
    }                                        
    //{ x[i] = tbeg + ta + n - i - 1;            /* x[i]   from tbeg + tb     to tbeg + ta     */
    //  y[i] = t->track[ ta + n - i - 1 ];       /* itrack from        tb     to        ta     */
    //}                                        
    area = compute_polygon_area( x, y, n );
    free(x);
    free(y);
  }
  return fabs(area);
}

float compute_area_of_whisker_intersection( Whisker_Seg *s, 
                                             Whisker_Seg *t,
                                             int sa,int ta )
  /*     i        i+1
   *     a         c
   *     |.\     /.|   Measure the area where whiskers cross.
   *     |...\ /...|   1. Find p
   *     |.../p\...|   2. Calculate area of <abp and <cpd
   *     |./     \.|
   *     b         d
   */
{ float ax,bx,cx,dx,px;
  float ay,by,cy,dy,py;
  float m,delta,area = 0.0;
  float  *st,*tt;

  st = s->y + sa;
  tt = t->y + ta;

  ay = *st;
  by = *tt;
  dy = *(st+1);
  cy = *(tt+1);
  
  delta = ( dy - ay - cy + by ); 
  if ( delta==0 )
    return 0.0;
  m  = (by - ay) / delta;

  st = s->x + sa;
  tt = t->x + ta;       // * old *
  ax = *st;             //ax = s->beg + sa; 
  bx = *tt;             //bx = t->beg + ta; 
  dx = *(st+1);         //dx = ax + 1;      
  cx = *(tt+1);         //cx = bx + 1;      
  
  px = m*( dx - ax ) + ax;
  py = m*( dy - ay ) + ay;

  area = fabs( (ax-px)*(by-py) - (ay-py)*(bx-px)
             + (dx-px)*(cy-py) - (dy-py)*(cx-px) );
#ifdef DEBUG_COMPUTE_AREA_WHISKER_INTERSECTION
  printf("Area of whisker intersection at sa = %5d, ta = %5d\n",sa,ta);
  printf("\tTriangle abp\n");
  printf("\t\tx=[%g,%g,%g]\n",ax,bx,px);
  printf("\t\ty=[%g,%g,%g]\n",ay,by,py);
  printf("\tTriangle cpd\n");
  printf("\t\tx=[%g,%g,%g]\n",cx,px,dx);
  printf("\t\ty=[%g,%g,%g]\n",cy,py,dy);
  printf("\tArea: %g\n",area);
#endif
  return area;
}

float compute_segment_length( Whisker_Seg *w, int a, int b )
{ float  length = 0.0, dx, dy;
  int i, n = b-a;

  /* Note: dx == 1 */
  for( i=0; i<n; i++ )
  { dy = w->y[i+1] - w->y[i];
    dx = w->x[i+1] - w->x[i];
    length += hypot(dx,dy); //sqrt( dx*dx + dy*dy );
  }
  return length;
}

float d1_interp_linear( float *array, float idx, int direction )
{ float i;
  float remainder, b, d;
  remainder = modff( idx, &i );
  b = array[(int)i];
  d = array[(int)i+direction] - b;
  return b + d * remainder;
}

float compute_area_between_whiskers( Whisker_Seg *s, 
                                      Whisker_Seg  *t, 
                                      int sa, int sb, 
                                      int ta, int tb )
/* A (potentially self intersecting) polygon is constructed from the two whisker segments
 * by connecting the endpoints defined by `sa`, `sb`, `ta`, and `tb`.
 *
 * Distance is computed by interpolating the index intervals [sa,sb] and [ta,tb] so
 * that each interval is uniformly sampled.  Then the distance between sampling
 * points is averaged.
 *
 */
{ int n = MAX( sb-sa, tb-ta ) * 5;
  float dis = (sb-sa) / (float)n,        // step sizes
        dit = (tb-ta) / (float)n;
  float length, norm, d = 0.0;
  norm = n;
  do
  { float sx, sy, tx, ty;
    sx = d1_interp_linear( s->x + sa, dis*n, -1 ); 
    sy = d1_interp_linear( s->y + sa, dis*n, -1 ); 
    tx = d1_interp_linear( t->x + ta, dit*n, -1 ); 
    ty = d1_interp_linear( t->y + ta, dit*n, -1 ); 
    d += hypot( tx-sx, ty-sy );
  } while( --n );
  length = ( compute_segment_length(s,sa,sb) +          // HACK! this just gets divided out later
             compute_segment_length(t,ta,tb)   ) / 2.0; //       to fix need to rework...here for testing
  return length * d / norm;
}




float  compute_average_distance_between_whiskers( Whisker_Seg *s, Whisker_Seg *t, int sa, int sb, int ta, int tb)
{
#ifdef DEBUG_COMPUTE_AVERAGE_DISTANCE_BETWEEN_WHISKERS
  printf("sa: %4d sb: %4d ta: %4d tb: %4d\n",sa,sb,ta,tb);
#endif
  
  float area = compute_area_between_whiskers(s,t,sa,sb,ta,tb);
  float length = 0.0;
  int i;
  length = ( compute_segment_length(s,sa,sb) +
             compute_segment_length(t,ta,tb)   ) / 2.0;
#ifdef DEBUG_COMPUTE_AVERAGE_DISTANCE_BETWEEN_WHISKERS
  printf("area: %10.3g length: %10.3g\n",area,length);
#endif
  return area/length;
}

float pair_distance( Whisker_Seg *q, Whisker_Seg *t)
{ int beg, end;
  if( hit(q, t, 0, 0, &beg) )
  { if( hit(q,t,1, beg+1, &end) )
    { /*
       *   ---+--------+---    t
       *
       *      +----l---+       q
       *
       */
#ifdef  DEBUG_WHISKER_PAIR_DISTANCE
      printf("Case 1: beg = %d, end = %d\n",beg,end);
#endif
      return compute_average_distance_between_whiskers( q,t, 0, q->len-1, beg, end );
    } else if( hit(t,q,1,0,&end) )
    { /*
       *   ---+--------+       t
       *
       *      +----l---+----   q
       *
       */
#ifdef  DEBUG_WHISKER_PAIR_DISTANCE
      printf("Case 2: beg = %d, end = %d\n",beg,end);
#endif
      return compute_average_distance_between_whiskers( q,t, 0, end, beg, t->len-1);
    }
  } else if( hit(q,t,1,0,&end) )
  { if( hit(t,q,0,0,&beg) )
    { /*
       *   ---+----l---+       q
       *
       *      +--------+----   t
       *
       */
#ifdef  DEBUG_WHISKER_PAIR_DISTANCE
      printf("Case 3: beg = %d, end = %d\n",beg,end);
#endif
      return compute_average_distance_between_whiskers( q,t, beg, q->len -1 , 0, end );
    } 
  } else if( hit(t,q,0,0,&beg) )
  { if( hit(t,q,1,beg+1,&end) )
    { /*
       *   ---+----l---+---    q
       *
       *      +--------+       t
       *
       */
#ifdef  DEBUG_WHISKER_PAIR_DISTANCE
      printf("Case 4: beg = %d, end = %d\n",beg,end);
#endif
      return compute_average_distance_between_whiskers( q,t, beg, end, 0, t->len-1);
    }
  }
  return INFINITY;
}

void print_hits( int *m, int nrows, int ncols )
  /* Example: 
   *
   *   int m[rows][cols];
   *   ...
   *   print_hits(m, rows, cols);
   *
   */  
{ int i,j;
  int *row;
  for( i=0; i<nrows; i++ )
  { row = m + ncols*i; 
    printf("\trow: %p  ",row);
    for( j=0; j<ncols; j++ )
    { if( row[j] != -1 )
        printf("%4d ",row[j]);
      else
        printf("   . ");
    }
    printf("\n");
  }
}

void distance_matrix( Whisker_Seg **sv, int nsv, Whisker_Seg **tv, int ntv , float *result )
{ float *row;
  int i,j;
  Whisker_Seg *s, *t;

  for( i = 0; i < nsv; i++ )
  { row = result + i*ntv;
    s = sv[i];
    for( j = 0; j < ntv; j++ )
    { *(row+j) = pair_distance( s, tv[j] );
    }
  }
}

void **compat_win32_malloc_2d( int rows, int cols, int nbytes )
{ void **t = (void**) malloc( rows * sizeof(void*) );
  int i;  
  if(t)
    for(i=0; i<rows; i++)
      t[i] = malloc( cols * nbytes );
  return t;
}

void distance_matrix_cull( Whisker_Seg **sv, int nsv, Whisker_Seg **tv, int ntv, float *result ) 
  /* Heuristically, one might imagine that when trying to match whiskers only
   * nearest neighbors should be considered.  This adds a bit of overhead to
   * the distance computation, since nearest-neighbor testing and the distance
   * function are both linear in the total whisker length.  However, this might
   * be offset by better discrimination, which, hopefully, helps simplify the
   * overall matching problem.
   *
   * Can only close a quad between whisker segments s and t if one of the
   * following scenarios happens for hit-testing from the segments' endpoints:
   *
   *          GOOD                         BAD        
   *       s        t                  s        t                   
   *    -------   -------           -------   -------               
   *    beg end   beg end           beg end   beg end               
   *     1   1     _   _             1   0     1   0                
   *     1   _     _   1             0   1     0   1                
   *     _   1     1   _                                            
   *     _   _     1   1  
   *
   *     where _ can be anything, 1 is a hit, and zero is no hit.  Only,
   *     two-hit scenarios are considered for the `bad` cases.  The bad 
   *     cases are cases that don't result in a quad.  Notice that one
   *     needs at least one `beg` hit and at least one `end` hit.
   *                                                  
   * Hit testing from the endpoint of one whisker into a set of other whiskers
   * may yeild several hits, but really only the nearest-neighbor hits should
   * be considered. (What about crossed whiskers?) Drawing a line normal to the
   * querying segment's end, the nearest-neighbor hits are the first segments
   * intersecting the rays along that line starting at the query endpoint.
   * Hence, there can be at most two nearest neighbors for an endpoint.
   *
   * To determine nearest-neighbor hits, need the hit, distance and direction.
   * Every hit that isn't nearest neighbor can be eliminated.  
   *
   * Once only nearest neighbor hits are considered, then look for cases where
   * a quad can be formed and measure the distance using the quad.
   *
   *
   */
{ int i,j;
  float *r;
  int **hsb, **hse, **htb, **hte;
  float **d2m;
  hsb = (int**) compat_win32_malloc_2d(nsv,ntv, sizeof(int) );
  hse = (int**) compat_win32_malloc_2d(nsv,ntv, sizeof(int) );
  htb = (int**) compat_win32_malloc_2d(nsv,ntv, sizeof(int) );
  hte = (int**) compat_win32_malloc_2d(nsv,ntv, sizeof(int) );
  d2m = (float**) compat_win32_malloc_2d(nsv,ntv, sizeof(float) );
  
  /*
   * Compute hit matrices
   */
  for( i = 0; i < nsv; i++ )            /* Hit matrices get filled with:    */
  { Whisker_Seg     *s,*t;                  /*   on hit:  index of intersection */
    int *hsb_row = hsb[i];              /*  off hit:  -1                    */
    int *hse_row = hse[i];                                                    
    int *htb_row = htb[i];              /* transposes of hsb and hse        */
    int *hte_row = hte[i];              /*  after pruning they're different */
    s = sv[i];
    for( j = 0; j < ntv; j++ )          
    { t = tv[j];                        
      hit( s, t, 0, 0, hsb_row + j );   
      hit( t, s, 0, 0, htb_row + j );  

      if( *(hsb_row+j) > -1 )
        hit( s, t, 1, *(hsb_row+j), hse_row + j );
      else
        hit( s, t, 1, 0, hse_row + j ); 
      
      if( *(htb_row+j) > -1 )
        hit( t, s, 1, *(htb_row+j), hte_row + j );
      else
        hit( t, s, 1, 0, hte_row + j );  

      /* check */
      if( *(hsb_row+j) >= *(hse_row+j) )
        *(hse_row+j) = -1;
      if( *(htb_row+j) >= *(hte_row+j) )
        *(hte_row+j) = -1;
    }
  }

  /*
   * if hit, compute distance and direction to hit
   * only keep best
   */

  /* 
   * s beginings
   */
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS 
  printf("--- Pre  prune --- hsb %p\n",hsb);
  print_hits( (int*)hsb, nsv, ntv );
#endif

  for( i = 0; i < nsv; i++ )
  { Whisker_Seg     *s, *t;
    int *row = hsb[i];
    float ndy, dx, dy, dir, d2;
    float best_pos = INFINITY;
    float best_neg = INFINITY;
    int best_pos_idx = -1, best_neg_idx = -1;
    s = sv[i];
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS_HSB
    printf("*** %d\n",i);
#endif
    for( j = 0; j < ntv; j++ )
    { t = tv[j];
      if( row[j] > -1 )
      { //dx = (t->beg+row[j]) - s->beg;
        //dy = t->track[row[j]] - s->track[0];
        dx = t->x[row[j]] - s->x[0];
        dy = t->y[row[j]] - s->y[0];
        d2 = dx*dx + dy*dy;
        ndy = s->y[1] - s->y[0];
        /* cross product between tangent and vector to interesect. */
        /* this is the same as the dot product with the normal.    */ 
        dir = ndy*dx - dy; 
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS_HSB
        printf("\t--- (%d,%d)\n",i,j);
        printf("\td2: %g +%d -%d\n",d2, d2<best_pos, d2<best_neg);
        printf("\tNEG (%10.3g,%5d) dir: %g\n", best_neg, best_neg_idx, dir );
        printf("\tPOS (%10.3g,%5d) dir: %g\n", best_pos, best_pos_idx, dir );
#endif
        if( dir < 0.0 )
        { if( d2 < best_neg)
          { best_neg = d2;
            best_neg_idx = j;
          }
          d2m[i][j] = -d2;
        } else //if( dir>=0.0 )
        { if( d2 < best_pos)
          { best_pos = d2;
            best_pos_idx = j;
          }
          d2m[i][j] = d2;
        }
      }
    } /* finish search along row for best nearest neighbors */
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS_HSB 
    printf("NEG (%10.3g,%5d)\n", best_neg, best_neg_idx);
    printf("POS (%10.3g,%5d)\n\n", best_pos, best_pos_idx);
#endif
    /* eliminate all non-nearest neighbors */
    for( j = 0; j < ntv; j++ )
    { float d2 = d2m[i][j];
      if( d2 < 0.0 )
      { if( fabs( d2 + best_neg ) > 1.0 )
          row[j] = -1;
      } else {
        if( fabs( d2 - best_pos ) > 1.0 )
          row[j] = -1;
      }
    }
  }
  
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS 
  printf("--- Post prune --- hsb %p\n",hsb);
  print_hits( (int*) hsb, nsv, ntv );
#endif

  /* 
   * s ends     
   */
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS 
  printf("--- Pre  prune --- hse %p\n",hse);
  print_hits((int*)  hse, nsv, ntv );
#endif
  for( i = 0; i < nsv; i++ )
  { Whisker_Seg     *s, *t;
    int *row = hse[i];
    float ndy, dx, dy, dir, d2;
    float best_pos = INFINITY;
    float best_neg = INFINITY;
    int best_pos_idx = -1, best_neg_idx = -1;
    int iend;
    s = sv[i];
    iend = s->len-1; //s->end - s->beg;
    //printf("beg: %d   end: %d\n",s->beg, s->end);
    for( j = 0; j < ntv; j++ )
    { t = tv[j];
      if( row[j] > -1 )
      { //printf("(%d,%d) %d %d\n",i,j,row[j],iend);
        //dx = (t->beg + row[j]) - s->end;
        //dy = t->track[row[j]] - s->track[iend];
        dx = t->x[row[j]] - s->x[iend];
        dy = t->y[row[j]] - s->y[iend];
        d2 = dx*dx + dy*dy;
        ndy = s->y[ iend ] - s->y[ iend-1 ];
        dir = ndy*dx - dy; /* cross product between tangent and vector to interesect */
                           /* this is the same as the dot product with the normal. */
        if( dir<0.0 )
        { if( d2 < best_neg  )
          { best_neg = d2;
            best_neg_idx = j;
          }
          d2m[i][j] = -d2;
        } else //if( dir>0.0 )
        { if( d2 < best_pos  )
          { best_pos = d2;
            best_pos_idx = j;
          }
          d2m[i][j] = d2;
        }
      }
    } /* finish search along row for best nearest neighbors */
    /* eliminate all non-nearest neighbors */
    for( j = 0; j < ntv; j++ )
    { float d2 = d2m[i][j];
      if( d2 < 0.0 )
      { if( fabs( d2 + best_neg ) > 1.0 )
          row[j] = -1;
      } else {
        if( fabs( d2 - best_pos ) > 1.0 )
          row[j] = -1;
      }
    }
  }
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS 
  printf("--- Post prune --- hse %p\n",hse);
  print_hits((int*)  hse, nsv, ntv );
#endif

  /* 
   * t beginings
   */
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS 
  printf("--- Pre  prune --- htb %p\n",htb);
  print_hits((int*)  htb, nsv, ntv );
#endif
  for( i = 0; i < nsv; i++ )
  { Whisker_Seg *s, *t;
    int *row = htb[i];
    float ndy, dx, dy, dir, d2;
    float best_pos = FLT_MAX; 
    float best_neg = FLT_MAX; 
    int best_pos_idx = -1, best_neg_idx = -1;
    s = sv[i];
    for( j = 0; j < ntv; j++ )
    { t = tv[j];
      if( row[j] > -1 )
      { //dx = (s->beg+row[j]) - t->beg;
        //dy = s->track[row[j]] - t->track[0];
        dx = s->x[row[j]] - t->x[0];
        dy = s->y[row[j]] - t->y[0];
        d2 = dx*dx + dy*dy;
        ndy = t->y[1] - t->y[0];
        dir = ndy*dx - dy; /* cross product between tangent and vector to interesect */
                           /* this is the same as the dot product with the normal. */
        if( dir<0.0 )
        { if( best_neg > d2 )
          { best_neg = d2;
            best_neg_idx = j;
          }
          d2m[i][j] = -d2;
        } else //if( dir>0.0 )
        { if( best_pos > d2 )
          { best_pos = d2;
            best_pos_idx = j;
          }
          d2m[i][j] = d2;
        }
      }
    } /* finish search along row for best nearest neighbors */
    /* eliminate all non-nearest neighbors */
    for( j = 0; j < ntv; j++ )
    { float d2 = d2m[i][j];
      if( d2 < 0.0 )
      { if( fabs( d2 + best_neg ) > 1.0 )
          row[j] = -1;
      } else {
        if( fabs( d2 - best_pos ) > 1.0 )
          row[j] = -1;
      }
    }
  }
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS 
  printf("--- Post prune --- htb %p\n",htb);
  print_hits((int*)  htb, nsv, ntv );
#endif

  /* 
   * t ends     
   */
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS 
  printf("--- Pre  prune --- hte %p\n",hte);
  print_hits( (int*) hte, nsv, ntv );
#endif
  for( i = 0; i < nsv; i++ )
  { Whisker_Seg     *s, *t;
    int *row = hte[i];
    float ndy, dx, dy, dir, d2;
    float best_pos = INFINITY;
    float best_neg = INFINITY;
    int best_pos_idx = -1, best_neg_idx = -1;
    s = sv[i];
    for( j = 0; j < ntv; j++ )
    { int iend;
      t = tv[j];
      iend = t->len-1;//t->end - t->beg;
      if( row[j] > -1 )
      { //dx = (s->beg + row[j]) - t->end;
        //dy = s->track[row[j]] - t->track[iend];
        dx = s->x[row[j]] - t->x[iend];
        dy = s->y[row[j]] - t->y[iend];
        d2 = dx*dx + dy*dy;
        ndy = t->y[ iend ] - t->y[ iend-1 ];
        dir = ndy*dx - dy; /* cross product between tangent and vector to interesect */
                           /* this is the same as the dot product with the normal. */
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS_HTE
        printf("\t--- (%d,%d)\n",i,j);
        printf("\td2: %g +%d -%d\n",d2, d2<best_pos, d2<best_neg);
        printf("\tNEG (%10.3g,%5d) dir: %g\n", best_neg, best_neg_idx, dir );
        printf("\tPOS (%10.3g,%5d) dir: %g\n", best_pos, best_pos_idx, dir );
#endif
        
        if( dir<0.0 )
        { if( best_neg > d2 )
          { best_neg = d2;
            best_neg_idx = j;
          }
          d2m[i][j] = -d2;
        } else //if( dir>0.0 )
        { if( best_pos > d2 )
          { best_pos = d2;
            best_pos_idx = j;
          }
          d2m[i][j] = d2;
        }
      }
    } /* finish search along row for best nearest neighbors */
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS_HTE 
    printf("NEG (%10.3g,%5d)\n", best_neg, best_neg_idx);
    printf("POS (%10.3g,%5d)\n\n", best_pos, best_pos_idx);
#endif
    /* eliminate all non-nearest neighbors */
    for( j = 0; j < ntv; j++ )
    { float d2 = d2m[i][j];
      if( d2 < 0.0 )
      { if( fabs( d2 + best_neg ) > 1.0 )
          row[j] = -1;
      } else {
        if( fabs( d2 - best_pos ) > 1.0 )
          row[j] = -1;
      }
    }
  }
#ifdef DEBUG_WHISKER_NEAREST_NEIGHBORS 
  printf("--- Post prune --- hte %p\n",hte);
  print_hits( (int*) hte, nsv, ntv );
#endif

  /*
   * Look for appropriate hits and measure areas   
   * For the four cases see `pair_distance`
   * Length normalization not computed here, because I will probably 
   * incorporate a proper normalization in a replacement for 
   * `compute_area_between_whiskers`
   */

#ifndef DEBUG_WHISKER_NEAREST_NEIGHBORS
#define DEBUG_NN_PRINT(X)
#else
#define DEBUG_NN_PRINT(X) printf("(%3d,%3d) Case: %d\n",i,j,(X))
#endif

  
  for( i = 0; i < nsv; i++ )
  { Whisker_Seg     *s,*t;
    int *hsb_row = hsb[i];
    int *hse_row = hse[i];
    int *htb_row = htb[i];
    int *hte_row = hte[i];
    s = sv[i];
    for( j = 0; j < ntv; j++ )
    { t = tv[j];
      r = result + i*ntv + j; /* tested */
      if( (hsb_row[j] > -1) && (hse_row[j] > -1) )
      { DEBUG_NN_PRINT(1);  
        //*r = compute_average_distance_between_whiskers(s,t,0,s->end-s->beg,hsb_row[j],hse_row[j]);
        *r = compute_average_distance_between_whiskers(s,t,0,s->len,hsb_row[j],hse_row[j]);
      } else if( (hsb_row[j] > -1) && (hte_row[j] > -1) )
      {  DEBUG_NN_PRINT(2);  
         //*r = compute_average_distance_between_whiskers(s,t,0,hte_row[j], hsb_row[j], t->end-t->beg);
         *r = compute_average_distance_between_whiskers(s,t,0,hte_row[j], hsb_row[j], t->len);
      } else if( (hse_row[j] > -1) && (htb_row[j] > -1) )
      {  DEBUG_NN_PRINT(3);  
         //*r = compute_average_distance_between_whiskers(s,t,htb_row[j],s->end-s->beg,0,hse_row[j]);
         *r = compute_average_distance_between_whiskers(s,t,htb_row[j],s->len,0,hse_row[j]);
      } else if( (htb_row[j] > -1) && (hte_row[j] > -1) )
      {  DEBUG_NN_PRINT(4);  
         //*r = compute_average_distance_between_whiskers(s,t,htb_row[j],hte_row[j],0,t->end-t->beg);
         *r = compute_average_distance_between_whiskers(s,t,htb_row[j],hte_row[j],0,t->len);
      } else
      {   *r = INFINITY;
      }
      //printf( "(%4d,%4d) %g\n",i,j,*r );
    }
  } 

  free(hsb);
  free(hse);
  free(htb);
  free(hte);
  free(d2m);

}

/************************************************************** 
 * OLD STUFF
 **/

float d1( Whisker_Seg *a, Whisker_Seg *b )
{ int n = MAX( a->len, b->len ); // number of steps
  float dia = a->len / n,        // step sizes
        dib = b->len / n;
  float d = 0.0;
  do
  { float ax, ay, bx, by;
    ax = d1_interp_linear( a->x, dia*n, -1 ); 
    ay = d1_interp_linear( a->y, dia*n, -1 ); 
    bx = d1_interp_linear( b->x, dib*n, -1 ); 
    by = d1_interp_linear( b->y, dib*n, -1 ); 
    d += hypot( bx-ax, by-ay );
  } while( --n );
  return d;
}

float d4( Whisker_Seg_Old *w0, Whisker_Seg_Old *w1 )
/* measures average distance over intersection of domains */
{ float x0,x1,x2,x3,a,b,acc=0;
  float res;
  x0 = (float) w0->beg;
  x1 = (float) w1->beg;
  x2 = (float) w0->end;
  x3 = (float) w1->end;

  /* make [x1,x2] the interior interval */
  a = (x0<x1) ? x1 : x0; /* makes x1 the larger of the two beg  */
  b = (x2<x3) ? x2 : x3; /* makes x2 the smaller of the two end */
  if( b < a ) 
    return INFINITY;     /* hopefully this is large enough      */

  { int i,j;
    for(i=a-x0,j=a-x1; i < b-x0+1, j < b-x1+1; i++, j++)
      acc += fabs( w1->track[j] - w0->track[i] );
  }
  /*
  fprintf(stderr, "acc: %g\n",acc);
  fprintf(stderr, "a  : %g\n",a);
  fprintf(stderr, "b  : %g\n",b);
  */
  res = acc / (b-a+1);
  return res;
}

float whisker_distance( Whisker_Seg *w0, Whisker_Seg *w1 )
{ float dt, d;
  dt = (float) (w1->time - w0->time );
  if( dt <= 0 )
    return INFINITY;

  d = pair_distance(w0,w1) + dt;
  //d = d1(w0,w1) + dt;
  if( isnan(d) )
    fprintf(stderr, "warning: NaN returned from distance function.\n" );
  return d;
}

float whisker_distance_time_independent( Whisker_Seg *w0, Whisker_Seg *w1 )
{ //float d = d1(w0,w1); 
  float d = pair_distance(w0,w1);
  if( isnan(d) )
    fprintf(stderr, "warning: NaN returned from distance function.\n" );
  return d;
}
