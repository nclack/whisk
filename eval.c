/* In 2d: For a closed non-intersecting oriented parametric curve, compute the
 * overlap between that curve and each pixel in a pixel grid.
 *
 * The parametric curve is represented by a list of 2d vertices organized to
 * preserve orientation.  The curve is linearly interpolated between vertices.
 */
#include "compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES 
#include <math.h>
#include <float.h>
#include "eval.h"
#include "aip.h"
#include "utilities.h"



#if 0
#define DEBUG_READ_RANGE
#define DEBUG_READ_ARRAY
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

void Print_Range( FILE *fp, Range *r)
{ fprintf(fp, "Range: From %5.5g to %5.5g by %g\n", r->min, r->max, r->step);
  fflush(fp);
}

void  Read_Range( FILE *fp, Range *r )
{ int n = 0;
#ifdef DEBUG_READ_RANGE
  progress("READ RANGE: fp = %p\n"
           "             r = %p\n"
           " sizeof(Range) = %d\n", fp, r, sizeof(Range) );
  progress("errno: %d\n",errno);
  progress("ftell: %d\n",ftell(fp));
  progress("feof : %d\n",feof(fp));
#endif
#if 1 
  n = fread( r, sizeof(Range), 1, fp );  
#else
  r->min = 0.0; r->max = 0.0; r->step = 0.0;
  fseek(fp,sizeof(Range),SEEK_CUR);
  progress("errno: %d\n",errno);
#endif
#ifdef DEBUG_READ_RANGE
  progress("READ RANGE (done): Read %d items.\n",n);
#endif
}

void Write_Range( FILE *fp, Range *r )
{ //printf("fwrite: %p, %d, %d, %p\n",r,sizeof(Range),1,fp);
  fwrite( r, sizeof(Range), 1, fp );
}

Array *Make_Array( int *shape , int ndim, int bytesperpixel )
{ int i = ndim;
  Array    *a       = (Array   *) Guarded_Malloc ( sizeof(Array   ), "array struct" );
  a->ndim           = ndim;
  a->shape          = (     int*) Guarded_Malloc ( ndim     * sizeof(int), "array shape" );
  a->strides_bytes  = (     int*) Guarded_Malloc ( (ndim+1) * sizeof(int), "array strides bytes" );
  a->strides_px     = (     int*) Guarded_Malloc ( (ndim+1) * sizeof(int), "array strides px" );
  a->strides_bytes[ndim] = bytesperpixel;
  a->strides_px[ndim] = 1;

  while(i--)                                            // For shape = (w,h,d):
  { a->strides_bytes[i] = a->strides_bytes[i+1] * shape[ndim-1 - i];//   strides = (whd, wh, w, 1)
    a->strides_px[i] = a->strides_bytes[i] / bytesperpixel;   
    a->shape[i]   = shape[i];
  }
  a->data = Guarded_Malloc( a->strides_bytes[0],"array data" );
  return a;
}

Array *Read_Array( FILE *fp )
{ Array    *a;
  int ndim;
  //printf("\there sizeof(Array) is %d\n",sizeof(Array)); fflush(stdout);
  a       = (Array   *) Guarded_Malloc ( sizeof(Array   ), "array struct" );
#ifdef DEBUG_READ_ARRAY
  progress("\tArray: %p\n",a);
#endif
  fread( &ndim,    sizeof(int), 1,   fp);
#ifdef DEBUG_READ_ARRAY
  progress("\tndim: %d\n",ndim);
#endif
  a->ndim = ndim;

  a->shape          = (     int*) Guarded_Malloc ( ndim     * sizeof(int), "array shape" );
  a->strides_bytes  = (     int*) Guarded_Malloc ( (ndim+1) * sizeof(int), "array strides bytes" );
  a->strides_px     = (     int*) Guarded_Malloc ( (ndim+1) * sizeof(int), "array strides px" );

  fread( a->shape,         sizeof(int), ndim,       fp);
  fread( a->strides_bytes, sizeof(int), ndim+1,     fp);
  fread( a->strides_px   , sizeof(int), ndim+1,     fp);
#ifdef DEBUG_READ_ARRAY
  progress("\tnbytes: %d\n",a->strides_bytes[0]);
#endif
  a->data = Guarded_Malloc( a->strides_bytes[0],"array data" );

  { int nitems;
    nitems = fread( a->data, 1, a->strides_bytes[0], fp );
    if( nitems != a->strides_bytes[0] )
    { error( "Incorrect number of bytes read. Got %d. Expected %d\n"
             "\t ferror = %d\tfeof = %d\n",
             nitems, a->strides_bytes[0], ferror(fp), feof(fp) );
    }
  }

  return a;
}

void Write_Array(FILE *fp, Array *a)
{ fwrite( &a->ndim, sizeof(int),            1,       fp );
  fwrite( a->shape, sizeof(int),         a->ndim,    fp );
  fwrite( a->strides_bytes, sizeof(int), a->ndim+1,  fp ); 
  fwrite( a->strides_px, sizeof(int),    a->ndim+1,  fp ); 
  fwrite( a->data, 1, a->strides_bytes[0]          , fp );
}

void Free_Array( Array *a)
{ free( a->shape   );
  free( a->strides_bytes );
  free( a->strides_px );
  free( a->data    );
  free(a);
}

inline void pixel_to_vertex_array(int p, int stride, float *v)
{ float x = p%stride;
  float y = p/stride;
  v[0] = x;      v[1] = y;
  v[2] = x+1.0f; v[3] = y;
  v[4] = x+1.0f; v[5] = y+1.0f;
  v[6] = x;      v[7] = y+1.0f;
  return;
}

inline unsigned array_max_f32u ( float *buf, int size, int step, float bound )
{ float *t = buf + size;
  float r = 0.0;       
  while( (t -= step) >= buf )
    r = MAX( r, ceil(*t) );
  return (unsigned) MIN(r,bound);
}

inline unsigned array_min_f32u ( float *buf, int size, int step, float bound )
{ float *t = buf + size;
  float r = FLT_MAX;
  while( (t -= step) >= buf )
    r = MIN( r, floor(*t) );
  return (unsigned) MAX(r,bound);
}

void rotate( point *pbuf, int n, float angle)
  /* positive angle rotates counter clockwise */
{ float s = sin(angle),
        c = cos(angle);
  point *p = pbuf + n;
  while( p-- > pbuf )
  { float x = p->x,
          y = p->y;
    p->x =  x*c - y*s;
    p->y =  x*s + y*c;
  }
}

void scale( point* pbuf, int n, float sc)
{ point *p = pbuf + n;
  while( p-- > pbuf )
  { p->x =  p->x * sc;
    p->y =  p->y * sc;
  }
  return;
}

void translate( point* pbuf, int n, point ori)
{ point *p = pbuf + n;
  while( p-- > pbuf )
  { p->x =  p->x + ori.x;
    p->y =  p->y + ori.y;
  }
  return;
}

void Sum_Pixel_Overlap( float *xy, int n, float gain, float *grid, int *strides )
/* `xy`      is an array of `n` vertices arranged in (x,y) pairs.
 * `n`       the number of vertices in `xy`.
 * `grid`    should point to the origin pixel in an image buffer in the
 *           channel of interest.
 * `strides` should be an array of size ndim+1 (for 2d, size=3)
 *           with { width*height*channels, width*channels, channels }
 *           For now assume channels == 1.
 */
{
  /*
   * 1. Find which pixels are involved (simplest: all of them).
   *    a. restrict to bbox
   *    b. try to only do the expensive stuff on "interesting pixels"
   *       i.e. pixels the line crosses.
   *       The others are either all the way inside or all the way outside
   *       which a quick(?) even-odd rule test can check
   *    c. A more interesting approach would be to generalize the intersect
   *       computation for arbitrary tiling/subdivisions of the plane.
   *       This would reduce duplicate effort on shared edges.
   * 2. Convert pixels into paramterized polygons (4-tuples of vertices)
   *                      4-tuples of vertices    - simplest
   * 3. For each pixel, compute intersection area
   */
  float pxverts[8];
  int px,ix,iy;
  unsigned minx, maxx, miny, maxy;

  // Compute AABB
  minx = array_min_f32u( xy  , 2*n, 2, 0.0                      ); 
  maxx = array_max_f32u( xy  , 2*n, 2, strides[1]-1             ); 
  miny = array_min_f32u( xy+1, 2*n, 2, 0.0                      ); 
  maxy = array_max_f32u( xy+1, 2*n, 2, strides[0]/strides[1] - 1); 
  
  for( ix = minx; ix <= maxx; ix++ )
  { for( iy = miny; iy <= maxy; iy ++ )
    { px = iy*strides[1] + ix;
      pixel_to_vertex_array( px, strides[1], pxverts );
      *(grid+px) += gain * inter( (point*) xy, n, (point*) pxverts, 4 );
    }
  }
  return;
}

void Label_Pixel_Overlap( float *xy, int n, float gain, float *grid, int *strides )
/* `xy`      is an array of `n` vertices arranged in (x,y) pairs.
 * `n`       the number of vertices in `xy`.
 * `grid`    should point to the origin pixel in an image buffer in the
 *           channel of interest.
 * `strides` should be an array of size ndim+1 (for 2d, size=3)
 *           with { width*height*channels, width*channels, channels }
 *           For now assume channels == 1.
 */
{ float pxverts[8];
  int px,ix,iy;
  unsigned minx, maxx, miny, maxy;

  // Compute AABB
  minx = array_min_f32u( xy  , 2*n, 2, 0.0                      ); 
  maxx = array_max_f32u( xy  , 2*n, 2, strides[1]-1             ); 
  miny = array_min_f32u( xy+1, 2*n, 2, 0.0                      ); 
  maxy = array_max_f32u( xy+1, 2*n, 2, strides[0]/strides[1] - 1); 
  
  // add gain to pixels dominated by the poly
  for( ix = minx; ix <= maxx; ix++ )
  { for( iy = miny; iy <= maxy; iy ++ )
    { float v,w;
      int lbl;
      px = iy*strides[1] + ix;
      v = *(grid+px);
      lbl = lround(v);
      w = v - lbl;
      pixel_to_vertex_array( px, strides[1], pxverts );
      if( ( fabs( v ) > 1e-5                          ) &&
          ( inter( (point*) xy, n, (point*) pxverts, 4 ) > 0.5 ) )
      { if(lbl)
        {  *(grid+px) = w + gain*lbl;
        } else {
          *(grid+px) = w + gain;
        }
      }
    }
  }

  // ignore everything outside of the AABB
  return;
}

void Multiply_Pixel_Overlap( float *xy, int n, float gain, float boundary, float *grid, int *strides )
/* `xy`      is an array of `n` vertices arranged in (x,y) pairs.
 * `n`       the number of vertices in `xy`.
 * `grid`    should point to the origin pixel in an image buffer in the
 *           channel of interest.
 * `strides` should be an array of size ndim+1 (for 2d, size=3)
 *           with { width*height*channels, width*channels, channels }
 *           For now assume channels == 1.
 */
{ float pxverts[8];
  int px,ix,iy;
  unsigned minx, maxx, miny, maxy;

  // Compute AABB
  minx = array_min_f32u( xy  , 2*n, 2, 0.0                      ); 
  maxx = array_max_f32u( xy  , 2*n, 2, strides[1]-1             ); 
  miny = array_min_f32u( xy+1, 2*n, 2, 0.0                      ); 
  maxy = array_max_f32u( xy+1, 2*n, 2, strides[0]/strides[1] - 1); 
  
  // multiply by overlaps
  for( ix = minx; ix <= maxx; ix++ )
  { for( iy = miny; iy <= maxy; iy ++ )
    { px = iy*strides[1] + ix;
      pixel_to_vertex_array( px, strides[1], pxverts );
      *(grid+px) *= gain * inter( (point*) xy, n, (point*) pxverts, 4 );
    }
  }

  // everything outside of the AABB gets multiplied times boundary
  { float *p;
    for( iy = 0; iy < strides[0]/strides[1]; iy++ )
    { p = grid + iy*strides[1];
      for( ix = 0; ix < strides[1]; ix++ )
        if( (ix<minx) || (maxx<ix) ||
            (iy<miny) || (maxy<iy) )
          *(p + ix) *= boundary;
    }
  }
  return;
}

void Multiply_Gaussian_Along_Direction( float x0, float y0, float vx, float vy, float *grid, int *strides )
{ int px, ix, iy;
  const float norm = 1.0/sqrt( M_PI*2.0 );
  px = strides[0];
  while(px--)
  { float w,l;
    ix = px % strides[1];
    iy = px / strides[1];
    l = (ix - x0)*vx + (iy - y0)*vy; // projection along v - v in units of sigma [px/sigma]
    w = norm * exp( - l*l / 2.0 );
    grid[px] *= w;
  }
}

/* Anchor is in the center */
inline void Simple_Line_Primitive( point *verts, point offset, float length, float thick )
{ point v0 = { offset.x - length,  offset.y - thick},
        v1 = { offset.x + length,  offset.y - thick},
        v2 = { offset.x + length,  offset.y + thick},
        v3 = { offset.x - length,  offset.y + thick};
  verts[0] = v0;
  verts[1] = v1;
  verts[2] = v2;
  verts[3] = v3;
}


void Simple_Circle_Primitive( point *verts, int npoints, point center, float radius, int direction)
{ int i = npoints;
  float k = direction*2*M_PI/(float)npoints;
  while(i--)
  { point p = { center.x + radius * cos( k*i ),
                center.y + radius * sin( k*i )};
    verts[i] = p;
  }
}

void Render_Line_Detector( float offset, 
                           float length, 
                           float angle, 
                           float width,
                           point anchor,
                           float *image, int *strides  )
/*
 * `strides` should be an array of size ndim+1 (for 2d, size=3)
 *           with { width*height*channels, width*channels, channels }
 *           For now assume channels == 1.
 *  `anchor` The center is at anchor.
 */
{ point prim[4];
  const float thick = 0.7;
  const float r = 1.0;
  //const float area = 4*thick*length;
  //length -=2;

  { point off = { 0.0, offset + width/2.0 + r*thick/2.0   };
    Simple_Line_Primitive(prim, off, length, r*thick   );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4, -1.0/r, image, strides );
  }
  { point off = { 0.0, offset + width/2.0 - thick/2.0 };
    Simple_Line_Primitive(prim, off, length/r, thick );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4,  r, image, strides );
  }
  { point off = { 0.0, offset - width/2.0 + thick/2.0 }; 
    Simple_Line_Primitive(prim, off, length/r, thick );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4,  r, image, strides );
  }
  { point off = { 0.0, offset - width/2.0 - r*thick/2.0  };
    Simple_Line_Primitive(prim, off, length, r*thick   );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4, -1.0/r, image, strides );
  }

  //Multiply_Gaussian_Along_Direction( strides[1]/2            - offset * sin(angle), 
  //                                  strides[0]/strides[1]/2 + offset * cos(angle),
  //                                  2.0*cos(angle)/length,
  //                                  2.0*sin(angle)/length, 
  //                                  image, strides );

  return;
}

void Render_Curved_Line_Detector( float offset, 
                                  float length, 
                                  float angle, 
                                  float width,
                                  float radius_of_curvature,
                                  point anchor,
                                  float *image, int *strides  )
/*
 * `strides` should be an array of size ndim+1 (for 2d, size=3)
 *           with { width*height*channels, width*channels, channels }
 *           For now assume channels == 1.
 *  `anchor` The center is at anchor.
 */
{ point prim[32];
  const float thick = 1.0;
  const float r = 2.0;
  const int nv =  32;
  //const float area = 4*thick*length;
  //length -=2;

  { point off = { 0.0, radius_of_curvature + offset + width/2.0 - thick };
    Simple_Circle_Primitive( prim, nv, off, radius_of_curvature, 1 );
    rotate( prim,nv, angle );
    translate( prim,nv, anchor );
    Sum_Pixel_Overlap( (float*) prim,nv,  r, image, strides );
  }
  { point off = { 0.0, radius_of_curvature + offset + width/2.0    };
    Simple_Circle_Primitive( prim, nv, off, radius_of_curvature, 1 );
    rotate( prim,nv, angle );
    translate( prim,nv, anchor );
    Sum_Pixel_Overlap( (float*) prim,nv, -2*r, image, strides );
  }
  { point off = { 0.0, radius_of_curvature + offset + width/2.0 + thick   };
    Simple_Circle_Primitive( prim, nv, off, radius_of_curvature, 1 );
    rotate( prim,nv, angle );
    translate( prim,nv, anchor );
    Sum_Pixel_Overlap( (float*) prim,nv, r, image, strides );
  }

  { point off = { 0.0, -radius_of_curvature + offset - width/2.0 + thick }; 
    Simple_Circle_Primitive( prim, nv, off, radius_of_curvature, 1 );
    rotate( prim,nv, angle );
    translate( prim,nv, anchor );
    Sum_Pixel_Overlap( (float*) prim,nv,  r, image, strides );
  }
  { point off = { 0.0, -radius_of_curvature + offset - width/2.0  };
    Simple_Circle_Primitive( prim, nv, off, radius_of_curvature, 1 );
    rotate( prim,nv, angle );
    translate( prim,nv, anchor );
    Sum_Pixel_Overlap( (float*) prim,nv, -2*r, image, strides );
  }
  { point off = { 0.0, -radius_of_curvature + offset - width/2.0 - thick }; 
    Simple_Circle_Primitive( prim, nv, off, radius_of_curvature, 1 );
    rotate( prim,nv, angle );
    translate( prim,nv, anchor );
    Sum_Pixel_Overlap( (float*) prim,nv,  r, image, strides );
  }

//{ point prim[12];
//  int npoint = 12;
//  point off = { 0.0, offset };
//  Simple_Circle_Primitive(prim,npoint,off,length, 1);
//  rotate( prim, npoint, angle );
//  translate( prim, npoint, anchor );
//  Multiply_Pixel_Overlap( (float*) prim, npoint, 1, 0, image, strides );
//}
  Multiply_Gaussian_Along_Direction( strides[1]/2            - offset * sin(angle), 
                                    strides[0]/strides[1]/2 + offset * cos(angle),
                                    2.5*cos(angle)/length,
                                    2.5*sin(angle)/length, 
                                    image, strides );
  
  return;
}

void Render_Harmonic_Line_Detector( float offset, 
                                    float length, 
                                    float angle, 
                                    float width,
                                    point anchor,
                                    float *image, int *strides  )
/*
 * `strides` should be an array of size ndim+1 (for 2d, size=3)
 *           with { width*height*channels, width*channels, channels }
 *           For now assume channels == 1.
 *  `anchor` The center is at anchor.
 */
{ point prim[4];
  const float thick = 0.5;
  const float r = 1.0;
  const float mag = 0.1;
  //const float area = 4*thick*length;
  //length -=2;

  // basic shapes
  { point off = { 0.0, offset + width/2.0 + r*thick/2.0   };
    Simple_Line_Primitive(prim, off, length, r*thick   );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4, -mag/r, image, strides );
  }
  { point off = { 0.0, offset + width/2.0 - thick/2.0 };
    Simple_Line_Primitive(prim, off, length/r, thick );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4,  r*mag, image, strides );
  }
  { point off = { 0.0, offset - width/2.0 + thick/2.0 }; 
    Simple_Line_Primitive(prim, off, length/r, thick );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4,  r*mag, image, strides );
  }
  { point off = { 0.0, offset - width/2.0 - r*thick/2.0  };
    Simple_Line_Primitive(prim, off, length, r*thick   );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4, -mag/r, image, strides );
  }

  // Label columns w. primes
  { point off = { 0.50*length, offset   };
    Simple_Line_Primitive(prim, off, 0.5*length+1, width + 2.0*r*thick + 1 );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Label_Pixel_Overlap( (float*) prim, 4, 2.0, image, strides );
  }
//{ point off = { 0.25*length, offset   };
//  Simple_Line_Primitive(prim, off, 0.5*length, width + 2.0*r*thick );
//  rotate( prim, 4, angle );
//  translate( prim, 4, anchor );
//  Label_Pixel_Overlap( (float*) prim, 4, 3.0, image, strides );
//}
  { point off = {-0.50*length, offset   };
    Simple_Line_Primitive(prim, off, 0.5*length+1, width + 2.0*r*thick + 1 );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Label_Pixel_Overlap( (float*) prim, 4, 3.0, image, strides );
  }
//{ point off = {-0.75*length, offset   };
//  Simple_Line_Primitive(prim, off, 0.5*length, width + 2.0*r*thick );
//  rotate( prim, 4, angle );
//  translate( prim, 4, anchor );
//  Label_Pixel_Overlap( (float*) prim, 4, 7.0, image, strides );
//}
    
  return;
}

void Render_Line_Detector_v0( float offset, 
                           float length, 
                           float angle, 
                           float width,
                           point anchor,
                           float *image, int *strides  )
/*
 * `strides` should be an array of size ndim+1 (for 2d, size=3)
 *           with { width*height*channels, width*channels, channels }
 *           For now assume channels == 1.
 *  `anchor` The center is at anchor.
 */
{ point prim[4];
  const float thick = 1.0;

  { point off = { 0.0, offset + width/2.0 + thick/2.0 /*+ thick*/   };
    Simple_Line_Primitive(prim, off, length, thick   );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4, -1.0, image, strides );
  }
  { point off = { 0.0, offset + width/2.0 - thick/2.0 };
    Simple_Line_Primitive(prim, off,     length, thick );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4,  1.0, image, strides );
  }
  { point off = { 0.0, offset - width/2.0 + thick/2.0 }; 
    Simple_Line_Primitive(prim, off,     length, thick );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4,  1.0, image, strides );
  }
  { point off = { 0.0, offset - width/2.0 -     thick/2.0 /*- thick */  };
    Simple_Line_Primitive(prim, off, length,     thick   );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4, -1.0, image, strides );
  }
  return;
}

inline int compute_number_steps( Range *r )
{  return lround( (r->max - r->min) / r->step ) + 1;
}

inline float *Get_Line_Detector( Array *bank, int ioffset, int iwidth, int iangle  )
{ return ((float*) (bank->data)) + iangle  * bank->strides_px[1] 
                                 + iwidth  * bank->strides_px[2] 
                                 + ioffset * bank->strides_px[3]; 
}

Array *Build_Line_Detectors( Range off, 
                             Range wid, 
                             Range ang, 
                             float length, 
                             int supportsize )
{ Array *bank;
  int noff =  compute_number_steps( &off ),
      nwid =  compute_number_steps( &wid ),
      nang =  compute_number_steps( &ang );
  int shape[5] = { supportsize,
                   supportsize,
                   noff,
                   nwid,
                   nang};
  bank = Make_Array( shape, 5, sizeof(float) );
  memset( bank->data, 0, bank->strides_bytes[0] );

  { int    o,a,w;
    for( o = 0; o < noff; o++ )
    { //point anchor = {supportsize/2.0, o*off.step + off.min + supportsize/2.0};
      point anchor = {supportsize/2.0, supportsize/2.0};
      for( a = 0; a < nang; a++ )
        for( w = 0; w < nwid; w++ )
//        Render_Curved_Line_Detector(
//            o*off.step + off.min,                       //offset (before rotation)
//            length,                                     //length,
//            a*ang.step + ang.min,                       //angle,
//            w*wid.step + wid.min,                       //width,
//            3*length,                                   //radius of curvature
//            anchor,                                     //anchor,
//            Get_Line_Detector( bank, o,w,a),            //image
//            bank->strides_px + 3);                      //strides
          Render_Line_Detector(
              o*off.step + off.min,                       //offset (before rotation)
              length,                                     //length,
              a*ang.step + ang.min,                       //angle,
              w*wid.step + wid.min,                       //width,
              anchor,                                     //anchor,
              Get_Line_Detector( bank, o,w,a),            //image
              bank->strides_px + 3);                      //strides
    }
  }

  return bank;
}

Array *Build_Curved_Line_Detectors( Range off, 
                                    Range wid, 
                                    Range ang, 
                                    float length, 
                                    int supportsize )
{ Array *bank;
  int noff =  compute_number_steps( &off ),
      nwid =  compute_number_steps( &wid ),
      nang =  compute_number_steps( &ang );
  int shape[5] = { supportsize,
                   supportsize,
                   noff,
                   nwid,
                   nang};
  bank = Make_Array( shape, 5, sizeof(float) );
  memset( bank->data, 0, bank->strides_bytes[0] );

  { int    o,a,w;
    for( o = 0; o < noff; o++ )
    { //point anchor = {supportsize/2.0, o*off.step + off.min + supportsize/2.0};
      point anchor = {supportsize/2.0, supportsize/2.0};
      for( a = 0; a < nang; a++ )
        for( w = 0; w < nwid; w++ )
          Render_Curved_Line_Detector(
              o*off.step + off.min,                       //offset (before rotation)
              length,                                     //length,
              a*ang.step + ang.min,                       //angle,
              w*wid.step + wid.min,                       //width,
              3*length,                                   //radius of curvature
              anchor,                                     //anchor,
              Get_Line_Detector( bank, o,w,a),            //image
              bank->strides_px + 3);                      //strides
    }
  }

  return bank;
}


Array *Build_Harmonic_Line_Detectors( Range off, 
                                      Range wid, 
                                      Range ang, 
                                      float length, 
                                      int supportsize )
{ Array *bank;
  int noff =  compute_number_steps( &off ),
      nwid =  compute_number_steps( &wid ),
      nang =  compute_number_steps( &ang );
  int shape[5] = { supportsize,
                   supportsize,
                   noff,
                   nwid,
                   nang};
  bank = Make_Array( shape, 5, sizeof(float) );
  memset( bank->data, 0, bank->strides_bytes[0] );

  { int    o,a,w;
    for( o = 0; o < noff; o++ )
    { //point anchor = {supportsize/2.0, o*off.step + off.min + supportsize/2.0};
      point anchor = {supportsize/2.0, supportsize/2.0};
      for( a = 0; a < nang; a++ )
        for( w = 0; w < nwid; w++ )
          Render_Harmonic_Line_Detector(
              o*off.step + off.min,                       //offset (before rotation)
              length,                                     //length,
              a*ang.step + ang.min,                       //angle,
              w*wid.step + wid.min,                       //width,
              anchor,                                     //anchor,
              Get_Line_Detector( bank, o,w,a),            //image
              bank->strides_px + 3);                      //strides
    }
  }

  return bank;
}

void Render_Half_Space_Detector( float offset, 
                                 float length, 
                                 float angle, 
                                 float width,
                                 point anchor,
                                 float *image, int *strides  )
/*
 * `strides` should be an array of size ndim+1 (for 2d, size=3)
 *           with { width*height*channels, width*channels, channels }
 *           For now assume channels == 1.
 *  `anchor` The center is at anchor.
 */
{ 
  float thick = length;
  float density = 1.0f;

  { point prim[4];
    point off = { 0.0, offset + thick /*+ width/2.0 */   };
    Simple_Line_Primitive(prim, off, 2*length, thick   );
    rotate( prim, 4, angle );
    translate( prim, 4, anchor );
    Sum_Pixel_Overlap( (float*) prim, 4, density, image, strides );
  }

  //{ point prim[4];
  //  point off = { 0.0, offset - thick - width/2.0   };
  //  Simple_Line_Primitive(prim, off, 2*length, thick   );
  //  rotate( prim, 4, angle );
  //  translate( prim, 4, anchor );
  //  Sum_Pixel_Overlap( (float*) prim, 4, -density, image, strides );
  //}

  { point prim[12];
    int npoint = 12;
    point off = { 0.0, offset };
    Simple_Circle_Primitive(prim,npoint,off,length, 1);
    rotate( prim, npoint, angle );
    translate( prim, npoint, anchor );
    Multiply_Pixel_Overlap( (float*) prim, npoint, density, 0, image, strides );
  }

  return;
}

inline float *Get_Half_Space_Detector( Array *bank, int ioffset, int iwidth, int iangle  )
{ return ((float*) (bank->data)) + iangle  * bank->strides_px[1] 
                                 + iwidth  * bank->strides_px[2] 
                                 + ioffset * bank->strides_px[3]; 
}

Array *Build_Half_Space_Detectors( Range off, 
                                   Range wid, 
                                   Range ang, 
                                   float length,
                                   int supportsize )
{ Array *bank;
  int noff =  compute_number_steps( &off ),
      nwid =  compute_number_steps( &wid ),
      nang =  compute_number_steps( &ang );
  int shape[5] = { supportsize,
                   supportsize,
                   noff,
                   nwid,
                   nang};
  bank = Make_Array( shape, 5, sizeof(float) );
  memset( bank->data, 0, bank->strides_bytes[0] );

  { int    o,a,w;
    for( o = 0; o < noff; o++ )
    { //point anchor = {supportsize/2.0, o*off.step + off.min + supportsize/2.0};
      point anchor = {supportsize/2.0, supportsize/2.0};
      for( a = 0; a < nang; a++ )
        for( w = 0; w < nwid; w++ )
          Render_Half_Space_Detector(
              o*off.step + off.min,                       //offset (before rotation)
              length,                                     //length,
              a*ang.step + ang.min,                       //angle,
              w*wid.step + wid.min,                       //width,
              anchor,                                     //anchor,
              Get_Half_Space_Detector( bank, o,w,a),      //image
              bank->strides_px + 3);                      //strides
    }
  }

  return bank;
}
