#include "error.h"
#include "adjust_scan_bias.h"
#include "image_lib.h"
#include <math.h>

#ifndef MIN
#define MIN(a,b) ( a<b ? a : b )
#endif

/*********************************************************************************
 * Image based interface - operates on an image series
 */

typedef struct _Statistic
{ double sum;
  double sumsq;
  size_t count;
} Statistic;

SHARED_EXPORT
double incremental_estimate_scan_bias_h(Image *image, double thresh, double *stat)
// pass NULL for Image* to reset the accumulator
// otherwise, assumes Image* is a uint8 image
{ static Statistic accumulator = { 0.0, 0.0, 0 };
  int w,h,a;
  double mean = 0.0,
          meansq = 0.0;

  if( image == NULL )
  { accumulator.sum   = 0.0;
    accumulator.sumsq = 0.0;
    accumulator.count = 0;
    return 0.0;
  }

  w = image->width;
  h = image->height;
  a = w*h; 

  { int i, j, k, count = 0;
    uint8 *s = image->array;
    //compute mean gain difference
    for( j=0; j<h-1; j+=2 )         //horizontal scan lines
    { s = image->array + j*w;       //pointer to line
      for( k=0; k<w; k++ )          //iterate over line
      { if( (s[k] > thresh) && //exclude dark pixels
          (s[k] < 250 ) &&          //exclude nearly (within 2%) saturated pixels
          s[k+w])                   // avoid divide by zero

        { double bias = s[k] / (double) s[k+w];
          mean += bias;
          meansq += bias * bias;
          count++;
        }
      }
    }
    accumulator.sum   += mean;
    accumulator.sumsq += meansq;
    accumulator.count += count;
    mean   = accumulator.sum   / accumulator.count;
    meansq = accumulator.sumsq / accumulator.count;
  }
  
  { double std = sqrt(meansq - mean*mean);
    *stat = fabs( (mean-1.0f) / std );
    //debug( "H Bias: %5.4g (std: %5.4g, stat: %5.4g)\n", mean, std, *stat );
  }
  return mean;
}

SHARED_EXPORT
void image_adjust_scan_bias_h( Image *image, double gain )
{ int    w, h, a;
  w = image->width;
  h = image->height;
  a = w*h; 

  { int i,j,k; //bias odd scan lines
    uint8 *s;
    { for( j=1; j<h; j+=2 )           //horizontal scan lines
      { s = image->array + j*w;       //pointer to first line
        for( k=0; k<w; k++ )          //iterate over line
          s[k] = (uint8) MIN(s[k]*gain,255);
      }
    }
  }
}


SHARED_EXPORT
double incremental_estimate_scan_bias_v(Image *image, double thresh, double *stat)
// pass NULL for Image* to reset the accumulator
// otherwise, assumes Image* is a uint8 image
{ static Statistic accumulator = { 0.0, 0.0, 0 };
  int w,h,a;

  if( image == NULL )
  { accumulator.sum   = 0.0;
    accumulator.sumsq = 0.0;
    accumulator.count = 0;
    return 0.0;
  }

  w = image->width;
  h = image->height;
  a = w*h; 

  { int j, k, count = 0;
    double mean = 0.0,
           meansq = 0.0;
    uint8 *s = image->array;
    //compute mean gain difference
    { for( j=0; j<w-1; j+=2 )           //vertical scan lines
      { s = image->array + j;           //pointer to first line
        for( k=0; k<h; k++ )            //iterate over line
        { int idx = k*w;
          if( (s[idx] > thresh ) &&        //exclude dark pixels
              (s[idx] < 250 ) &&           //exclude nearly (within 2%) saturated pixels
               s[idx+1])                   //avoid divide by zero
          { double bias = s[idx] / (double) s[idx+1];
            mean += bias;
            meansq += bias * bias;
            count++;
          }
        }
      }
    }

    accumulator.sum   += mean;
    accumulator.sumsq += meansq;
    accumulator.count += count;
    mean   = accumulator.sum   / (double) accumulator.count;
    meansq = accumulator.sumsq / (double) accumulator.count;
    
    { double std = sqrt(meansq - mean*mean);
      *stat = fabs( (mean-1.0f) / std );
      //debug( "V Bias: %5.4g (std: %5.4g, stat: %5.4g)\n", mean, std, *stat );
      return mean;
    }
  }
}
  
SHARED_EXPORT
void image_adjust_scan_bias_v( Image *image, double gain )
{ int    w, h, a;
  w = image->width;
  h = image->height;
  a = w*h; 

  { int i,j,k; //bias odd scan lines
    uint8 *s;
    { for( j=1; j<w; j+=2 )               //vertical scan lines
      { s = image->array + j;             //pointer to first line
        for( k=0; k<h; k++ )              //iterate over line
        { int idx = k*w;
          s[idx] = (uint8) MIN(s[idx]*gain,255); 
        }
      }
    }
  }
}


/*********************************************************************************
 * Stack based interface
 */

SHARED_EXPORT
double estimate_scan_bias_h(Stack *movie, double mean_intensity, double *stat)
{ int d = movie->depth;
  double mean;
  incremental_estimate_scan_bias_h(NULL, 0, NULL);
  while(d--)
    mean = incremental_estimate_scan_bias_h( Select_Plane(movie,d), mean_intensity, stat );
  debug( "H Bias: %5.4g (stat: %5.4g)\n", mean, *stat );
  return mean;
}

SHARED_EXPORT
void adjust_scan_bias_h( Stack *movie, double gain )
{ int d = movie->depth;
  debug("Adjust horizontal\n");
  while(d--)
  { Image *image = Select_Plane(movie,d);
    image_adjust_scan_bias_h(image,gain);
  }
}

SHARED_EXPORT
double estimate_scan_bias_v(Stack *movie, double mean_intensity, double *stat)
{ int d = movie->depth;
  double mean;
  incremental_estimate_scan_bias_v(NULL, 0, NULL);
  while(d--)
    mean = incremental_estimate_scan_bias_v( Select_Plane(movie,d), mean_intensity, stat );
  debug( "V Bias: %5.4g (stat: %5.4g)\n", mean, *stat );
  return mean;
}

SHARED_EXPORT
void adjust_scan_bias_v( Stack *movie, double gain )
{ int d = movie->depth;
  debug("Adjust vertical\n");
  while(d--)
  { Image *image = Select_Plane(movie,d);
    image_adjust_scan_bias_v(image,gain);
  }
}

SHARED_EXPORT
void adjust_scan_bias( Stack *movie )
{ double mean_intensity = 0.0;
  double vgain, vstat, hgain, hstat;
  int    w, h, d, a;

  w = movie->width;
  h = movie->height;
  d = movie->depth;
  a = w*h; 

  { uint8 *s = movie->array + w*h*d;
    while( s-- >= movie->array )
      mean_intensity += *s;
    mean_intensity /= (w*h*d);
  }

  hgain = estimate_scan_bias_h(movie, mean_intensity, &hstat );
  vgain = estimate_scan_bias_v(movie, mean_intensity, &vstat );
  if( hstat > vstat )
    adjust_scan_bias_h(movie, hgain);
  else
    adjust_scan_bias_v(movie, vgain);
}

#if 0
double estimate_scan_bias_h(Stack *movie, double mean_intensity, double *stat)
{ int    w, h, d, a;

  w = movie->width;
  h = movie->height;
  d = movie->depth;
  a = w*h; 

  { int i, j, k, count = 0;
    double mean = 0.0,
           meansq = 0.0;
    uint8 *s = movie->array;
    //compute mean gain difference
    for( i = 0; i<d; i++ )
    { for( j=0; j<h-1; j+=2 )         //horizontal scan lines
      { s = movie->array + i*a + j*w; //pointer to first line
        for( k=0; k<w; k++ )          //iterate over line
        { if( (s[k] > mean_intensity) && //exclude dark pixels
            (s[k] < 250 ) &&          //exclude nearly (within 2%) saturated pixels
            s[k+w])                   // avoid divide by zero

          { double bias = s[k] / (double) s[k+w];
            mean += bias;
            meansq += bias * bias;
            count++;
          }
        }
      }
    }
    mean /= count;
    meansq /= count;
    
    { double std = sqrt(meansq - mean*mean);
      *stat = fabs( (mean-1.0f) / std );
      debug( "H Bias: %5.4g (std: %5.4g, stat: %5.4g)\n", mean, std, *stat );
      return mean;
    }
  }
}

void adjust_scan_bias_h( Stack *movie, double gain )
{ int    w, h, d, a;
  debug("Adjust horizontal\n");
  w = movie->width;
  h = movie->height;
  d = movie->depth;
  a = w*h; 

  { int i,j,k; //bias odd scan lines
    uint8 *s;
    for( i = 0; i<d; i++ )
    { for( j=1; j<h; j+=2 )           //horizontal scan lines
      { s = movie->array + i*a + j*w; //pointer to first line
        for( k=0; k<w; k++ )          //iterate over line
          s[k] = (uint8) MIN(s[k]*gain,255);
      }
    }
  }
}

double estimate_scan_bias_v(Stack *movie, double mean_intensity, double *stat)
{ int    w, h, d, a;

  w = movie->width;
  h = movie->height;
  d = movie->depth;
  a = w*h; 

  { int i, j, k, count = 0;
    double mean = 0.0,
           meansq = 0.0;
    uint8 *s = movie->array;
    //compute mean gain difference
    for( i = 0; i<d; i++ )
    { for( j=0; j<w-1; j+=2 )           //vertical scan lines
      { s = movie->array + i*a + j;     //pointer to first line
        for( k=0; k<h; k++ )            //iterate over line
        { int idx = k*w;
          if( (s[idx] > mean_intensity) && //exclude dark pixels
              (s[idx] < 250 ) &&           //exclude nearly (within 2%) saturated pixels
               s[idx+1])                   //avoid divide by zero
          { double bias = s[idx] / (double) s[idx+1];
            mean += bias;
            meansq += bias * bias;
            count++;
          }
        }
      }
    }
    mean /= count;
    meansq /= count;

    { double std = sqrt(meansq - mean*mean);
      *stat = fabs( (mean-1.0) / std );
      debug( "V Bias: %5.4g (std: %5.4g, stat: %5.4g)\n", mean, std, *stat );
      return mean;
    }
  }
}
#endif


#if 0
void adjust_scan_bias_v( Stack *movie, double gain )
{ int    w, h, d, a;
  debug("Adjust vertical\n");
  w = movie->width;
  h = movie->height;
  d = movie->depth;
  a = w*h; 

  { int i,j,k; //bias odd scan lines
    uint8 *s;
    //bias odd scan lines
    for( i = 0; i<d; i++ )
    { for( j=1; j<w; j+=2 )               //vertical scan lines
      { s = movie->array + i*a + j;       //pointer to first line
        for( k=0; k<h; k++ )              //iterate over line
        { int idx = k*w;
          s[idx] = (uint8) MIN(s[idx]*gain,255); 
        }
      }
    }
  }
}
#endif

