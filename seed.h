#ifndef _H_SEED
#define _H_SEED

#include "compat.h"
#include "image_lib.h"
#include "contour_lib.h"

#include "parameters/param.h"

//#define MIN_LENGTH  20     //  If span of object is not 20 pixels will not use as a seed
//#define MIN_LENSQR 100     //  (MIN_LENGTH/2)^2
//#define MIN_LENPRJ  14     //  floor(MIN_LENGTH/sqrt(2))

typedef struct _Object_Map
  { int       num_objects;
    Contour **objects;
  } Object_Map;

typedef struct _Raster
  { short major;
    short minor;
  } Raster;

typedef struct _Seed
  { int  xpnt, ypnt;
    int  xdir, ydir;
  } Seed;

typedef struct _Seed_Vector
  { int   nseeds;
    Seed *seeds;
  } Seed_Vector;

SHARED_EXPORT  Object_Map*   find_objects      (Image *image, int vthresh, int sthresh);                       
SHARED_EXPORT  Seed*         compute_seed      (Raster *raster, int n, int x0, int width, uint8 *value);       
SHARED_EXPORT  Seed_Vector*  decompose_trace_x (Contour *trace, int width, int height, uint8 *value);          
SHARED_EXPORT  Seed_Vector*  find_seeds        (Contour *trace, Image *image);                                 
                                                                                                               
SHARED_EXPORT  Seed*         compute_seed_from_point           ( Image *image, int p, int maxr );              
SHARED_EXPORT  Seed*         compute_seed_from_point_ex        ( Image *image, int p, int maxr,                
                                                                 float *out_m, float *out_stat);               
SHARED_EXPORT  void          compute_seed_from_point_histogram ( Image *image, int maxr, Image *hist);         
SHARED_EXPORT  void          compute_seed_from_point_field     ( Image *image, int maxr, Image *hist,          
                                                                 Image *slopes, Image *stats);                 
SHARED_EXPORT  void          compute_seed_from_point_field_windowed_on_contour                                 
                                                               ( Image *image, Contour *trace,                 
                                                                 int maxr, float statlow, float stathigh,      
                                                                 Image *hist, Image *slopes, Image *stats );   
SHARED_EXPORT  void compute_seed_from_point_field_on_grid( Image *image, int spacing,                          
                                                           int maxr, float statlow, float stathigh,            
                                                           Image *hist, Image *slopes, Image *stats );         
SHARED_EXPORT  Seed_Vector*  find_seeds2       ( Contour *trace, Image *image );                               
#endif //  _H_SEED
