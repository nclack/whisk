#ifndef _H_SEED
#define _H_SEED

#include "image_lib.h"
#include "contour_lib.h"

#define MIN_LENGTH  20     //  If span of object is not 20 pixels will not use as a seed
#define MIN_LENSQR 100     //  (MIN_LENGTH/2)^2
#define MIN_LENPRJ  14     //  floor(MIN_LENGTH/sqrt(2))

typedef struct
  { int       num_objects;
    Contour **objects;
  } Object_Map;

typedef struct
  { short major;
    short minor;
  } Raster;

typedef struct
  { int  xpnt, ypnt;
    int  xdir, ydir;
  } Seed;

typedef struct
  { int   nseeds;
    Seed *seeds;
  } Seed_Vector;

Object_Map*   find_objects      (Image *image, int vthresh, int sthresh);
Seed*         compute_seed      (Raster *raster, int n, int x0, int width, uint8 *value);
Seed_Vector*  decompose_trace_x (Contour *trace, int width, int height, uint8 *value);
Seed_Vector*  find_seeds        (Contour *trace, Image *image);

Seed*         compute_seed_from_point           ( Image *image, int p, int maxr );
Seed*         compute_seed_from_point_ex        ( Image *image, int p, int maxr, 
                                                  float *out_m, float *out_stat);
void          compute_seed_from_point_histogram ( Image *image, int maxr, Image *hist);
void          compute_seed_from_point_field     ( Image *image, int maxr, Image *hist, 
                                                  Image *slopes, Image *stats);
void          compute_seed_from_point_field_windowed_on_contour
                                                ( Image *image, Contour *trace,
                                                  int maxr, float statlow, float stathigh,
                                                  Image *hist, Image *slopes, Image *stats );
void compute_seed_from_point_field_on_grid( Image *image, int spacing,
                                            int maxr, float statlow, float stathigh,
                                            Image *hist, Image *slopes, Image *stats );
Seed_Vector*  find_seeds2       ( Contour *trace, Image *image );
#endif //  _H_SEED
