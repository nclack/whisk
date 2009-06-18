#include <assert.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "error.h"
#include "utilities.h"

#define NBINS 32

typedef struct 
{ int row;
  int fid;
  int wid;
  int state;
  int valid_velocity;
  int n;
  double *data;  // array of n elements
  double *velocity; // array of n elements - change in data/time
} Measurements;

typedef struct
{ int n_measures;
  int n_states;
  int n_bins;
  double *bin_min;    // array of n_measures elements
  double *bin_delta;  // array of n_measures elements
  double *data;       // array of holding histogram information with dimensions (n_bins,n_measures,n_states)
} Distributions;

typedef struct
{ int fid;
  int wid;
} SegKey;

Measurements *Alloc_Measurements_Table( int n_rows, int n_measurements )
{ Measurements *table = Guarded_Malloc( sizeof(Measurements)*n_rows, "allocate measurements table" );
  double *dataspace   = Guarded_Malloc( 2*sizeof(double)*n_measurements*n_rows, "allocate measurements table" ); 
  double *velocityspace  = dataspace + n_measurements*n_rows;
  while( n_rows-- )
  { table->data  = dataspace + n_rows;
    table->velocity = velocityspace + n_rows;
  }
  return table;
}

void Free_Measurements_Table( Measurements *table )
{ if( !table ) return;
  if( table[0].data ) free( table[0].data ); // frees the data/velocitys for all rows
  free(table);
}

Distributions *Alloc_Distributions( int n_bins, int n_measures, int n_states )
{ Distributions *this = Guarded_Malloc( sizeof(Distributions), "allocate distributions" );
  int nvol = n_bins*n_measures*n_states;
  double *data = Guarded_Malloc( sizeof(double)*nvol, "allocate distributions - data block" );
  double *bindata = Guarded_Malloc( sizeof(double)*n_measures*2, "allocate distributions - bin block" );
  this->data = data;
  this->bin_min = bindata;
  this->bin_delta = bindata + n_measures;
  return this;
}

void Free_Distributions( Distributions *this )
{ if( !this ) return;
  if( this->bin_min ) free( this->bin_min ); // also frees bin_delta
  if( this->data    ) free( this->data    );
  free(this);
}

// Use this to copy in from e.g. matlab, python
// Assumes first three columns are fid,wid,state (all ints) followed by data
// so Measurements.n = n_cols - 3 
//
Measurements *Measurements_Table_From_Doubles( double *raw, int n_rows, int n_cols )
{ int n = n_cols - 3;
  Measurements *table = Alloc_Measurements_Table( n_rows, n );
  while( n_rows-- )
  { double *rawrow = raw + n_cols*n_rows;
    Measurements *row = table + n_rows;
    row->fid   = (int) rawrow[0];
    row->wid   = (int) rawrow[1];
    row->state = (int) rawrow[2];
    row->valid_velocity = 0;
    row->n     = n;
    memcpy( row->data, rawrow+3, sizeof(double)*n );
  }
  return table;
}

int cmp_sort_state_time( const void* a, const void* b )
{ Measurements *rowa = (Measurements*)a,
               *rowb = (Measurements*)b;
  int dstate = (rowb->state) - (rowa->state);  
  return (dstate==0) ? (rowb->fid - rowa->fid) : dstate;
}

int cmp_sort_time( const void* a, const void* b )
{ Measurements *rowa = (Measurements*)a,
               *rowb = (Measurements*)b;
  int dstate = (rowb->state) - (rowa->state);  
  return (dstate==0) ? (rowb->fid - rowa->fid) : dstate;
}

void Enumerate_Measurements_Table( Measurements *table, int nrows )
{ int i = nrows;
  while(i--)
    table[i].row = i;
}

void Sort_Measurements_Table_State_Time( Measurements *table, int nrows )
{ qsort( table, nrows, sizeof(Measurements), cmp_sort_state_time );
}

void Sort_Measurements_Table_Time( Measurements *table, int nrows )
{ qsort( table, nrows, sizeof(Measurements), cmp_sort_time );
}

inline double _diff(double a, double b) 
{ return fabs(a - b);
}

// Assumes `sorted_table` is sorted by Sort_Measurements_Table
void Compute_Velocities( Measurements *sorted_table, int n_rows )
{ int i;
  int n = sorted_table[0].n;
  while( n_rows-- > 1 )
  { Measurements *a = sorted_table + n_rows - 1,
                 *b = sorted_table + n_rows;
    if( (b->fid - a->fid) == 1 )
    { double *d  = b->velocity,
             *bd = b->data,
             *ad = a->data;;
      for( i=0 ; i<n ; i++)  // for each field
        d[i] = _diff( bd[i] , ad[i] ); // compute difference
    } else 
    { b->valid_velocity = 0;
    }
  }
  // Handle the first row:
  if( sorted_table[1].valid_velocity )
  { memcpy( sorted_table[0].velocity, sorted_table[1].velocity, sizeof(double)*n );
    sorted_table[0].valid_velocity = 1;
  } else 
  { sorted_table[0].valid_velocity = 0;
  }
}

// Can pass NULL for minstate, maxstate if you doesn't need these.
// Returns the number of states
//
// If sorted, this is constant time
// If not sorted, this is O(n_rows)
int _count_n_states( Measurements *table, int n_rows, int sorted, int *minstate, int *maxstate )
{ int mn,mx;
  if( sorted )
  { mn = table[0].state;
    mx = table[n_rows-1].state;
  } else
  { n_rows--;
    mn = table[n_rows].state;
    mx = table[n_rows].state;
    while(n_rows--)
    { int s = table[n_rows].state;
      mn = MIN( mn, s );
      mx = MAX( mx, s );
    }
  }
  if( minstate )
    *minstate = mn;
  if( maxstate )
    *maxstate = mx;
  return mx - mn + 1;
}

Distributions *Alloc_Distributions_For_Sorted_Table( Measurements *sorted_table, int n_rows, int n_bins, int *minstate, int* maxstate )
{ return Alloc_Distributions( n_bins,                                                   // n_bins
                              sorted_table[0].n,                                        // n_measures
                              _count_n_states( sorted_table, n_rows, 1, minstate, maxstate ) ); // n_states
}

// The range of these histograms covers the state space sampled by the movie
Distributions *Build_Distributions( Measurements *sorted_table, int n_rows )
{ int minstate,maxstate;
  Distributions *d = Alloc_Distributions_For_Sorted_Table( sorted_table, n_rows, NBINS, &minstate, &maxstate );
  int i,j,k;
  int n = sorted_table[0].n;
  int *mn, *mx;
  double *delta;
  int measure_stride = d->n_bins,
      state_stride   = d->n_bins * d->n_measures,
      dvol           = d->n_bins * d->n_measures * d->n_states; 

  mn = (int*) Guarded_Malloc( 2 * sizeof(int) * n, "Build distributions - alloc mn and mx" );
  mx = mn + n;

  // Get extents for bins
  for( j=0; j < n; j++ )
  { mx[j] = sorted_table[0].data[j];
    mn[j] = sorted_table[0].data[j];
  }
  for( i=1; i < n_rows; i++ )
  { double *row = sorted_table[i].data;
    for( j=0; j < n; j++ )
    { double v = row[j];
      mn[j] = MIN( mn[j], v );
      mx[j] = MAX( mn[j], v );
    }
  }
  for( j=0; j < n; j++ )
  { d->bin_min[j] = mn[j];
    d->bin_delta[j] = (NBINS) / (mx[j]*(1.001) - mn[j]);
  }
  delta = d->bin_delta;

  // Accumulate
  memset( d->data, 0, sizeof(double) * dvol );
  for( i=0; i < n_rows; i++ )
  { Measurements *mrow = sorted_table + i;
    double *data = mrow->data;
    int istate = mrow->state - minstate;
    double *hist = d->data + istate * state_stride;
    for( j=0; j<n; j++ )
      hist[ j*measure_stride + (int) floor(  (data[j] - mn[j]) / delta[j]  ) ] ++;
  }

  // Normalize
  for( i=0; i < d->n_states; i++ )
  { double *hists = d->data + i * state_stride;
    for( j=0; j < d->n_measures; j++ )
    { double *h = hists + j * measure_stride;
      double norm = 0;
      for( k=0; k < d->n_bins; k++ )  // Add one to each bin (make sure nothing is impossible)
        h[k]++;
      for( k=0; k < d->n_bins; k++ )  // Accumulate 
        norm += h[k];
      norm = log2(norm);
      for( k=0; k < d->n_bins; k++ )  // Apply norm, take log2
        h[k] = log2(h[k]) - norm;
    }
  }

  return d;
}

// Builds histograms using the precompted valid velocties.
// This changes the sort order of the table.  The input should be sorted in
//   state,time order.
// These histograms cover the required state space
Distributions *Build_Velocity_Distributions( Measurements *sorted_table, int n_rows )
{ int minstate,maxstate;
  Distributions *d = Alloc_Distributions_For_Sorted_Table( sorted_table, n_rows, NBINS, &minstate, &maxstate );
  int i,j,k;
  int n = sorted_table[0].n;
  int *mn, *mx;
  double *delta;
  int measure_stride = d->n_bins,
      state_stride   = d->n_bins * d->n_measures,
      dvol           = d->n_bins * d->n_measures * d->n_states; 

  mn = (int*) Guarded_Malloc( 2 * sizeof(int) * n, "Build distributions - alloc mn and mx" );
  mx = mn + n;

  // Get extents for bins
  Sort_Measurements_Table_Time( sorted_table, n_rows );
  { Measurements *last = sorted_table,
                  *this = sorted_table,
                  *next = NULL;
    int nlast, nthis;
    int fid = last->fid;

    while( (++this)->fid == fid && this - sorted_table < n_rows );
    nlast = this-last;
    next = this;
    fid = this->fid;
    while( (++next)->fid == fid && next - sorted_table < n_rows );
    nthis = next-this;

    for( j=0; j < n; j++ )
    { double v = _diff( last->data[j], this->data[j] ); 
      mx[j] = v;
      mn[j] = v;
    }

    while( next - sorted_table < n_rows )
    { 

      for(i=0; i<nlast; i++)
      { double *ldata = last[i].data;
        for(j=0; j<nthis; j++)  
        { double *tdata = this[j].data;
          for(k=0; k<n; k++)
          { double diff = _diff( ldata[k], tdata[k] ); 
            mx[k] = MAX(mx[k],diff);
            mn[k] = MIN(mn[k],diff);
          }
        }
      }
      last = this;
      nlast = nthis;
      this = next;

      fid = this->fid;
      while( (++next)->fid == fid && next - sorted_table < n_rows);
      nthis = next-this;
    }
  }
  for( j=0; j < n; j++ )
  { d->bin_min[j] = mn[j];
    d->bin_delta[j] = (NBINS) / (mx[j]*(1.001) - mn[j]); 
  }
  delta = d->bin_delta;

  // Accumulate
  memset( d->data, 0, sizeof(double) * dvol );
  for( i=0; i < n_rows; i++ )
  { Measurements *mrow = sorted_table + i;
    if( mrow->valid_velocity )
    { double *data = mrow->velocity;
      int istate   = mrow->state - minstate;
      double *hist = d->data + istate * state_stride;
      for( j=0; j<n; j++ )
        hist[ j*measure_stride + (int) floor(  (data[j] - mn[j]) / delta[j]  ) ] ++;
    }
  }

  // Normalize
  for( i=0; i < d->n_states; i++ )
  { double *hists = d->data + i * state_stride;
    for( j=0; j < d->n_measures; j++ )
    { double *h = hists + j * measure_stride;
      double norm = 0;
      for( k=0; k < d->n_bins; k++ )  // Add one to each bin (make sure nothing is impossible)
        h[k]++;
      for( k=0; k < d->n_bins; k++ )  // Accumulate 
        norm += h[k];
      norm = log2(norm);
      for( k=0; k < d->n_bins; k++ )  // Apply norm - take log
        h[k] = log2( h[k] ) - norm;
    }
  }

  return d;
}

// vec must be an array of length dist->n_measures
// Assumes distributions encodes densities as log2 probability
double Eval_Likelihood_Log2( Distributions *dist, double *vec, int istate )
{ int measure_stride = dist->n_bins,
      state_stride   = dist->n_bins * dist->n_measures;
  double *hists = dist->data + istate * state_stride;
  double acc = 0;
  int ibin;
  int i;

  for(i=0; i < dist->n_measures; i++)
  { ibin = floor( ( vec[i] - dist->bin_min[i] ) / ( dist->bin_delta[i] ) );
    acc += hists[measure_stride*i + ibin]; // addition here bc probs are in log space
  }
  return acc;
}

// prev and next must be arrays of length dist->n_measures
// Assumes distributions encodes densities as log2 probability
// Distributions should be functions of the differences between next and prev
double Eval_Transition_Likelihood_Log2( Distributions *dist, double *prev, double *next, int istate )
{ static double *vec = NULL;
  static maxn = 0;
  int i = dist->n_measures;
  vec = (double*) request_storage( vec, &maxn, sizeof(double), i, "eval transitions");
  while(i--)
    vec[i] = _diff( next[i], prev[i] );
  
  return Eval_Likelihood_Log2(dist,vec,istate);;
}

// sorted_table must be sorted in ascending time order (i.e. ascending fid)
// Start and end should have the same `state` property.
// Algorithm here is to just take the transition with maximum liklihood at 
// each step.  This will be the most probable markov path joining the start
// and end states. 
//
// Returns a vector of pointers into the table that trace out the best path
// This vector will need to be freed later.
Measurements **Find_Path( Measurements *sorted_table, 
                         Distributions *shape, 
                         Distributions *velocity,
                         Measurements *start,
                         Measurements *end )
{ int path_length = end->fid - start->fid; // path excludes start and end
  Measurements **path;
  double valmax = -DBL_MAX;
  int istep = 0;
  int fid;
  int istate = start->state - minstate; //FIXME need minstate?
  double *ldata = start->data;
  Measurements *last = sorted_table,
               *this = sorted_table,
               *next = NULL;

  assert(path_length > 0);
  path = (Measurements**) Guarded_Malloc( sizeof(Measurements*)*path_length, "find path - allocate path");

  fid = start->fid;
  while( (++last)->fid != fid && last - sorted_table < n_rows ); // advance `last` till start fid is found
  this = last;
  while( (++this)->fid == fid && this - sorted_table < n_rows ); // advance `this` to next frame
  nlast = this - last;
  next = this;
  fid = this->fid;
  while( (++next)->fid == fid && next - sorted_table < n_rows ); // advance `next` to next-next frame
  nthis = next - this;

  while( next - sorted_table < n_rows )
  { 
    for(j=0; j<nthis; j++)  
    { double *tdata = this[j].data;
      double p = Eval_Transition_Likelihood_Log2( velocity, ldata, tdata, istate );
      if( p > valmax )
      { valmax = p;
        path[istep] = this;
      }
    }
    ldata = path[istep++]; // replace argmax 

    this = next; // advance `this` to next frame

    fid = this->fid;
    while( (++next)->fid == fid && next - sorted_table < n_rows); //advance `next` to next-next frame
    nthis = next-this;
  }

  return path;
}

void Solve( Measurements *table, int n_rows )
{ Distributions *shape, *velocity;
  int minstate, maxstate, nstates;
  int i;
  
  Sort_Measurements_Table_State_Time( table, n_rows );
  nstates = _count_n_states( table, n_rows, &minstate, &maxstate );
  Compute_Velocities( table, n_rows );
  shape    = Build_Distributions( table, n_rows );
  velocity = Build_Velocity_Distributions( table, n_rows ); // This changes the sort order, now table is in time order

  { SegKey **tstacks = Guarded_Malloc( sizeof(SeqKey*) * nstate, "trajectory stacks" );
    int  *maxidx  = Guarded_Malloc( sizeof(int) * nstate, "trajectory stacks" ); 
    memset(maxidx,0, sizeof(int)*nstate);

    // TODO ...
    // probably want to build trajectories as stacks - really just bundles
    //   of (fid,wid) pairs...order won't matter.  
    // ? what's the best way to get these out to e.g. python...raw structs should
    //   fine.  To file is another problem.
    // Iterate through time till gap found
    // Find end of gap
    // For all states at edge of gap:
    //   get start, end
    //   Use Fill_Path
    // Push result on appt trajectory
  }

  Free_Distributions( shape );
  Free_Distributions( velocity );
}
