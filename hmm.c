#include <assert.h>
#include "error.h"
#include "common.h"
#include "utilities.h"

#define NFEATURES 8
#define NBINS     32

typedef struct {
  int row;
  int class;
  int fid;
  int wid;
  double features[NFEATURES]; //length,score,angle,curvature,rootx,rooty,tipx,tipy
} Feature;

typedef struct
{ double[NBINS] bins;
  double        bin_min;
  double        bin_delta;
} Distribution;
typedef Distribution[NFEATURES]  DistributionBundle;
typedef struct {double min; double max;} minmax;

inline void minmax_update( minmax* this, double arg )
{ double mx = this->mx;
  double mn = this->mn;
  this->max = MAX(mx,arg);
  this->min = MIN(mn,arg);
}

/**
 * COMPARISON FUNCTIONS FOR SORTING FEATURE TABLE
 */

int cmp_sort_class( const void* a, const void* b )
{ Feature* rowa = (Feature*)a,
           rowb = (Feature*)b;
  return rowb->class - rowa->class;
}

int cmp_sort_time( const void* a, const void* b )
{ Feature* rowa = (Feature*)a,
           rowb = (Feature*)b;
  return rowb->fid   - rowa->fid;
}

int cmp_sort_class_time( const void* a, const void* b )
{ Feature* rowa = (Feature*)a,
           rowb = (Feature*)b;
  int dclass = rowb->class - rowa->class;  
  return (dclass==0) ? (rowb->fid - rowa->fid) : dclass;
}

void Enumerate_Feature_Table( Feature *table, int nrows )
{ int i = nrows;
  while(i--)
    table[i].row = i;
}

/**
 * Returns an array of normalized, log2 histograms for each class in the
 * feature table.
 */
DistributionBundle *Build_Distrubutions( Feature *table, int nrows )
{ 
  minmax[NFEATURES] bounds;
  int i,j;
  int minclass, maxclass, nclass;
  
  // determine bounds - same for all classes
  for(i=0; i<nrows; i++)
  { double* row = table[i].features;
    for(j=0; j<NFEATURES; j++)
      minmax_update( bounds + j, row[j] );
  }

  // Sort feature table to group classes
  qsort( table, nrows, sizeof(Feature), cmp_sort_class );
  minclass = ((Feature*)table)[0].class;
  maxclass = ((Feature*)table)[nrows-1].class; 
  nclass   = maxclass - minclass + 1;
  assert( nclass > 0 );

  // build distributions for each class
  { DistributionBundle *bundle = (DistributionBundle*) Guarded_Malloc( nclass * sizeof(DistributionBundle), "Build Distrubutions" );
    int *class_index  = (int*) Guarded_Malloc( (nclass+1) * sizeof(int), "Build Distrubutions"); 
    int iclass;

    // count number of rows in each class
    memset( class_index, 0, sizeof(int)*nclass );
    for( i=0; i<nrows; i++ )
      class_index[ table[i].class - minclass ]++;
    
    // get index of first row for each class
    { int last = class_index[0];
      class_index[0] = 0;
      for( i=0; i<=nrows; i++ ) 
      { int t = class_index[i];
        class_index[i] = class_index[i-1] + last;
        last = t;
      }
    }

    for( iclass = 0; iclass < nclass; iclass++ )
    { Distribution *hists = (Distribution*)(bundle+iclass); // Select set of histograms for this class
      
      // Initialize: Fill in bin_min and bin_delta
      for(j=0; j<NFEATURES; j++)
      { double mx = bounds[j].max,
               mn = bounds[j].min;
        hists[j].bin_min = mn;
        hists[j].bin_delta = NBINS/(mx*(1.0+1e-5)-mn);
        if( mx-mn < (1e-3 * mx) )
          warning("Warning: Histogram has very small bin width.\n");
      }
      
      // Compute histograms
      for(i=class_index[iclass]; i<class_index[iclass+1]; i++)
      { double *row = table[i].features;
        double norm = 0.0;

        // Add contributions to bins
        for(j=0; j<NFEATURES; j++)
        { Distribution *h = hists + j;
          double mn = h->min,
                 d  = h->delta;
          int idx = floor( (row[i] - mn)/d ); // mn-->0, mx-->NBINS-1
          h[idx]++;
        }

        // Adjust bins with count of zero
        for(j=0; j<NFEATURES; j++)
        { Distribution *h = hists + j;
          int k;
          for(k=0; k<NBINS; k++)
            if( h[k] < 0.9 )
              h[k] += 1.0;
        }

        // Compute norm
        for(j=0; j<NFEATURES; j++)
        { Distribution *h = hists + j;
          int k;
          for(k=0; k<NBINS; k++)
            norm += h[k];
        }
        
        // Apply log2 and norm
        norm = log2(norm);
        for(j=0; j<NFEATURES; j++)
        { Distribution *h = hists + j;
          int k;
          for(k=0; k<NBINS; k++)
            h[k] = log2(h[k]) - norm;
        }

      } //end loop over rows in class
    } //end loop over classes

    free( class_index );
    return bundle;
  } // end context for building distributions
}
