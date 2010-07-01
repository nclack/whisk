/*
 * The main entrance point here is the function:
 *
 *  Whisker_Seg  *find_segments(  int iFrame,
 *                                Image *image,
 *                                Zone *mask,
 *                                Image *bg,
 *                                int *pnseg )
 *
 *
 * [ ] TLEN not checked on detector bank load.  Need to rebuild if this
 *     changes.
 */
#include "compat.h"
#define _USE_MATH_DEFINES
#include "trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <assert.h>

#include "utilities.h"
#include "common.h"
#include "image_lib.h"
#include "draw_lib.h"
#include "image_filters.h"
#include "level_set.h"
#include "contour_lib.h"

//#include "distance.h"
#include "eval.h"
#include "seed.h"
   
#include "parameters/param.h"

#if 0
#define DEBUG_DETECTOR_BANK
#define DEBUG_READ_LINE_DETECTOR_BANK
#define SHOW_HALF_SPACE_DETECTOR
#define SHOW_LINE_DETECTOR
#define DEBUG_SEEDING_FIELDS
#define DEBUG_LINE_FITTING
#define SHOW_WHISKER_TRACE

#define DEBUG_LINE_FITTING
#define DEBUG_WHISKER_TRACE
#define DEBUG_MOVE
#define DEBUG_LINE_DETECTOR
#define DEBUG_DETECT_LOOPS
#define DEBUG_SEEDING_MASK
#endif

#undef  DEBUG_COMPUTE_SEED_FROM_POINT
#undef  DEBUG_REQUEST_STORAGE

#define FILE_FORMAT_WHISKBIN1

#undef  SHOW_ZONE_HISTOGRAM

#undef  EXPORT_COMPZONE_TIF
#undef  APPLY_ZONE_MASK

//#define SEED_ON_GRID
//#if 0
//#define SEED_ON_MHAT_CONTOURS
//#endif

//#define SEED_ON_GRID_LATTICE_SPACING 50

#define HARMONIC_MEAN_N_LABELS 2

#define LINE_DETECTOR_NORMAL   0
#define LINE_DETECTOR_HARMONIC 1
#define LINE_DETECTOR_CURVED   2
#define LINE_DETECTOR_SPECIAL  20

SHARED_EXPORT
Image       * subtract_background_inplace  (Image *a, Image *b);

SHARED_EXPORT
void          initialize_paramater_ranges  (Line_Params *line, Interval *roff, Interval *rang, Interval *rwid);

void breakme(void) {
}

static inline int outofbounds(int q, int cwidth, int cheight)
{ int x = q%cwidth;
  int y = q/cwidth;
  return (x < 1 || x >= cwidth-1 ||y < 1 || y >= cheight-1);
}

inline int is_small_angle( float angle )
  /* true iff angle is in [-pi/4,pi/4) or [3pi/4,5pi/4) */
{ static const float qpi = M_PI/4.0;
  static const float hpi = M_PI/2.0;
  int n = floorf( (angle-qpi)/hpi );
  return  (n % 2) != 0;
}

inline int is_angle_leftward( float angle )
  /* true iff angle is in left half plane */
{ //static const float qpi = M_PI/4.0;
  static const float hpi = M_PI/2.0;
  int n = floorf( (angle-hpi)/M_PI );
  return  (n % 2) == 0;
}

SHARED_EXPORT
Whisker_Seg *Make_Whisker_Seg( int n )
{ Whisker_Seg *w = (Whisker_Seg*) Guarded_Malloc( sizeof(Whisker_Seg), "Make whisker segment - root." );
  w->len = n;
  w->x =      (float*) Guarded_Malloc( sizeof(float) * n, "Make whisker segment - x." );
  w->y =      (float*) Guarded_Malloc( sizeof(float) * n, "Make whisker segment - y." );
  w->thick =  (float*) Guarded_Malloc( sizeof(float) * n, "Make whisker segment - thick." );
  w->scores = (float*) Guarded_Malloc( sizeof(float) * n, "Make whisker segment - scores." );
  return w;
}

SHARED_EXPORT
void Free_Whisker_Seg( Whisker_Seg *w )
{ if(w)
  { if ( w->scores ) free( w->scores );
    if ( w->thick  ) free( w->thick  );
    if ( w->y      ) free( w->y      );
    if ( w->x      ) free( w->x      );
    free(w);
  }
}

SHARED_EXPORT
void Free_Whisker_Seg_Vec ( Whisker_Seg *wv, int n )
{
  while( n-- )
  { Whisker_Seg *w = wv + n;
    if(w)
    { if ( w->scores ) {free( w->scores ); w->scores = NULL; }
      if ( w->thick  ) {free( w->thick  ); w->thick  = NULL; }
      if ( w->y      ) {free( w->y      ); w->y      = NULL; }
      if ( w->x      ) {free( w->x      ); w->x      = NULL; }
    }
  }
  free(wv);
}

int _cmp_whisker_seg_uid(const void *a, const void *b)
{ Whisker_Seg *wa =(Whisker_Seg*) a,
              *wb =(Whisker_Seg*) b;
  int d = wa->time - wb->time;
  return (d==0) ? (wa->id - wb->id) : d;
}


SHARED_EXPORT
void Whisker_Seg_Sort_By_Id( Whisker_Seg *wv, int n )
{ qsort( wv, n, sizeof(Whisker_Seg), _cmp_whisker_seg_uid );
}

SHARED_EXPORT
void Estimate_Image_Shape_From_Segments( Whisker_Seg* wv, int n, int *width, int *height )
{ int w = 0,
      h = 0;
  while(n--)
  { Whisker_Seg *cur = wv + n;
    int i = cur->len;
    float *x = cur->x,
          *y = cur->y;
    while(i--)
    { w = MAX( w, x[i] );
      h = MAX( h, y[i] );
    }
  }
  *width  = w+1;
  *height = h+1;
}

SHARED_EXPORT
Line_Params line_param_from_seed( const Seed *s )
{   Line_Params line;
    const double hpi = M_PI/4.0;
    const double ain = hpi/ANGLE_STEP;
    line.offset = .5;
    { if( s->xdir < 0 ) // flip so seed points along positive x
      { line.angle  = round(atan2(-1.0f* s->ydir,-1.0f* s->xdir) / ain) * ain;
      } else {
        line.angle  = round(atan2(s->ydir,s->xdir) / ain) * ain;
      }
    } 
    line.width  = 2.0;
    return line;
}

SHARED_EXPORT
Image *subtract_background_inplace(Image *a, Image *b)
{ int i,n = a->width*a->height;
  uint8 *ta,*tb;
  uint8 max=0;

  ta = a->array;
  tb = b->array;
  for( i=0; i<n; i++, ta++, tb++ )
  { *ta = CLAMP_UINT8( (*tb) - (*ta)  );
    if( (*ta)>max )
      max = *ta;
  }
  /* invert so dark whiskers on a bright bg */
  Scale_Image_To_Range( a, 0, 0, 255 );
  Scale_Image( a, 0, -1, -255 );

  return a;
}

SHARED_EXPORT
Object_Map *get_objectmap( Image *image )
{ static Image *hat = NULL;
  Image *imhat;
  Object_Map *omap;
  if( !hat )
    hat = Mexican_Hat_2D_Filter(HAT_RADIUS);
  imhat = Copy_Image( image );
  Translate_Image(imhat,FLOAT32,1);
  Convolve_Image(hat,imhat,1);
  Scale_Image(imhat,0,-1.,0.);
  Truncate_Image(imhat,0,0.);
  Scale_Image_To_Range(imhat,0,0.,255.);
  //Scale_Image_To_Range(imhat,0,255,0);
  Translate_Image(imhat,GREY,1);

  omap = find_objects(imhat,MIN_LEVEL,MIN_SIZE);
  Free_Image(imhat);
  return omap;
}


SHARED_EXPORT
void draw_whisker_update_rasters( int *raster, float x0, float y0, float x1, float y1, int height )
{ int y;
  int yl = (int) y1,
      yh = (int) y0;
  float t, n;
  if( yl > yh )
    SWAP( yl, yh );
  n = y1 - y0;
  for( y = yl; y <= yh; y++ )
  { float x,dx;
    int *run;
    if( (y<0) || (y>=height) )
      continue;
    run = raster + 2*y;
    t = (y-yl)/(yh-yl+1);
    dx  = x1 - x0;
    x = MAX( 0.0f, roundf( x0 + t * dx ) );
    if(x==0.0f)
      breakme();
    if( run[0] < 0 )      // no hits on this line yet
    { run[0] = (int) x;
    } else                        // there's been a hit
    { if( run[1] < 0 )            // Case 0: this is the second hit
      { run[1] = (int) x;
        if( run[0] > run[1] )
          SWAP( run[0], run[1] );
      } else
      { if( x < run[0] )          // Case 1: new x is outside and smaller
        { run[0] = (int) x;
        } else if( x>run[1] )     // Case 2: new x is outside and larger
        { run[1] = (int) x;
        }                         // Case 3: new x is inside interval - do nothing
      }
    }
  }
  return;
}

SHARED_EXPORT
void draw_whisker( Image *image, Whisker_Seg *w, int thick, uint8 value )
  /* The basic steps are:
   * 1. Form polygon by offsetting whisker segment backbone by thick along
   *    direction approximately normal to whisker
   * 2. Compute rasters from the polygon
   * 3. Fill image by rasters
   *
   * This is a bit different than computing the raster for the contours
   * described by contour_lib.h in that the vertexes of the polygon are not
   * confined to an integral lattice nor are they necessarily adjacent on any
   * lattice.
   */
{ static int *rasters = NULL; // a pairs of x values for each y in image
  static size_t maxrasters = 0;
  float ox,oy;

  rasters = (int*) request_storage( rasters, &maxrasters, 2*sizeof(int), image->height, "draw_whisker - rasters");
  memset( rasters, -1, 2*sizeof(int)*(image->height) );

  //compute offsets by average whisker angle
  { float th = 0.0;
    int i = w->len;
    float *y = w->y,
          *x = w->x;
    while( --i )                        //accumulate angle
      th += atan2f( y[i] - y[i-1], x[i] - x[i-1] );
    th /= (w->len-1.0f);                //average
    th += M_PI/2.0;                     //right turn
    ox = thick*cos(th);                 //project
    oy = thick*sin(th);
  }

  // compute rasters, both sets of edges
  { int i = w->len;
    while( --i )
    { draw_whisker_update_rasters( rasters,
                                   w->x[i]    - ox,
                                   w->y[i]    - oy,
                                   w->x[i-1]  - ox,
                                   w->y[i-1]  - oy,
                                   image->height );
      draw_whisker_update_rasters( rasters,
                                   w->x[i]    + ox,
                                   w->y[i]    + oy,
                                   w->x[i-1]  + ox,
                                   w->y[i-1]  + oy,
                                   image->height );
    }
    // add the end caps
    draw_whisker_update_rasters(  rasters,
                                  w->x[0]    + ox,
                                  w->y[0]    + oy,
                                  w->x[0]    - ox,
                                  w->y[0]    - oy,
                                  image->height );
    i = w->len-1;
    draw_whisker_update_rasters(  rasters,
                                  w->x[i]    + ox,
                                  w->y[i]    + oy,
                                  w->x[i]    - ox,
                                  w->y[i]    - oy,
                                  image->height );
  } // end - compute raster

  // clean up any out of bounds
  { int i = 2*image->height;
    int w = image->width - 1;
    while( i-- )
      rasters[i] = MIN( rasters[i], w );
  }

  // paint
  { int y = image->height;
    while(y--)
    { int *run = rasters + 2*y;
      if( run[0] >= 0 ) // for a closed polygon, run[0] >=0 <==> run[1] >=0
      { //printf( "Paint from %3d to %3d @ y = %3d (n = %5d)\n", run[0], run[1], y, run[1]-run[0]+1 );
        memset( ((uint8*)image->array) + image->width * y + run[0],
                value,
                sizeof(uint8) * (run[1]-run[0]+1) );
      }
    }
  }

}

int _cmp_seed_scores(const void *a, const void *b)
{ typedef struct {int i; float s;} S;
  float d = ((S*)a)->s - ((S*)b)->s;
  if( d==0.0 ) return 0;
  return d < 0 ? -1 : 1;
}

SHARED_EXPORT
Whisker_Seg *find_segments( int iFrame, Image *image, Image *bg, int *pnseg )
{ static Image *h = NULL,   // histogram from compute_seed_from_point_field_windowed_on_contour
               *th = NULL,  // slopes                             "
               *s = NULL,   // stats                              "
               *mask = NULL;// Mask for keeping track of seed points
  static int sarea = 0;
         int  area = image->width * image->height;
  Object_Map  *omap;
  Whisker_Seg *wsegs = NULL;
  size_t max_segs= 0;
  int n_segs=0;

  // Prepare
  if( !h || ( sarea != area ) )
  { if(h)
    { Free_Image(h);
      Free_Image(th);
      Free_Image(s);
    }
    h     = Make_Image(GREY8,   image->width, image->height );
    th    = Make_Image(FLOAT32, image->width, image->height );
    s     = Make_Image(FLOAT32, image->width, image->height );
    mask  = Make_Image(GREY8,   image->width, image->height );
    sarea = area;
  }
  memset(    h->array, 0, sarea *    h->kind );
  memset(   th->array, 0, sarea *   th->kind );
  memset(    s->array, 0, sarea *    s->kind );
  memset( mask->array, 0, sarea * mask->kind );

  // Get contours, and compute correlations on perimeters
  switch(SEED_METHOD)
  {
    case SEED_ON_MHAT_CONTOURS:
      {
        omap = get_objectmap( image );
#ifdef DEBUG_SEEDING_FIELDS
        { Image *cim = Copy_Image( image );
          Paint_Brush brush = { 1.0, -1.0, -1.0 }; 
          int i;

          cim = Translate_Image( cim, COLOR, 1 );
          for( i=0; i < omap->num_objects; i++ )
            Draw_Contour_Interior( omap->objects[i], &brush, cim );
          Write_Image("hat_contour.tif", cim);
        }
#endif
        { int i;
          for( i=0; i < omap->num_objects; i++ )
          { compute_seed_from_point_field_windowed_on_contour( image, omap->objects[i],
              SEED_ITERATIONS,       // maxr
              SEED_ITERATION_THRESH, // iteration threshold
              SEED_ACCUM_THRESH,     // accumulation threshold
              h, th, s );
            Free_Contour( omap->objects[i] );
          }
        }
      }
      break;
    case SEED_ON_GRID:
      {
        compute_seed_from_point_field_on_grid( image,
            SEED_ON_GRID_LATTICE_SPACING, // lattice spacing
            SEED_ITERATIONS,              // maxr
            SEED_ITERATION_THRESH,        // iteration threshold
            SEED_ACCUM_THRESH,            // accumulation threshold
            h,th,s );
      }
      break;
    default:
      error("Did not recognize value for SEED_METHOD.  Got: %d\n",SEED_METHOD);
  }
#ifdef DEBUG_SEEDING_FIELDS
  { Image *tmp_th, *tmp_s;
    Write_Image( "trace_seed_hist.tif", h );
    Write_Image( "trace_seed_image.tif", image );

    tmp_th = Copy_Image( th );
    Scale_Image( tmp_th, 0, 255.0/2.0/M_PI, M_PI );
    tmp_th = Translate_Image( tmp_th, GREY8, 1 );
    Write_Image( "trace_seed_angles.tif", tmp_th);
    Free_Image(tmp_th);

    tmp_s = Copy_Image( th );
    Scale_Image_To_Range( tmp_s, 0, 0.0, 255.0 );
    tmp_s = Translate_Image( tmp_s, GREY8, 1 );
    Write_Image( "trace_seed_stat.tif", tmp_s);
    Free_Image(tmp_s);
  }
#endif

  { int i = sarea;
    int nseeds = 0;
    float *sa    = (float*)   s->array,
          *tha   = (float*)  th->array;
    uint8 *ha    = (uint8*)   h->array,
          *maska = (uint8*)mask->array;

    // Compute means and mask
    while( i-- )
    { float n = (float) ha[i];
      if( n > 0.0f )
        tha[i] /= n;
    }
    i = sarea;
    while( i-- )
    { if( sa[i] > 0.0 )
      { maska[i] = 1;
        nseeds ++;
      }
    }

    { typedef struct { int idx; float score; } seedrecord;
      seedrecord *scores = malloc(sizeof(seedrecord)*nseeds);
      int stride = image->width;
      Line_Params line;
      int j = 0;

      i = sarea;
      while( i-- )
      { if( sa[i] > 0.0 )
        { Seed seed = { i%stride,
              i/stride,
              (int) 100 * cos( tha[i] ),
              (int) 100 * sin( tha[i] ) };

          line = line_param_from_seed( &seed );

          scores[j  ].score = eval_line( &line, image, i );

          scores[j  ].idx   = i;
          j++;
        }
      }

      qsort(scores, nseeds, sizeof(seedrecord), &_cmp_seed_scores );

      j = nseeds;
      while(j--)
      { i = scores[j].idx;
        if( maska[i]==1 )
        { Whisker_Seg *w;
          Seed seed = { i%stride,
            i/stride,
            (int) 100 * cos( tha[i] ),
            (int) 100 * sin( tha[i] ) };
          w = trace_whisker( &seed, image );
          if (w != NULL)
          { wsegs = (Whisker_Seg*) request_storage( wsegs, &max_segs, sizeof(Whisker_Seg), n_segs+1, "find segments" );
            w->time = iFrame;
            w->id  = n_segs;
            wsegs[n_segs++] = *w;
            draw_whisker( mask , w, 2, 3 ); // set to 3 for debug, could be anything but 1
            free(w);
#ifdef DEBUG_SEEDING_MASK
            //Write_Image("trace_seed_mask.tif",mask);
            Write_Image( "trace_seed_image.tif", image );
            breakme();
#endif
          } // ... if w
        } // ... if maska[i]
      }
      free(scores);
    }
  } // end context
  *pnseg = n_segs;
  return wsegs;
}

SHARED_EXPORT
void   median_uint8(  unsigned char *s,/* array with the data. size = n x m    */
                      int n,           /* e.g. n pixels in an image            */
                      int m,           /* e.g. m images in a time-series       */
                      int stride,      /* number of bytes between planes       */
                      unsigned char *r)/* array of size n storing the median   */
{ unsigned long hist[256], count, half;
  int i,j;
  unsigned char k;
  unsigned char *cs;

  half = m/2;
  for( i=0; i<n; i++ )
  { memset(hist, 0, 256*sizeof(unsigned long));
    cs = s + i;
    for( j=0; j<m; j++ )
    { hist[ *(cs + j*stride) ]++;
    }
    count = 0;
    k = 255;
    while( k != 0 )
    { count += hist[k--];
      if( count >= half )
      { r[i] = k + 1;
        break;
      }
    }
  }

  return;
}

SHARED_EXPORT
Image *compute_background(Stack *movie)
{ Image *bg;

 

  bg = Make_Image( GREY8, movie->width, movie->height );

  if( movie->kind != GREY8 )
  { fprintf( stderr, "Can only handle GREY8 data right now.\n");
    goto error;
  }
  median_uint8( movie->array,
                movie->width * movie->height,
                movie->depth,
                movie->width * movie->height,
                bg->array );

//// MAX PROJECTION
//for( x=0; x<movie->width; x++ )                /* Compute max projection:   */
//  for( y=0; y<movie->height; y++ )             /*  Since moving things are  */
//  { for( max=z=0; z<movie->depth; z++ )        /*  dark, this should result */
//    { v = Get_Stack_Pixel( movie, x, y, z, 0 );/*  in the stationary bright */
//      max = ( v>max ) ? v : max;               /*  background.              */
//    }                                          /*                           */
//    Set_Image_Pixel( bg, x, y, 0, max );
//  }

#ifdef WRITE_BACKGROUND
  Write_Image("bg.tif", bg);
#endif
  return bg;
error:
  Free_Image( bg );
  return (NULL);
}

SHARED_EXPORT
Zone *compute_zone(Stack *movie)
{ static Zone myzone;

  int histo[256];
  int    w, h, d, a;
  Image *mask;

  DEPRICATED;

  w = movie->width;
  h = movie->height;
  d = movie->depth;
  a = w*h;

  myzone.mask = mask = Make_Image(GREY,w,h);

  { uint8 *t0, *t1, *za;
    int    t, p;

    za = mask->array;
    for (p = 0; p < a; p++)
      *za++ = 0;

    t0 = movie->array;
    t1 = movie->array + FRAME_DELTA*a;
    for (t = FRAME_DELTA; t < d; t++)
      { za = mask->array;
        for (p = 0; p < a; p++)
          { int da = abs(*t1-*t0);
            if (da > 255)
              da = 255;
            if (da > *za)
              *za = da;
            za += 1;
            t0 += 1;
            t1 += 1;
          }
      }
  }

#ifdef EXPORT_COMPZONE_TIF
  Write_Image( "compzone.tif", mask );
#endif

  { uint8 *za;
    int    h, p;
    int    total, median;
    int    ahalf = a/2;

    for (h = 0; h < 256; h++)
      histo[h] = 0;

    za = mask->array;
    for (p = 0; p < a; p++)
      histo[*za++] += 1;

    total  = 0;
    median = 0;
    for (h = 0; h < 256; h++)
      { total += histo[h];
        if (total < ahalf)
          median = h+1;

#ifdef SHOW_ZONE_HISTOGRAM
        printf("%3d: %12ld (%6.2f)\n",h,histo[h],(total*100.)/a);
#endif
      }

    myzone.cutoff = median/2;
#ifdef SHOW_ZONE_HISTOGRAM
    printf("  Cutoff = %d (%d)\n",myzone.cutoff,median);
#endif
  }

  return (&myzone);
}

SHARED_EXPORT
void remove_duplicate_segments( Whisker_Seg *wv, int *n )
{ assert(0); // FIXME: impliment -- see merge.c
}

SHARED_EXPORT
int  write_line_detector_bank( char *filename, Array *bank, Range *off, Range *wid, Range *ang )
{ FILE *fp = fopen(filename,"wb");
//  printf("%s %p\n",filename,fp);

  if(fp != NULL)
  { fflush(fp);
    fseek(fp,0,SEEK_SET);
//    Print_Range(stdout, off);
    Write_Range(fp, off);
//    Print_Range(stdout, wid);
    Write_Range(fp, wid);
//    Print_Range(stdout, ang);
    Write_Range(fp, ang);
//    printf("write bank\n");
    Write_Array(fp, bank);
    fclose(fp);
    return 1;
  } else {
    warning( "Couldn't write line detector bank.\n\tUnable to open file for writing.\n");
    return 0;
  }
}

SHARED_EXPORT
int  read_line_detector_bank( char *filename, Array **bank, Range *off, Range *wid, Range *ang )
{ FILE *fp;
  Range o,w,a;

  fp = fopen(filename,"rb");
#ifdef DEBUG_READ_LINE_DETECTOR_BANK
  //progress("errno: %d\n",errno);
  progress("%s %p\n"
           "pointer to banks pointer: %p\n"
           "           banks pointer: %p\n",filename,fp,bank,*bank);
#endif  
  if(fp)
  { int n = fseek(fp,0,SEEK_SET);
#ifdef DEBUG_READ_LINE_DETECTOR_BANK
  progress("seek 0: result %d\n",n);
#endif
    Read_Range(fp, &o);
#ifdef DEBUG_READ_LINE_DETECTOR_BANK
    progress("read off (%p), ",&o);
    Print_Range(stderr, &o);    
#endif
    Read_Range(fp, &w);
#ifdef DEBUG_READ_LINE_DETECTOR_BANK
    progress("read wid, ");
    Print_Range(stderr, &w);
#endif
    Read_Range(fp, ang);
#ifdef DEBUG_READ_LINE_DETECTOR_BANK
    progress("read ang, ");
    Print_Range(stderr, &a);
#endif
    *bank = Read_Array(fp);
#ifdef DEBUG_READ_LINE_DETECTOR_BANK
    progress("read bank\n");
#endif
    fclose(fp);
    return Is_Same_Range(&o,off) && 
           Is_Same_Range(&w,wid) &&
           Is_Same_Range(&a,ang);
  } else {
    warning("Couldn't read line detector bank.\n");
    *bank = (NULL);
    return 0;
  }
}

SHARED_EXPORT
Array *get_line_detector_bank(Range *off, Range *wid, Range *ang)
{ static Array *bank = (NULL);
  static Range o,a,w;    
  if( !bank )
  { 
    Range v[3] = {{ -1.0,       1.0,         OFFSET_STEP },          //offset
                  { -M_PI/4.0,  M_PI/4.0,    M_PI/4.0/ANGLE_STEP },  //angle
                  {  WIDTH_MIN, WIDTH_MAX,   WIDTH_STEP  }};         //width
    o = v[0];
    a = v[1];
    w = v[2];
    if( read_line_detector_bank( "line.detectorbank", &bank, &o, &w, &a ) )
    { progress("Line detector bank loaded from file.\n");
    } else {
      progress("Computing line detector bank.\n");
      bank = Build_Line_Detectors( o, w, a, TLEN, 2*TLEN+3 );
      write_line_detector_bank( "line.detectorbank", bank, &o, &w, &a );
    }
    if(!bank) goto error;
  }
  *off = o; *ang = a; *wid = w;
  return bank;
error:
  warning("Couldn't build bank of line detectors!\n");
  return (NULL);
}


SHARED_EXPORT
Array *get_harmonic_line_detector_bank(Range *off, Range *wid, Range *ang) // FIXME: Bad name - should be labelled line detectors
{ static Array *bank = (NULL);
  static Range o,a,w;
  DEPRICATED;
  // [ ] if this gets put back into active duty - need to adapt to
  //      get_line_detector_bank pattern
  if( !bank )
  { if( read_line_detector_bank( "harmonic.detectorbank", &bank, &o, &w, &a ) )
    { fprintf(stderr,"Harmonic detector bank loaded from file.\n");
    } else {
      Range v[3] = {{ -1.0,       1.0,         OFFSET_STEP },          //offset
                    { -M_PI/4.0,  M_PI/4.0,    M_PI/4.0/ANGLE_STEP },  //angle
                    {  0.5,       6.5,         WIDTH_STEP  }};         //width
      fprintf(stderr,"Computing harmonic detector bank.\n");
      o = v[0];
      a = v[1];
      w = v[2];
      bank = Build_Harmonic_Line_Detectors( o, w, a, TLEN, 2*TLEN+3 );
      write_line_detector_bank( "harmonic.detectorbank", bank, &o, &w, &a );
    }
    if(!bank) goto error;
  }
  *off = o; *ang = a; *wid = w;
  return bank;
error:
  fprintf(stderr,"Warning: Couldn't build bank of line detectors!\n");
  return (NULL);
}

SHARED_EXPORT
float integrate_harmonic_mean_by_labels( uint8 *im, float* w, int *pxlist, int npx )
{ // assumes 4 groups.  Takes harmonic mean of group means
  float acc[HARMONIC_MEAN_N_LABELS] = {0.0,0.0/*,0.0,0.0*/};
  float norm[HARMONIC_MEAN_N_LABELS] = {0.0,0.0/*,0.0,0.0*/};
  float totalnorm = 0.0;
  unsigned int labels[HARMONIC_MEAN_N_LABELS] = {2,3/*,5,7*/};
  const float  sigmin = 255.0;
  int j,i;

  DEPRICATED;
  i = npx; 
  while( i-- )
  { float u = w[ pxlist[2*i+1] ];
    int code = lround( u );
    float v = (u-code) * 10.0;
    if( code  )
    { 
      //printf("\t[harmonic]%d: \n",i);
      for(j=0;j<HARMONIC_MEAN_N_LABELS;j++)
        if( code%labels[j] == 0 )
        { //acc[j] += v * im[pxlist[2*i]]; 
          acc[j] += v * ( im[ pxlist[2*i] ] ); // don't need counts b.c. weighted mean
          norm[j] += 1; //fabs(v);
          //printf("\t[harmonic]\t%3d: (code: %3d) (label: %3d) mean: %+g\n",j,code,labels[j], acc[j]/norm[j]);
        }
    }
  }
  totalnorm = 0.0;
  j = HARMONIC_MEAN_N_LABELS;
  while( j-- )
    totalnorm += norm[j];


#ifdef DEBUG_INTEGRATE_HARMONIC_MEAN_BY_LABELS
  { float ttl1=0.0, ttl2= 0.0, ttl3 = 0.0;
    j=HARMONIC_MEAN_N_LABELS;
    printf("\t[scores]\t");
    while(j--)
    { printf("% 7.5g ",-acc[j]+sigmin);
      ttl1 += acc[j]-sigmin;
    }
    printf("|| %g\n",ttl1);
    j=HARMONIC_MEAN_N_LABELS;
    printf("\t        \t");
    while(j--)
    { ttl2 += norm[j];
      printf("% 7.5g ", norm[j]);
      ttl3 += norm[j]/(acc[j]-sigmin);
    }
    printf("|| %g\n",ttl2);
    if( fabs(ttl2/ttl3) > 1e3 )
      printf("\t********\tmean = % +7.5g  harmonic mean = % +7.5g ********\n",ttl1/ttl2,ttl2/ttl3);
    else
      printf("\t        \tmean = % +7.5g  harmonic mean = % +7.5g\n",ttl1/ttl2,ttl2/ttl3);
  }
#endif// DEBUG_INTEGRATE_HARMONIC_MEAN_BY_LABELS
  
  
  // return harmonic mean
  { float ret = 0.0;
    j = HARMONIC_MEAN_N_LABELS;
    while(j--)
      ret += norm[j]/(acc[j]-sigmin); // const bias to aid convergence...samples must be on one side of zero
    return totalnorm/ret;
  }
}

SHARED_EXPORT
float integrate_special_by_labels( uint8 *im, float *w, int *pxlist, int npx )
{ float acc[HARMONIC_MEAN_N_LABELS] = {0.0,0.0};
  float total;
  unsigned int labels[HARMONIC_MEAN_N_LABELS] = {2,3};
  int j,i = npx;
  float norm = 0.0;

  DEPRICATED;

  while( i-- )
  { float u = w[ pxlist[2*i+1] ];
    int code = lround( u );
    float v = (u-code) * 10.0;
    norm += v;
    if( code  )
    { for(j=0;j<HARMONIC_MEAN_N_LABELS;j++)
        if( code%labels[j] == 0 )
          acc[j] += v *  im[ pxlist[2*i] ] ; 
    }
  }
  { float a = acc[0],  // remember these can be negative
          b = acc[1];  // or near zero
    //const float sigmin = (2*TLEN+1)*MIN_SIGNAL;// + 255.00;
    //float t1,t2;
    total = 2*(a+b)*(a*b)/(a*a+b*b);
  }
  //printf("\t\t\t\t% +5.3g  % +5.3g %g\n",acc[0],acc[1],-log10(fabs(norm)));
  //return MAX(acc[0],acc[1]);
  //return total*MIN(fabs(acc[0]),fabs(acc[1]))/MAX(fabs(acc[0]),fabs(acc[1]));
  return total; //2*acc[0]*acc[1]/total;
}

SHARED_EXPORT
float *get_nearest_from_line_detector_bank(float offset, float width, float angle)
{ int o,a,w;
  Range orng, arng, wrng;
  //Array *bank = get_harmonic_line_detector_bank( &orng, &wrng, &arng );
  Array *bank = get_line_detector_bank( &orng, &wrng, &arng );

  //If the angle is > 45 deg, fetch the detector
  //  that, when transposed, will be correct.
  //  This mechanism lets me store only half the detectors.
  if( !is_small_angle( angle ) )  // if large angle then transpose
  { angle = 3.0*M_PI/2.0 - angle; //   to small ones ( <45deg )
	//offset = -offset;			  // The transpose is a rotation and flip
  }								  // T = R(3pi/2) Flip
  WRAP_ANGLE_2PI( angle );        // rotating as angle -= 3pi/2 also flips the offset
                                  //    so it doesn't need to be done explicitly
  //Lines are left right symmetric
  //This allows us to store only half the angles (again)
  if( is_angle_leftward(angle) )
  { WRAP_ANGLE_HALF_PLANE( angle );
    offset = -offset; 
  }

  o = lround( ( offset - orng.min ) / orng.step );
  a = lround( (  angle - arng.min ) / arng.step );
  w = lround( (  width - wrng.min ) / wrng.step );
#ifdef DEBUG_DETECTOR_BANK
  { int wrn = 0;
  printf("\tBANK: Fetching (%3d,%3d,%3d) o: %7.4g, a: %7.4g, w:%7.4g\n",
      o,a,w,
      o*orng.step + orng.min,
      a*arng.step + arng.min,
      w*wrng.step + wrng.min);
  if( offset - orng.max >  1e-3) {printf("Warning: Offset exceeded max. (%g)\n",offset); wrn=1;}
  if( offset - orng.min < -1e-3) {printf("Warning: Offset exceeded min. (%g)\n",offset); wrn=1;}
  if(  width - wrng.max >  1e-3) {printf("Warning:  Width exceeded max. (%g)\n",width);  wrn=1;}
  if(  width - wrng.min < -1e-3) {printf("Warning:  Width exceeded min. (%g)\n",width);  wrn=1;}
  if(  angle - arng.max >  1e-3) {printf("Warning:  Angle exceeded max. (%g)\n",angle);  wrn=1;}
  if(  angle - arng.min < -1e-3) {printf("Warning:  Angle exceeded min. (%g)\n",angle);  wrn=1;}
  if(wrn)
    breakme();
  }
#endif
  return Get_Line_Detector( bank, o, w, a );
}

#if 0
float *get_nearest_from_curved_line_detector_bank(float offset, float width, float angle)
{ int o,a,w;
  Range orng, arng, wrng;
  Array *bank = get_curved_line_detector_bank( &orng, &wrng, &arng );
  DEPRICATED;
  if( !is_small_angle( angle ) )  // if large angle then transpose
  { angle = 3.0*M_PI/2.0 - angle; //   to small ones ( <45deg )
  }
  WRAP_ANGLE_2PI( angle );

  //sometimes need to flip the line upside down
  if( is_angle_leftward(angle) )
  { WRAP_ANGLE_HALF_PLANE( angle );
    offset = -offset;
  }

  o = lround( ( offset - orng.min ) / orng.step );
  a = lround( (  angle - arng.min ) / arng.step );
  w = lround( (  width - wrng.min ) / wrng.step );
#ifdef DEBUG_DETECTOR_BANK
  { int wrn = 0;
  printf("\tBANK: Fetching (%3d,%3d,%3d) o: %7.4g, a: %7.4g, w:%7.4g\n",
      o,a,w,
      o*orng.step + orng.min,
      a*arng.step + arng.min,
      w*wrng.step + wrng.min);
  if( offset - orng.max >  1e-3) {printf("Warning: Offset exceeded max. (%g)\n",offset); wrn=1;}
  if( offset - orng.min < -1e-3) {printf("Warning: Offset exceeded min. (%g)\n",offset); wrn=1;}
  if(  width - wrng.max >  1e-3) {printf("Warning:  Width exceeded max. (%g)\n",width);  wrn=1;}
  if(  width - wrng.min < -1e-3) {printf("Warning:  Width exceeded min. (%g)\n",width);  wrn=1;}
  if(  angle - arng.max >  1e-3) {printf("Warning:  Angle exceeded max. (%g)\n",angle);  wrn=1;}
  if(  angle - arng.min < -1e-3) {printf("Warning:  Angle exceeded min. (%g)\n",angle);  wrn=1;}
  if(wrn)
    breakme();
  }
#endif
  return Get_Line_Detector( bank, o, w, a );
}
#endif

SHARED_EXPORT
Array *get_half_space_detector_bank(Range *off, Range *wid, Range *ang, float *norm)
{ static Array *bank = (NULL);
  static float sum = -1.0;
  static Range o,a,w;
  if( !bank )  
  {
    Range v[3] = {{ -1.0,       1.0,         OFFSET_STEP },          //offset
                  { -M_PI/4.0,  M_PI/4.0,    M_PI/4.0/ANGLE_STEP },  //angle
                  {  WIDTH_MIN, WIDTH_MAX,   WIDTH_STEP  }};         //width
    o = v[0];
    a = v[1];
    w = v[2];
    if( read_line_detector_bank( "halfspace.detectorbank", &bank, &o, &w, &a ) )
    { progress("Half-space detector bank loaded from file.\n");
    } else {
      fprintf(stderr,"Computing half space detector bank.\n");
      bank = Build_Half_Space_Detectors( o, w, a, TLEN, 2*TLEN+3 );
      write_line_detector_bank( "halfspace.detectorbank", bank, &o, &w, &a );
    }
    if(!bank) goto error;
    
    { float *m = Get_Half_Space_Detector(bank,0,0,0);
      int n = (2*TLEN+3)*(2*TLEN+3);
      while(n--)      
        sum += m[n];
    }
  }
  *off = o; *ang = a; *wid = w; *norm = sum;
  return bank;
error:
  fprintf(stderr,"Warning: Couldn't build bank of half-space detectors!\n");
  return (NULL);
}

SHARED_EXPORT
float *get_nearest_from_half_space_detector_bank(float offset, float width, float angle, float *norm)
{ int o,a,w;
  Range orng, arng, wrng;
  Array *bank = get_half_space_detector_bank( &orng, &wrng, &arng, norm );

  if( !is_small_angle( angle ) )  // if large angle then transpose
  { angle = 3.0*M_PI/2.0 - angle; //   to small ones ( <45deg )
	//offset = -offset;
  }
  WRAP_ANGLE_2PI( angle );

  //sometimes need to flip the line upside down
  if( is_angle_leftward(angle) )
  { WRAP_ANGLE_HALF_PLANE( angle );
    offset = -offset;
  }

  o = lround( ( offset - orng.min ) / orng.step );
  a = lround( (  angle - arng.min ) / arng.step );
  w = lround( (  width - wrng.min ) / wrng.step );
#ifdef DEBUG_HALF_SPACE_DETECTOR_BANK
  { int wrn = 0;
  printf("\tBANK: Fetching (%3d,%3d,%3d) o: %7.4g, a: %7.4g, w:%7.4g\n",
      o,a,w,
      o*orng.step + orng.min,
      a*arng.step + arng.min,
      w*wrng.step + wrng.min);
  if( offset - orng.max >  1e-3) {printf("Warning: Offset exceeded max. (%g)\n",offset); wrn=1;}
  if( offset - orng.min < -1e-3) {printf("Warning: Offset exceeded min. (%g)\n",offset); wrn=1;}
  if(  width - wrng.max >  1e-3) {printf("Warning:  Width exceeded max. (%g)\n",width);  wrn=1;}
  if(  width - wrng.min < -1e-3) {printf("Warning:  Width exceeded min. (%g)\n",width);  wrn=1;}
  if(  angle - arng.max >  1e-3) {printf("Warning:  Angle exceeded max. (%g)\n",angle);  wrn=1;}
  if(  angle - arng.min < -1e-3) {printf("Warning:  Angle exceeded min. (%g)\n",angle);  wrn=1;}
  if(wrn)
    breakme();
  }
#endif
  return Get_Half_Space_Detector( bank, o, w, a );
}

//TODO: impliment lookup table for offsets as possible optimization
#if 0
int *make_offset_list_lookup_table(Image *image, int support)
{ //stores offset list and number of in-bounds for each pixel in image, for high and low angles
  //  size of table is 4 * width * height * ( support * support + 1 )
  //               e.g.  4 *   300 *    400 * (19 * 19 + 1 )  * sizeof(uint16) 
  //                        = 347 MB
}
#endif

SHARED_EXPORT
int *get_offset_list( Image *image, int support, float angle, int p, int *npx )
  /* returns a static buffer with *npx integer pairs.  The integer pairs are
   * indices into the image and weight arrays such that:
   *
   * The following will perform the correlation of filter and image centered at
   * p in the * image (with the center of the filter as its origin).
   *
   *     int *pairs = get_offset_list(image, support, line, p, &npx);     
   *     float score = 0.0;                                               
   *     while(npx--)                                                     
   *      score += image->array[ pairs[2*npx] * filter[ pairs[2*npx+1] ]  
   *
   * A list of out-of-bounds pixels of the weight array is stored in:
   *
   *     area = support * support                                               
   *     pairs[ npx+1...a-1] // out of bound pairs, clamped to boarder  
   *
   *     // integrate over out-of-bouds
   *     int *pairs = get_offset_list(image, support, line, p, &npx);     
   *     while( npx++ <  area)                                                     
   *      score += image->array[ pairs[2*npx] * filter[ pairs[2*npx+1] ]  
   *
   */
{ static int *pxlist = (NULL);
  static int snpx = 0;
  static size_t maxsupport = 0;
  static int lastp = -1;
  static int last_issmallangle = -1;
  int i,j, issa;
  int half = support / 2;
  int px = p%(image->width),
      py = p/(image->width);
  int ioob = 2*support*support; // index for out-of-bounds pixels

  pxlist = (int*) request_storage( pxlist, &maxsupport, sizeof(int), 2*support*support, "pixel list" );

  issa = is_small_angle( angle );
  if( p != lastp || issa != last_issmallangle ) //recompute only if neccessary
  { int tx,ty,ww,hh,ox,oy;                     //  Neglects to check if support has changed
    //float angle = line->angle;
    ww = image->width;
    hh = image->height;
    ox = px - half;
    oy = py - half;
    lastp = p;
    last_issmallangle = issa;
    //lastangle = line->angle;

    snpx = 0;
    if( issa )
    { for( i=0; i<support; i++ )
      { ty = oy + i;
        if( (ty >= 0) && (ty < hh) )
        { for( j=0; j<support; j++ )
          { tx = ox + j;
            if( (tx >= 0) && (tx<ww) )
            { pxlist[ snpx++ ] = ww * ty + tx;    // image   pixel address
              pxlist[ snpx++ ] = support * i + j; // weights pixel address
            } 
          }
        }
        //oob
        for( j=0; j<support; j++ )
        { tx = ox + j;
          if( (ty<0) || (ty>=hh) || (tx < 0) || (tx>=ww) ) //out of bounds
          { 
            pxlist[ ioob-- ] = ww * MIN(MAX(0,ty),hh-1) + MIN(MAX(0,tx),ww-1); // clamps to border
            pxlist[ ioob-- ] = support * i + j; 
          }
        }
      }

    } else // large angle, do transpose
    { for( i=0; i<support; i++ )
      { tx = ox + i;
        if( (tx >= 0) && (tx < ww) )
        { for( j=0; j<support; j++ )
          { ty = oy + j;
            if( (ty >= 0) && (ty<hh) )
            { pxlist[ snpx++ ] = ww * ty + tx;
              pxlist[ snpx++ ] = support * i + j;
            }
          }
        }

		// put out of bounds pixels at end
        for( j=0; j<support; j++ )
        { ty = oy + j;
          if( (ty<0) || (ty>=hh) || (tx < 0) || (tx>=ww) ) //out of bounds
          { 
            pxlist[ ioob-- ] = ww * MIN(MAX(0,ty),hh-1) + MIN(MAX(0,tx),ww-1); // clamps to border
            pxlist[ ioob-- ] = support * i + j; 
          }
        }
      }
    } // end if/ angle check
  }
  *npx = snpx/2;
  return pxlist;
}

SHARED_EXPORT
float round_anchor_and_offset( Line_Params *line, int *p, int stride )
/* rounds pixel anchor, p, to pixel nearest center of line detector (rx,ry)
** returns best offset to line                   
**
** This moves the center of the detector a little bit since the line
**  is a bit overconstrained.  However, the size of the error can be 
**  bounded to less than the pixel size (proof?).
*/
{ float ex,ey,rx,ry,px,py;
  float ppx, ppy, drx, dry, t;     // ox, oy;
  ex  = cos(line->angle + M_PI/2); // unit vector normal to line
  ey  = sin(line->angle + M_PI/2);
  px  = (*p % stride );            // current anchor
  py  = (*p / stride );
  rx  = px + ex * line->offset;    // current position
  ry  = py + ey * line->offset;
  ppx = round( rx );               // round to nearest pixel as anchor
  ppy = round( ry );
  drx = rx - ppx;                  // ppx - rx;       // dr: vector from pp to r
  dry = ry - ppy;                  // ppy - ry;
  t   = drx*ex + dry*ey;           // dr dot e (projection along normal to line)
  
	// Max error is ~0.6 px

  //if( ppx != px || ppy != py)
  //{
	 // float rx2 = ppx + t * ex,
		//    ry2 = ppy + t * ey;
	 // float err = sqrt((rx2-rx)*(rx2-rx) + (ry2-ry)*(ry2-ry));
	 // printf("(%3d, %3d) off: %5.5f --> (%3d, %3d) off: %5.5f\terr:%g\n",
	 //			(int)px,(int)py,line->offset, (int)ppx, (int)ppy,t, err);
  //}
  *p = ((int)ppx) + stride*( (int) ppy );
  return t;
}

SHARED_EXPORT
int mean_uint8( Image *im )
{ float acc = 0.0;
  int a = im->width * im->height;
  uint8 *d = im->array + a;
  while(d > im->array)
    acc += *(--d);
  return acc/(float)a;
}

SHARED_EXPORT
int threshold_bottom_fraction_uint8( Image* im ) //, float fraction )
{ float acc, mean, lm;
  int a,i, count;
  uint8 *d;

  d = im->array;
  a = i = im->width * im->height;
  acc = 0.0f;
  while(i--)
    acc += *(d+i);
  mean = acc / (float)a;

  i = a;
  acc = 0.0f;
  count = 0;
  while(i--)
  { float v = *(d+i);
    if( v<mean )
    { acc += v;
      count ++;
    }
  }
  lm = acc / (float) count;

  return (int) lm;
}

SHARED_EXPORT
int threshold_upper_fraction_uint8( Image* im )
{ float acc, mean, lm, hm;
  int a,i, count;
  uint8 *d;

  d = im->array;
  a = i = im->width * im->height;
  acc = 0.0f;
  while(i--)
    acc += *(d+i);
  mean = acc / (float)a;

  i = a;
  acc = 0.0f;
  count = 0;
  while(i--)
  { float v = *(d+i);
    if( v>mean )
    { acc += v;
      count ++;
    }
  }
  lm = acc / (float) count;

  return (int) lm;
}

SHARED_EXPORT
float eval_half_space( Line_Params *line, Image *image, int p, float *rr, float *ll )
{ int i,support  = 2*TLEN + 3;
  int npxlist, a = support*support;
  
  int   *pxlist;
  float coff;
  float leftnorm, *lefthalf;
  float rightnorm, *righthalf;
  float  r,l,q       = 0.0;
  //static void *lastim = NULL;
  //static float bg = -1.0;

  // Out of bounds will be clamped to a constant corresponding to the bright
  // background
  //if( bg < 0.0 || lastim != image->array) /* memoize - recomputes when image changes */
  //{ bg = threshold_upper_fraction_uint8(image);
  //  lastim = image->array;
  //}

  coff = round_anchor_and_offset( line, &p, image->width );
  pxlist    = get_offset_list( image, support, line->angle, p, &npxlist );
  lefthalf  = get_nearest_from_half_space_detector_bank( coff, line->width, line->angle, &leftnorm );
  righthalf  = get_nearest_from_half_space_detector_bank( -coff, line->width, line->angle, &rightnorm );
  {
    uint8* parray = image->array;
    i = a; //npxlist;
    l = 0.0;
    r = 0.0;
    while(i--)
    {
      l += parray[pxlist[2*i]] * lefthalf[pxlist[2*i+1]];
      r += parray[pxlist[2*i]] * righthalf[a - pxlist[2*i+1]];
    }

  }

  //
  // Take averages
  //

  q = (r-l)/(r+l);
  r /= rightnorm;
  l /= leftnorm;

  //printf("\tHALF SPACE: left (%7.7g) right     (%7.7g) ratio(%5.5g) diff (%5.5g)\n",
  //            l,r,l/r,l-r);
  //printf("\t            sum  (%7.7g) q[right] (%7.7g)\n",
  //            l+r, q);

#ifdef SHOW_HALF_SPACE_DETECTOR
  { Image *im   = Make_Image(GREY8, image->width, image->height);
    Image *leftim, *rightim = Copy_Image( image  );
    Translate_Image( rightim, FLOAT32, 1 );
    leftim = Copy_Image( rightim );
    memset( im->array, 0, (image->width)*(image->height) );
    i = npxlist;
    while(i--)
    { im->array[ pxlist[2*i] ]   = image->array[ pxlist[2*i] ];
      ((float*)leftim->array) [ pxlist[2*i] ] =(255.0 - lefthalf[ pxlist[2*i+1] ]*image->array[ pxlist[2*i] ])/2.0;
      /* note: lefthalf is indexed differently to rotate it by 180 */
      ((float*)rightim->array)[ pxlist[2*i] ] =(255.0 - righthalf[ support*support - pxlist[2*i+1] ]*image->array[ pxlist[2*i] ])/2.0;
    }
    im->array[p] = 0;
    Scale_Image_To_Range( leftim, 0, 0, 255);
    Scale_Image_To_Range( rightim, 0, 0, 255);
    Translate_Image( leftim, GREY8, 1);
    Translate_Image( rightim, GREY8, 1);
    Write_Image( "halfspace_pxlist.tif", im );
    Write_Image( "halfspace_left.tif", leftim );
    Write_Image( "halfspace_right.tif", rightim );
    Write_Image( "halfspace_current.tif", image );
    Free_Image(im);
    Free_Image(leftim);
    Free_Image(rightim);
  }
#endif
  // 
  // Finish up
  //
  *ll = l; *rr = r;
  return q;
}

SHARED_EXPORT
int is_change_too_big( Line_Params *new, Line_Params *old, float alim, float wlim, float olim)
{ float dth = old->angle - new->angle,
        dw  = old->width - new->width,
        doff= old->offset- new->offset;
#ifdef DEBUG_LINE_FITTING
  if( fabs((dth*180.0/M_PI)) >= 5.0 || /* suspicious */
      fabs(dw) >= 1                 ||
      fabs(doff) >= 1               )
  {  printf("\t\tdth = %7.4g\tdw = %7.4g\t doff = %g\n",dth*180.0/M_PI,dw,doff);
     breakme();
  }
#endif
  if( ( fabs((dth*180.0/M_PI)) >  alim ) ||
      ( fabs(dw)               >  wlim ) ||
      ( fabs(doff)             >  olim ) )
  {
#ifdef DEBUG_LINE_FITTING
    printf("\t\tDelta too big.\n" );
#endif
    return 1;
  }
  return 0;
}

SHARED_EXPORT
int is_local_area_trusted_conservative( Line_Params *line, Image *image, int p )
{ float q,r,l;
  static float thresh = -1.0;
  static uint8 *lastim = NULL;
  q = eval_half_space( line, image, p, &r, &l );

  if( thresh < 0.0 || lastim != image->array) /* recomputes when image changes */
  { //thresh = mean_uint8( image );
    thresh = threshold_two_means( image->array, image->width*image->height );
    lastim = image->array;
  }
#ifdef SHOW_HALF_SPACE_DETECTOR
  debug("\t(%5d) q:%7.3g r:%7.3g l:%7.3g thresh:%7.3g\n",p,q,r,l,thresh);
#endif
  if( ((r < thresh) && (l < thresh )) ||
      (fabs(q) > HALF_SPACE_ASSYMETRY_THRESH) )
  { return 0;
  } else
  { return 1;
  }
}

SHARED_EXPORT
int is_local_area_trusted( Line_Params *line, Image *image, int p )
{ float q,r,l;
  static float thresh = -1.0;
  static void  *lastim = NULL;
  q = eval_half_space( line, image, p, &r, &l );

  if( thresh < 0.0 || lastim != image->array) /* recomputes when image changes */
  { thresh = threshold_bottom_fraction_uint8(image);//,HALF_SPACE_FRACTION_DARK );
    lastim = image->array;
  }
#ifdef SHOW_HALF_SPACE_DETECTOR
  debug("\t(%5d) q:%7.3g r:%7.3g l:%7.3g thresh:%7.3g\n",p,q,r,l,thresh);
#endif
  if( ((r < thresh) && (l < thresh )) ||
      (fabs(q) > HALF_SPACE_ASSYMETRY_THRESH) )
  { return 0;
  } else
  { return 1;
  }
}


SHARED_EXPORT
float  eval_line(Line_Params *line, Image *image, int p)
{ int i;
  const int support  = 2*TLEN + 3;
  int npxlist;
  
  int   *pxlist;
  float *weights, coff;

  float  r,l,q,s       = 0.0;
  static float lastscore = 0.0;
  static float bg     = -1.0; //background is bright
  static void *lastim = NULL;

  // compute a nearby anchor

  coff      = round_anchor_and_offset( line, &p, image->width );
  pxlist    = get_offset_list( image, support, line->angle, p, &npxlist );

  weights   = get_nearest_from_line_detector_bank ( coff, line->width, line->angle );

#ifdef SHOW_LINE_DETECTOR
#if 0 
#define fpart(a) (10.0*( a - round(a) ))
#else
#define fpart(a) (a)
#endif

  { Image *im   = Make_Image(GREY8, image->width, image->height);
    Image *scim = Make_Image(FLOAT32, image->width, image->height);
    Image *resim = Copy_Image( image  );
    Translate_Image( resim, FLOAT32, 1 );
    //inhib = Copy_Image( resim );
    memset( im->array, 0, (image->width)*(image->height) );
    memset( scim->array, 0, sizeof(float)*(image->width)*(image->height) );
    i = npxlist;
    while(i--)
    { im->array[ pxlist[2*i] ]   = image->array[ pxlist[2*i] ];
      ((float*)scim->array)[ pxlist[2*i] ] = weights[ pxlist[2*i+1] ];
      ((float*)resim->array)  [ pxlist[2*i] ] =(255.0 - fpart(weights [ pxlist[2*i+1] ])*image->array[ pxlist[2*i] ])/2.0;
    }
    im->array[p] = 0;
    Scale_Image_To_Range( scim, 0, 0, 255);
    Scale_Image_To_Range( resim, 0, 0, 255);
    Translate_Image( scim, GREY8, 1);
    Translate_Image( resim, GREY8, 1);
    Write_Image( "evalline_pxlist.tif", im );
    Write_Image( "evalline_weights.tif", scim );
    Write_Image( "evalline_scores.tif", resim );
    Write_Image( "evalline_current.tif", image );
    Free_Image(im);
    Free_Image(scim);
    Free_Image(resim);
  }
#endif

  { 
    uint8* parray = image->array;
#if 0
    s = integrate_special_by_labels( parray, weights, pxlist, npxlist );
    s = integrate_harmonic_mean_by_labels( parray, weights, pxlist, npxlist );
#endif
    s = 0.0;
    i = npxlist;
    while(i--)
      s += parray[pxlist[2*i]] * weights[pxlist[2*i+1]];
  }

  return -s;
}

SHARED_EXPORT
float  eval_line_no_debug(Line_Params *line, Image *image, int p)
{ int i,support  = 2*TLEN + 3;
  int npxlist, a = support*support;
  int o;
  int   *pxlist;
  float *weights, coff;
  float leftnorm, *lefthalf;
  float  r,l,q,s       = 0.0;
  static float lastscore = 0.0;
  static float bg     = -1.0; //background is bright
  static void *lastim = NULL;
  
  // compute a nearby anchor
  coff = round_anchor_and_offset( line, &p, image->width );
  pxlist    = get_offset_list( image, support, line->angle, p, &npxlist );
  weights   = get_nearest_from_line_detector_bank      ( coff, line->width, line->angle );

  { 
    uint8* parray = image->array;
#if 0
    s = integrate_special_by_labels( parray, weights, pxlist, npxlist );
    s = integrate_harmonic_mean_by_labels( parray, weights, pxlist, npxlist );
#endif
    s = 0.0;
    i = npxlist;
    while(i--)
      s += parray[pxlist[2*i]] * weights[pxlist[2*i+1]];
  }
  return -s;
}

#if 0
float  eval_line_by_curved_detector(Line_Params *line, Image *image, int p)
{ int i,support  = 2*TLEN + 3;
  int npxlist, a = support*support;
  int o;
  int   *pxlist;
  float *weights, coff;
  float leftnorm, *lefthalf;
  float  r,l,q,s       = 0.0;
  static float lastscore = 0.0;
  static float bg     = -1.0; //background is bright
  static void *lastim = NULL;
  

  // compute a nearby anchor
  coff = round_anchor_and_offset( line, &p, image->width );
  pxlist    = get_offset_list( image, support, line->angle, p, &npxlist );
  weights   = get_nearest_from_curved_line_detector_bank( coff, line->width, line->angle );

#ifdef SHOW_LINE_DETECTOR
  { Image *im   = Make_Image(GREY8, image->width, image->height);
    Image *scim = Make_Image(FLOAT32, image->width, image->height);
    Image *resim = Copy_Image( image  );
    Translate_Image( resim, FLOAT32, 1 );
    memset( im->array, 0, (image->width)*(image->height) );
    memset( scim->array, 0, sizeof(float)*(image->width)*(image->height) );
    i = npxlist;
    while(i--)
    { im->array[ pxlist[2*i] ]   = image->array[ pxlist[2*i] ];
      ((float*)scim->array)[ pxlist[2*i] ] = weights[ pxlist[2*i+1] ];
      ((float*)resim->array)  [ pxlist[2*i] ] =(255.0 - weights [ pxlist[2*i+1] ]*image->array[ pxlist[2*i] ])/2.0;
    }
    im->array[p] = 0;
    Scale_Image_To_Range( scim, 0, 0, 255);
    Scale_Image_To_Range( resim, 0, 0, 255);
    Translate_Image( scim, GREY8, 1);
    Translate_Image( resim, GREY8, 1);
    Write_Image( "evalline_pxlist.tif", im );
    Write_Image( "evalline_weights.tif", scim );
    Write_Image( "evalline_scores.tif", resim );
    Write_Image( "evalline_current.tif", image );
    Free_Image(im);
    Free_Image(scim);
    Free_Image(resim);
  }
#endif

  { 
    uint8* parray = image->array;
    s = 0.0;
    i = npxlist;
    while(i--)
      s += parray[pxlist[2*i]] * weights[pxlist[2*i+1]];
  }
  return -s;
}
#endif

SHARED_EXPORT
int adjust_line_walk(Line_Params *line, Image *image, int *pp,
    Interval *roff, Interval *rang, Interval *rwid)
{ double hpi = acos(0.)/2.;
  double ain = hpi/ANGLE_STEP;
  double rad = 45./hpi;

  double x, v, best, last;
  int    better, isanglesmall;
  int    stride = image->width;
  int    p = *pp;

  Line_Params backup = *line;

  best   = line->score;
  last   = best;
  better = 1;
  while (better)
  { better = 0;

#ifdef  DEBUG_LINE_FITTING
    printf("        %.1f(%3d,%3d) %6.1f %4.1f: S=%g\n",
        line->offset,p%image->width,p/image->width,line->angle*rad,line->width,best);
#endif
    /* adjust top line with bottom line constant*/
    last = best;

   /* 
     * adjust offset 
     * */
    last = best;
    x = line->offset;
    do
    { line->offset -= OFFSET_STEP;
      v = eval_line(line,image,p);
    } while( fabs(v - last) < 1e-5 &&  line->offset >= roff->min);
    if (((v - best) > 1e-5)        && (line->offset >= roff->min))
    { best   =  v;
      better =  1;
    }
    else
    { line->offset = x;
      do
      { line->offset += OFFSET_STEP;
        v = eval_line(line,image,p);
      } while( fabs(v - last) < 1e-5 && line->offset <= roff->max);
      if (((v - best) > 1e-5)        && line->offset <= roff->max)
      { best   =  v;
        better =  1;
      }
      else
        line->offset = x;
    }

   /* 
     * adjust width 
     * */
    last = best;
    x = line->width;
    do
    { line->width -= WIDTH_STEP;
      v = eval_line(line,image,p);
    } while( fabs(v - last) < 1e-5 && line->width  >= rwid->min);
    if (((v - best) > 1e-5)        && (line->width >= rwid->min))
    { best   =  v;
      better =  1;
    }
    else
    { line->width = x;
      do
      { line->width += WIDTH_STEP;
        v = eval_line(line,image,p);
      } while( fabs(v - last) < 1e-5 && line->width <= rwid->max);
      if (((v - best) > 1e-5)        && line->width <= rwid->max)
      { best   =  v;
        better =  1;
      }
      else
        line->width = x;
    }

#if 0
    { double  ot = line->width,
              oo = line->offset,
              or = oo + ot,
              r  = or,      // the variable line
              cr = oo - ot; // the constant line
      do {
        r += 2*OFFSET_STEP;
        line->offset = (r + cr) / 2.0;
        line->width  = (r - cr) / 2.0;
        v = eval_line( line, image, p );
      } while( fabs(v-last) < 1e-5 && line->width <  rwid->max && line->offset <  roff->max );
      if( v > best                 && line->width <= rwid->max && line->offset <= roff->max )
      { best = v;
        better = 1;
      } else if ( or > OFFSET_STEP )
      { r = or;
        do {
          r -= 2*OFFSET_STEP;
          line->offset = (r + cr) / 2.0;
          line->width  = (r - cr) / 2.0;
          v = eval_line( line, image, p );
        } while( fabs(v-last) < 1e-5 && line->width >  rwid->min && line->offset >  roff->min );
        if( v > best                 && line->width >= rwid->min && line->offset >= roff->min )
        { best = v;
          better = 1;
        } else
        { line->offset = (or+cr)/2.0;
          line->width  = (or-cr)/2.0;
        }
      }
    }

    /* adjust bottom line with top line constant */
    last = best;

    { double  ot = line->width,
              oo = line->offset,
              or = oo - ot, // is negative
              r  = or,      // the variable line
              cr = oo + ot; // the constant line
      do {
        r -= 2*OFFSET_STEP;
        line->offset = (cr + r) / 2.0;
        line->width  = (cr - r) / 2.0;
        v = eval_line( line, image, p );
      } while( fabs(v-last) < 1e-5 && line->width <  rwid->max && line->offset >  roff->min );
      if( v > best                 && line->width <= rwid->max && line->offset >= roff->min )
      { best = v;
        better = 1;
      } else if ( or < -OFFSET_STEP ) //or is negative now
      { r = or;
        do {
          r += 2*OFFSET_STEP;
          line->offset = (cr + r) / 2.0;
          line->width  = (cr - r) / 2.0;
          v = eval_line( line, image, p );
        } while( fabs(v-last) < 1e-5 && line->width >  rwid->min && line->offset <  roff->max );
        if( v > best                 && line->width >= rwid->min && line->offset <= roff->max )
        { best = v;
          better = 1;
        } else
        { line->offset = (cr+or)/2.0;
          line->width  = (cr-or)/2.0;
        }
      }
    }
#endif
    line->score = best;
  }
  

  if( is_change_too_big( &backup, line, MAX_DELTA_ANGLE, MAX_DELTA_WIDTH, MAX_DELTA_OFFSET ) )
  {      
    *line = backup; //No adjustment
    return 0;
  }

  line->score = best;
  *pp = p;
  return 1;
}

SHARED_EXPORT
Line_Params *adjust_line_exhaustive( Line_Params *line, Image *image, int *pp,
                                     Interval *roff, Interval *rang, Interval *rwid)
{ double hpi = acos(0.)/2.;
  double ain = hpi/ANGLE_STEP;
  double rad = 45./hpi;
  int p = *pp;

  double x, v, best = eval_line(line,image,p);
  Line_Params cur = *line;

  for( cur.offset = roff->min; cur.offset <= roff->max; cur.offset += OFFSET_STEP )
  { for( cur.angle = rang->min; cur.angle <= rang->max; cur.angle += ain )
    { for( cur.width = rwid->min; cur.width <= rwid->max; cur.width += WIDTH_STEP )
      { v = eval_line( &cur, image, p );
        if( v > best )
        { best = v;
          line->angle = cur.angle;
          line->offset = cur.offset;
          line->width = cur.width;
          line->score = best;
        }
      }
    }
  }
  return line;
}

SHARED_EXPORT
int interval_size(Interval *r, double step)
{ // interval is inclusive of bounds
  //printf("[%5.5g, %5.5g] by %g\n", r->min, r->max, step );

  //return floor( ( r->max - r->min )/step ) + 1;
  
  // I must be retarded
  { int count = 0;
    double i;
    for( i=r->min; i<=r->max+1e-3*step; i += step )
      count++;
    return count;
  }
    
}

SHARED_EXPORT
void save_response(char *filename, Image *image, int p )
{ Line_Params cur;
  Interval roff, rang, rwid;
  double ain = M_PI/ANGLE_STEP/4.0;

  cur.angle = 0;
  initialize_paramater_ranges( &cur , &roff, &rang, &rwid );
  {
    int noff = interval_size( &roff, OFFSET_STEP ),
        nang = interval_size( &rang, ain         ),
        nwid = interval_size( &rwid, WIDTH_STEP  );
    Stack *stk = Make_Stack(4, nwid, nang, noff);
    float *data = (float*) stk->array;
    { int c = 0;
      for( cur.offset = roff.min; cur.offset <= roff.max+1e-3*OFFSET_STEP; cur.offset += OFFSET_STEP )
        c++;
      printf("\toff: %5d:%5d\n",noff,c);
      c=0;
      for( cur.angle = rang.min; cur.angle <= rang.max+1e-3*ain; cur.angle += ain )
        c++;
      printf("\tang: %5d:%5d\n",nang,c);
      c=0;
      for( cur.width = rwid.min; cur.width <= rwid.max+1e-3*WIDTH_STEP; cur.width += WIDTH_STEP )
        c++;
      printf("\twid: %5d:%5d\n",nwid,c);
    }
    for( cur.offset = roff.min; cur.offset <= roff.max+1e-3*OFFSET_STEP; cur.offset += OFFSET_STEP )
      for( cur.angle = rang.min; cur.angle <= rang.max+1e-3*ain; cur.angle += ain )
        for( cur.width = rwid.min; cur.width <= rwid.max+1e-3*WIDTH_STEP; cur.width += WIDTH_STEP )
          *(data++) = eval_line_no_debug( &cur, image, p );

    Scale_Stack_To_Range( stk, 0, 0.0, 255.0 );
    Translate_Stack(stk,1,1);
    Write_Stack("response.tif",stk);
    //{ FILE *fp = fopen(filename,"wb");
    //  fwrite( stk->array, stk->kind, noff * nang * nwid, fp );
    //  fclose(fp);
    //}

    Free_Stack( stk ); 
  }
}

SHARED_EXPORT
void get_response_extents( int *noff, int *nang, int *nwid )
{ Line_Params cur;
  Interval roff, rang, rwid;
  double ain = M_PI/ANGLE_STEP/4.0;

  cur.angle = 0;
  initialize_paramater_ranges( &cur , &roff, &rang, &rwid );
  *noff = interval_size( &roff, OFFSET_STEP );
  *nang = interval_size( &rang, ain         );
  *nwid = interval_size( &rwid, WIDTH_STEP  );
}

SHARED_EXPORT
void get_response_axes_ticks( float *x, float *y, float* z )
{ Line_Params cur;
  Interval roff, rang, rwid;
  double ain = M_PI/ANGLE_STEP/4.0;

  cur.angle = 0;
  initialize_paramater_ranges( &cur , &roff, &rang, &rwid );
  for( cur.offset = roff.min; cur.offset <= roff.max + 1e-3*OFFSET_STEP; cur.offset += OFFSET_STEP )
    *(z++) = cur.offset;
  for( cur.angle = rang.min; cur.angle <= rang.max + 1e-3*ain; cur.angle += ain )
    *(y++) = cur.angle;
  for( cur.width = rwid.min; cur.width <= rwid.max + 1e-3*WIDTH_STEP; cur.width += WIDTH_STEP )
    *(x++) = cur.width;
}

SHARED_EXPORT
void compute_dxdy( Line_Params *line, float *dx, float *dy )
{float ex,ey;
  ex  = cos(line->angle + M_PI/2);  // unit vector normal to line
  ey  = sin(line->angle + M_PI/2);
  *dx = ex * line->offset; // current position
  *dy = ey * line->offset;
}

SHARED_EXPORT
void Print_Position(Line_Params *line, int p, int stride)
{ float dx,dy;
  int x = p%stride,
	  y = p/stride;
  compute_dxdy(line,&dx,&dy);
  printf("(%3d%+3.2f, %3d%+3.2f)  offset = %3.2f\tangle = %f\n", 
	  x, dx, y, dy, line->offset, line->angle*180/M_PI );

}

SHARED_EXPORT
void get_response( float *buffer, Image *image, int p)
{ Line_Params cur;
  Interval roff, rang, rwid;
  double ain = M_PI/ANGLE_STEP/4.0;

  cur.angle = 0;
  initialize_paramater_ranges( &cur , &roff, &rang, &rwid );
  for( cur.offset = roff.min; cur.offset <= roff.max + 1e-3*OFFSET_STEP; cur.offset += OFFSET_STEP )
    for( cur.angle = rang.min; cur.angle <= rang.max + 1e-3*ain; cur.angle += ain )
      for( cur.width = rwid.min; cur.width <= rwid.max + 1e-3*WIDTH_STEP; cur.width += WIDTH_STEP )
        *(buffer++) = eval_line_no_debug( &cur, image, p );
}

SHARED_EXPORT
int adjust_line_start_old(Line_Params *line, Image *image, int *pp,
    Interval *roff, Interval *rang, Interval *rwid)
{ double hpi = acos(0.)/2.;
  double ain = hpi/ANGLE_STEP;
  double rad = 45./hpi;
  int trusted = 1;

  double x, v, best, last;
  int    better;
  int p = *pp;

  double atest = line->angle;
  Line_Params backup = *line;

  printf("\n");
  Print_Position(line, *pp, image->width);
  better = 1;
  while (better)
  { int i,ibest;
    float ca,co;
    float angles[] =  {   ain,    ain,   ain,
                        - ain,  - ain, - ain };
    float offsets[] = { OFFSET_STEP, 0.0, - OFFSET_STEP,
                        OFFSET_STEP, 0.0, - OFFSET_STEP };
    better = 0;
#ifdef  DEBUG_LINE_FITTING
    printf("      %.1f(%3d) %6.1f %4.1f: S=%g\n",
        line->offset,p/image->width,line->angle*rad,line->width,line->score);
#endif
    // optimize offset/thickness first 
    if( ! adjust_line_walk( line, image, pp, roff, rang, rwid ) )
		return 0; //not trusted 
    best = line->score;

	i=6;
	ca = line->angle;
	co = line->offset;
	while(i--)
	{	angles[i]  += ca;
		offsets[i] += co;
	}

    // Test moves in offset-angle plane.  Take first better.
    ibest = -1;
    for( i=0; i<6; i++ )
    { line->angle  = angles[i];
      line->offset = offsets[i];
	  if( (line->angle  >= rang->min) &&
		  (line->angle  <= rang->max) &&
		  (line->offset >= roff->min) &&
		  (line->offset <= roff->max) )
	  { v = eval_line( line, image, p );
	  printf("\tv:%+5.5f\tbest:%+5.5f : (%5.5g, %3.3g)\n",v,best, line->angle *180.0/M_PI, line->offset);
  	    if ((v - best) > 1e-5)
		{ best   =  v;
		  better =  1;
		  ibest  =  i;
		  break;
		}
	  }
    }
    if( ibest == -1 ) // no improvement
    { printf("No Improvement in (angle,offset)\n");
	  line->angle = ca;
      line->offset = co;
    } // else line is alread set

    line->score = best;
  } //end while

  if( is_change_too_big( &backup, line, MAX_DELTA_ANGLE, MAX_DELTA_WIDTH, MAX_DELTA_OFFSET ) )
  { printf("Change too big\n");     
    *line = backup; //No adjustment
    return 0;
  }

  //Print_Position(line, *pp, image->width);
  *pp = p; //restore
  //line->offset = round_anchor_and_offset( line, pp, image->width );
  Print_Position(line, *pp, image->width);
  printf("Done!\n");
  return trusted;
}


SHARED_EXPORT
int adjust_line_start(Line_Params *line, Image *image, int *pp,
                               Interval *roff, Interval *rang, Interval *rwid)
{ double hpi = acos(0.)/2.;
  double ain = hpi/ANGLE_STEP;
  double rad = 45./hpi;
  int trusted = 1;

  double x, v, best, last;
  int    better;
  int p = *pp;

  double atest = line->angle;
  Line_Params backup = *line;

  better = 1;
  while (better)
  { better = 0;
#ifdef  DEBUG_LINE_FITTING
    printf("      %.1f(%3d) %6.1f %4.1f: S=%g\n",
            line->offset,p/image->width,line->angle*rad,line->width,line->score);
#endif
    best = line->score;
  
    /* adjust angle */
    /* when the angle switches from small to large around 45 deg, the meaning
     * off the offset changes.  But at 45 deg, the x-offset and the y-offset
     * are the same.
     */
    last = best;
    x = line->angle;
    do
    { line->angle -= ain;
      v = eval_line(line,image,p);
    } while( fabs(v - last) < 1e-5 && line->angle >= rang->min);
    if (         (v - best) > 1e-5 && line->angle >= rang->min)
    { best   =  v;
      better =  1;
    }
    else
    { line->angle = x;
      do
      { line->angle += ain;
        v = eval_line(line,image,p);
      } while( fabs(v - last) < 1e-5  && line->angle <= rang->max);
      if (         (v - best) > 1e-5  && line->angle <= rang->max)
      { best   =  v;
        better =  1;
      }
      else
        line->angle = x;
    }
    
    /* 
     * adjust offset 
     * */
    last = best;
    x = line->offset;
    do
    { line->offset -= OFFSET_STEP;
      v = eval_line(line,image,p);
    } while( fabs(v - last) < 1e-5 && line->offset >= roff->min);
    if (         (v - best) > 1e-5 && line->offset >= roff->min)
    { best   =  v;
      better =  1;
    }
    else
    { line->offset = x;
      do
      { line->offset += OFFSET_STEP;
        v = eval_line(line,image,p);
      } while( fabs(v - last) < 1e-5 && line->offset <= roff->max);
      if (         (v - best) > 1e-5 && line->offset <= roff->max)
      { best   =  v;
        better =  1;
      }
      else
        line->offset = x;
    }

    /* 
     * adjust width 
     * */
    last = best;
    x = line->width;
    do
    { line->width -= WIDTH_STEP;
      v = eval_line(line,image,p);
    } while( fabs(v - last) < 1e-5 && line->width >= rwid->min);
    if (         (v - best) > 1e-5 && line->width >= rwid->min)
    { best   =  v;
      better =  1;
    }
    else
    { line->width = x;
      do
      { line->width += WIDTH_STEP;
        v = eval_line(line,image,p);
      } while( fabs(v - last) < 1e-5 && line->width <= rwid->max);
      if (         (v - best) > 1e-5 && line->width <= rwid->max)
      { best   =  v;
        better =  1;
      }
      else
        line->width = x;
    }

    line->score = best;
  }
  

  if( is_change_too_big( &backup, line, MAX_DELTA_ANGLE, MAX_DELTA_WIDTH, MAX_DELTA_OFFSET ) )
  {      
    *line = backup; //No adjustment
    return 0;
  }

  *pp = p;
  return trusted;
}

SHARED_EXPORT
int move_line( Line_Params *line, int *p, int stride, int direction )
{ float lx,ly,ex,ey,rx0,ry0,rx1,ry1;
  float ppx, ppy, drx, dry, t, ox, oy;
  float th = line->angle;
  lx  = cos(th);           // unit vector along direction of line
  ly  = sin(th);
  ex  = cos(th + M_PI/2);  // unit vector normal to line
  ey  = sin(th + M_PI/2);
  rx0 = (*p % stride ) + ex * line->offset; // current position
  ry0 = (*p / stride ) + ey * line->offset;
  rx1 = rx0 + direction * lx;        // step to next position
  ry1 = ry0 + direction * ly;
  ppx = round( rx1 );    // round to nearest pixel as anchor
  ppy = round( ry1 );    //   (largest error ~0.6 px and lies along direction of line)
  drx = rx1 - ppx; //ppx - rx1; //rx0 - ppx;       // vector from pp to r1
  dry = ry1 - ppy; //ppy - ry1; //ry0 - ppy;
  t   = drx*ex + dry*ey; // dr dot l

  line->offset = t;
  *p = ((int)ppx) + stride*( (int) ppy );
#ifdef DEBUG_MOVE
  printf("\t*** Step to: (%3d,%3d) offset: %7.5g\n",*p%stride,*p/stride,line->offset);
#endif
  return *p;
}

SHARED_EXPORT
int  detect_loops(int p, float o)
{ static int phistory[10] = {-1,-1,-1,-1,-1,
                             -1,-1,-1,-1,-1};
  static float ohistory[10] = {-5.0,-5.0,-5.0,-5.0,-5.0,
                               -5.0,-5.0,-5.0,-5.0,-5.0};
  int i,n = 10;
  i = n;
  while(--i)
    if( (p == phistory[i]) && (fabs(o - ohistory[i])<0.1) )
    { breakme();
      break;
    }
  while(--n)
  { phistory[n] = phistory[n-1];
    ohistory[n] = ohistory[n-1];
  }
  phistory[0] = p;
  ohistory[0] = o;
  if(i) fprintf(stderr," WARNING: Loop detected during tracing (i=%d)\n",i);
  return i;
}

SHARED_EXPORT
void initialize_paramater_ranges( Line_Params *line, Interval *roff, Interval *rang, Interval *rwid)
{
    rwid->min = 0.5;
    rwid->max = 3.0;
    roff->min = -2.5;
    roff->max =  2.5;
    rang->min = line->angle - M_PI; 
    rang->max = line->angle + M_PI;
}

SHARED_EXPORT
Whisker_Seg *trace_whisker(Seed *s, Image *image)
{ typedef struct { float x; float y; float thick; float score; } record;
  static record *ldata, *rdata;
  static size_t maxldata = 0, maxrdata = 0;

  int nleft = 0, nright = 0;
  float x,y,dx,dy,newoff;
  int cwidth  = image->width,
      cheight = image->height;
  Line_Params line,rline,oldline;
  int trusted = 1;

  const double hpi = M_PI/4.0;
  const double ain = hpi/ANGLE_STEP;
  const double rad = 45./hpi;
  const double sigmin = (2*TLEN+1)*MIN_SIGNAL;// + 255.00;

  //printf("***********************************************\n");
  x = s->xpnt;
  y = s->ypnt;
  { int      q,p = x + cwidth*y;
    int      oldp;
    Interval roff, rang, rwid;

#ifdef SHOW_WHISKER_TRACE
    printf("(%d,%d) -> %g [%g]\n",
        s->xpnt,
        s->ypnt,
        atan(s->ydir/(1.*s->xdir))*rad,
        round(atan2(s->ydir,(1.*s->xdir)) / ain) * ain * rad  );
#endif
    /*
     *  init
     */

    if( ldata ) memset( ldata, 0, maxldata );
    if( rdata ) memset( rdata, 0, maxrdata );

    line = line_param_from_seed( s );

    initialize_paramater_ranges( &line, &roff, &rang, &rwid);

    //Must start in a trusted area
    if( !is_local_area_trusted_conservative( &line, image, p ) )
      return (NULL);

    line.score = eval_line( &line, image, p );
    adjust_line_start(&line,image,&p,&roff,&rang,&rwid);

    ldata = (record*) request_storage( ldata, &maxldata, sizeof(record), nleft+1, "trace whisker 1" );
    compute_dxdy( &line, &dx, &dy);
    { record trec = {p%cwidth + dx, p/cwidth + dy, line.width, line.score };
      ldata[nleft++] = trec;
    }

#ifdef SHOW_WHISKER_TRACE
    printf("    %3d,%3d+%.1f %6.1f %4.1f: %g\n",
        p%cwidth,p/cwidth,line.offset,line.angle*rad,line.width,line.score);
#endif

    q = p;
    rline = line;

    /*
     * Move forward from seed
     */
    while( line.score > sigmin )
    { move_line( &line, &p, cwidth, 1 );
#if 0 //#ifdef SHOW_WHISKER_TRACE
      save_response("response.raw", image, p );
#endif
      if( outofbounds(p, cwidth, cheight) ) break;
      line.score = eval_line( &line, image, p );
      oldline = line;
      oldp    = p;
      trusted = adjust_line_start(&line,image,&p,&roff,&rang,&rwid);
      { int nmoves = 0;
        trusted = trusted && is_local_area_trusted( &line, image, p );
        while( !trusted /*&& score > sigmin*/ && nmoves < HALF_SPACE_TUNNELING_MAX_MOVES)
        { oldline = line; oldp = p;
          move_line( &line, &p, cwidth, 1 );
          nmoves ++;
          if( outofbounds(p, cwidth, cheight) ) break;
          trusted = is_local_area_trusted( &line, image, p );
          trusted &= adjust_line_start(&line,image,&p,&roff,&rang,&rwid);
          if(trusted && line.score < sigmin) 
          { // check to see if a line can be reaquired
            Seed *sd = compute_seed_from_point( image, p, 3.0);
            if(sd)
            { line = line_param_from_seed(sd);
              if( line.angle * oldline.angle < 0.0 ) //make sure points in same direction
                line.angle *= -1.0;
            }
            line.score = eval_line( &line, image, p );
            trusted = adjust_line_start(&line,image,&p,&roff,&rang,&rwid);
            if( !trusted || 
                line.score < sigmin || 
                !is_local_area_trusted( &line, image, p ) ||
                is_change_too_big(&line,&oldline, 2*MAX_DELTA_ANGLE, 10.0, 10.0) ) 
            { trusted = 0;    // nothing found, back up
              break;
            }
          }
         // score *= HALF_SPACE_TUNNELING_DECAY;
        }
        if( !trusted )
        {   p = oldp;
            line = oldline;
            break;
        }
      }

      ldata = (record*) request_storage( ldata, &maxldata, sizeof(record), nleft+1, "trace whisker 2" );
      compute_dxdy( &line, &dx, &dy);
      { record trec = {p%cwidth + dx, p/cwidth + dy, line.width, line.score };
        ldata[nleft++] = trec;
      }

#ifdef SHOW_WHISKER_TRACE
      printf("  +:%d\n",nleft);
      printf("    %3d,%3d+%.1f %6.1f %4.1f: %g\n",
          p%cwidth,p/cwidth,line.offset,line.angle*rad,line.width,
          line.score);
#endif
#ifdef DEBUG_DETECT_LOOPS
      if(detect_loops(p,line.offset) > 5)
        break; //terminate trace
#endif
    }

    /*
     * Move backward from seed
     */
    line = rline;
    p = q;
    nright = 0;
    while( line.score > sigmin )
    { move_line( &line, &p, cwidth, -1 );
#ifdef SHOW_WHISKER_TRACE
      save_response("response.raw", image, p );
#endif
      if( outofbounds(p, cwidth, cheight) ) break;
      line.score = eval_line( &line, image, p );
      trusted = adjust_line_start(&line,image,&p,&roff,&rang,&rwid);

      { int nmoves = 0;
        trusted = trusted && is_local_area_trusted( &line, image, p );
        //float score = line.score;
        while( !trusted /*&& score > sigmin*/ && nmoves < HALF_SPACE_TUNNELING_MAX_MOVES )
        { oldline = line; oldp = p;
          move_line( &line, &p, cwidth, -1 );
          nmoves ++;
          if( outofbounds(p, cwidth, cheight) ) break;
          trusted = is_local_area_trusted( &line, image, p );
          trusted &= adjust_line_start(&line,image,&p,&roff,&rang,&rwid);
          if(trusted && line.score < sigmin) 
          { // check to see if a line can be reaquired
            Seed *sd = compute_seed_from_point( image, p, 3.0); // this will often pop the line back on
            if(sd) // else just use last line
            { 
              line = line_param_from_seed(sd);
              if( line.angle * oldline.angle < 0.0 ) //make sure points in same direction
                line.angle *= -1.0;
            }
            line.score = eval_line( &line, image, p );
            trusted = adjust_line_start(&line,image,&p,&roff,&rang,&rwid);
            if( !trusted || 
                line.score < sigmin || 
                ! is_local_area_trusted( &line, image, p )  ||
                is_change_too_big(&line,&oldline, 2*MAX_DELTA_ANGLE, 10.0, 10.0 ) ) 
            { trusted = 0;  // nothing found, back up
              break;
            }
          }
        }
        if( !trusted )
        {   p = oldp;
            line = oldline;
            break;
        }
      }

      rdata = (record*) request_storage( rdata, &maxrdata, sizeof(record), nright+1, "trace whisker 3" );
      compute_dxdy( &line, &dx, &dy);
      { record trec = {p%cwidth + dx, p/cwidth + dy, line.width, line.score };
        rdata[nright++] = trec;
      }

#ifdef SHOW_WHISKER_TRACE
      printf("  -:%d\n",nright);
      printf("    %3d,%3d+%.1f %6.1f %4.1f: %g\n",
          p%cwidth,p/cwidth,line.offset,line.angle*rad,line.width,
          line.score);
#endif
#ifdef DEBUG_DETECT_LOOPS
      if(detect_loops(p,line.offset) > 5)
        break; //terminate trace
#endif
    }
  }

  /*
   * Copy results into a whisker segment
   */
  if( nright+nleft > 2*TLEN )
  { Whisker_Seg *wseg = Make_Whisker_Seg( nright + nleft );
    int j=0, i = nright;
    while( i-- )  /* backward copy */
    { wseg->x     [j] = rdata[i].x;
      wseg->y     [j] = rdata[i].y;
      wseg->thick [j] = rdata[i].thick;
      wseg->scores[j] = rdata[i].score;
      j++;
    }
    for( i=0; i< nleft; i++ )
    { wseg->x     [j] = ldata[i].x;
      wseg->y     [j] = ldata[i].y;
      wseg->thick [j] = ldata[i].thick;
      wseg->scores[j] = ldata[i].score;
      j++;
    }
    return wseg;
  } else
  { return (NULL);
  }
}
