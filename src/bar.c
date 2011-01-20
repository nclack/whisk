/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : May 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include "compat.h"
#include "image_lib.h"
#include "level_set.h"
#include "contour_lib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#undef  DEBUG_REGIONAL_MINIMA

/*
 * CIRCUMSCRIBED CIRCLES
 */

#define DET( a00, a01, a02, \
             a10, a11, a12, \
             a20, a21, a22) \
  ( (a00)*(a11)*(a22) - (a00)*(a12)*(a21) \
  + (a01)*(a12)*(a20) - (a01)*(a10)*(a22) \
  + (a02)*(a10)*(a21) - (a02)*(a11)*(a20) )

#define IMOD(a,b) ( ((a) + b*(1+(a)/(b)) )%(b) ) //argh...there's got to be a better way

int  Compute_Circumscribed_Circle(  double x0, double y0,
                                    double x1, double y1,  
                                    double x2, double y2,  
                                    double tol,
                                    double *x_result,
                                    double *y_result,
                                    double *rsq_result )
//  # see http://mathworld.wolfram.com/Circle.html eqs.28-34 2008-11-25
//  Returns 0 when computation is singular, otherwise returns 1.
{ double a,d,e,f;
  double r0,r1,r2;
  double t1,t2;

  r0 = x0*x0 + y0*y0;
  r1 = x1*x1 + y1*y1;
  r2 = x2*x2 + y2*y2;

  a =   DET( x0, y0, 1,
             x1, y1, 1,
             x2, y2, 1 );
  
  if( fabs(a) < tol )
    return 0;

  d = - DET( r0, y0, 1,
             r1, y1, 1,
             r2, y2, 1 );
  e =   DET( r0, x0, 1,
             r1, x1, 1,
             r2, x2, 1 );
  f = - DET( r0, x0, y0,
             r1, x1, y1,
             r2, x2, y2 );

  (*x_result) = -0.5 * d / a;
  (*y_result) = -0.5 * e / a;

  t1 = 0.25 * ( d*d + e*e ) / (a*a);
  t2 = f/a;
  (*rsq_result) = t1 - t2;

  return 1;
}
                                  
/*
 * LEVEL SETS
 */

typedef struct _bar_param
{ int width;
  int height;
  int gap;
  int minlen;
  int lvl_low;
  int lvl_high;
  double rsq_low;
  double rsq_high;
} bar_param;

int  bar_on_lvlset( Level_Set *self, 
                    unsigned int *result,
                    bar_param *parm )
// Returns 1 if the contour contrubuted to the histogram.
{ Contour *c; 
  int i,idx,len,lvl,res=0;
  double x0,y0,x1,y1,x2,y2;
  double xc  ,yc  ,rsq;
  int gap = parm->gap;
  int stride = parm->width; 
  int w,h, size;
  double rsq_low,rsq_high;
  double maxarea,minarea;

  rsq_low = parm->rsq_low;
  rsq_high = parm->rsq_high;

  maxarea = rsq_high*3.14159*2.0;
  minarea = rsq_low *3.14159/2.0;

  lvl = Level_Set_Level(self);
  size = Level_Set_Size(self);
//printf("level: %5d [%3d, %3d](%p) - size:%5d (%5g,%5g)\n",
//    lvl,
//    parm->lvl_low,
//    parm->lvl_high,
//    self,
//    size,
//    minarea-size,
//    maxarea-size);
  if( (lvl < parm->lvl_low)    
    ||(lvl > parm->lvl_high)  
    ||( (maxarea-size) < 0.0 )        
    ||( (minarea-size) > 0.0 )      
    )
    return 0;

  w = parm->width;
  h = parm->height;
  
  c = Trace_Level_Set( self );
  len = c->length;

  if( len > parm->minlen ) {
    res = 1;
    for( i = 0; i < c->length; i++ )
    { /* Sample contour */
      idx = c->tour[IMOD(i-gap,len)]; //(i-gap)%len];
      x0  = idx % stride;
      y0  = idx / stride;

      idx = c->tour[i];
      x1  = idx % stride;
      y1  = idx / stride;
      
      idx = c->tour[IMOD(i+gap,len)]; //(i+gap)%len];
      x2  = idx % stride;
      y2  = idx / stride;

      /* Get origin and radius of the circle circumscribed by the 3 points */
      if( Compute_Circumscribed_Circle( x0,y0,x1,y1,x2,y2, 1e-5, &xc, &yc, &rsq ))
      { if( xc  > 0       && 
            xc  < (w-0.5) && 
            yc  > 0       && 
            yc  < (h-0.5) && 
            rsq > rsq_low && 
            rsq < rsq_high ) 
        { //printf("\t(%5.5g, %5.5g) - %5.5g\tstride:%d\n",xc,yc,sqrt(rsq),stride);
          result[ lround(2.0*yc) * 2 * stride + lround(2.0*xc) ]++; 
        }
      }
    }
  }
  Free_Contour( c );
  return res;
}

void bar_lvlset_traverse( Level_Set *self, 
                          unsigned int *result,
                          bar_param *parm )
{ Level_Set *next; 
  
  bar_on_lvlset( self, result, parm ); 

  next = Level_Set_Sibling( self );
  if( next && (next != self) )
    bar_lvlset_traverse( next, result, parm );

  if( next == self )
    printf("*** Next == Self, sib\n");

  next = Level_Set_Child( self );
  if( next && (next != self) )
    bar_lvlset_traverse( next, result, parm );
  
  if( next == self )
    printf("*** Next == Self, chd\n");
  return;
}

SHARED_EXPORT
void Compute_Bar_Histogram( Image *im, 
                            unsigned int *result,
                            int gap,
                            int minlen,
                            int lvl_low,
                            int lvl_high,
                            double r_low,
                            double r_high )
{ Component_Tree *ctree;
  Level_Set *node;
  bar_param parm;

  parm.width    = im->width;
  parm.height   = im->height;
  parm.gap      = gap;
  parm.minlen   = minlen;
  parm.lvl_low  = lvl_low;
  parm.lvl_high = lvl_high;
  parm.rsq_low  = r_low*r_low;
  parm.rsq_high = r_high*r_high;

  ctree = Build_2D_Component_Tree( im, 0 );
  Set_Current_Component_Tree( ctree );
  node = Level_Set_Root();

  bar_lvlset_traverse( node, result, &parm );
  Free_Component_Tree( ctree );
  
  /* Eliminate hits where no neighbors were hit.
   * Use 4-connected neighbors.
   * Skip boundaries.
   **/
  { unsigned int *px;
    int i,npx = (im->width)*(im->height)*4;
    int stride = im->width * 2;
    for(px = result+stride; px < (result + npx - stride); px++)
    { if( *px )
      { //printf("%05d (%3d,%3d) = %4d\n", 
        //                px-result, 
        //                (px-result)%stride,
        //                (px-result)/stride,
        //                *px);
        i = ( px - result )%stride;
        if( i>0 && i<(stride-1) )   // border check
        { if( (*( px-stride ) <  5) // neighborhood check
            &&(*( px-1      ) <  5)
            &&(*( px+1      ) <  5)
            &&(*( px+stride ) <  5) )
          { (*px) = 0;  
            //printf("\t*** KILLED\n");
          }
        }
      }
    }
  }
  return;
}

SHARED_EXPORT
void Compute_Bar_Location(  Image *im, 
                            double *x,
                            double *y,
                            int gap,
                            int minlen,
                            int lvl_low,
                            int lvl_high,
                            double r_low,
                            double r_high )  
{ static unsigned int *histogram, max;
  static int maxlen = 0;
  int i, best, npx = (im->width)*(im->height)*4;
  int carea = sizeof(unsigned int)*npx;
  int stride = im->width * 2;

  if( maxlen < carea )
  { histogram = (unsigned int*) Guarded_Malloc( carea, "Compute Bar Location" );
    maxlen = carea;
  }

  memset( histogram, 0, carea );
  Compute_Bar_Histogram( im, histogram, 
                         gap, minlen, 
                         lvl_low, lvl_high, 
                         r_low, r_high);


  /* compute max */
  best = 0;
  max = 0;
  for(i=0; i<npx; i++)
  { if( histogram[i] > max )
    { int tx = (i % stride) / 2;
      int ty = (i / stride) / 2;
      int idx = ty * stride / 2 + tx;
      if( (im->array[idx] > lvl_low) && (im->array[idx] < lvl_high) )
      { max = histogram[i];
        best = i;
      }
    }
  }

  /* make an attempt at subpixel precision */
  { int offsets[25];// = { -stride-1, -stride, -stride+1,
                    //            -1,       0,         1,
                    //      stride-1,  stride,  stride+1 };
    int j, xo, yo;
    unsigned int v;
    double xc= 0.0, yc= 0.0, sum = 0.0;

    for( i=0; i<5; i++)
      for( j=0; j<5; j++ )
        offsets[ j*5 + i ] = i - 2 + (j-2) * stride;

    for( i=0; i<25; i++)
    { j = best + offsets[i];
      xo = j%stride;
      yo = j/stride;
      if( xo<0 ) continue;
      if( yo<0 || yo*stride >= npx ) continue;
      v = histogram[j];
      xc+= ( j % stride ) * v;
      yc+= ( j / stride ) * v;
      sum += v;
    }
    (*x) = xc / sum / 2.0;
    (*y) = yc / sum / 2.0;
  }

  return;
}

/*
 * REGIONAL MINIMA
 */

int iter_regional_minima( float *array, int width, int height, int bookmark )
{ int i,j,flag=0;
  float *c;
  int stride = height;
  int offsets[] = { -stride -1, -stride  , -stride +1, 
                            -1,                     1,
                     stride -1,  stride  ,  stride +1 };

  for( i = bookmark+1; i < (width*height - stride); i++ )
  { if( (i%stride >= 1)         &&   /* don't do borders */
        (i%stride) < (stride-1) &&
        i > stride )
    {
      flag = 0;
      c = array + i;
#ifdef DEBUG_REGIONAL_MINIMA
      printf("* %p\n",c);
      printf("- (%3d, %3d): %5.5g\n",i%stride,i/stride,*c);
#endif
      for( j=0; j<8; j++ )
      { if( *(c + offsets[j]) <= *c )
          flag = 1;
#ifdef DEBUG_REGIONAL_MINIMA
        printf("\t(%3d, %3d): %5.5g\n",(i+offsets[j])%stride,
                                       (i+offsets[j])/stride,
                                      *(c+offsets[j]) );
#endif
      }
      if( flag==0 )
        return i;
    }
  }
  return -1;
}

