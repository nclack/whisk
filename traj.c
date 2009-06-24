// Refectoring:

#include <assert.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "error.h"
#include "utilities.h"

// TESTS
#if 0  
#undef   TEST_MEASUREMENT_TABLE_IO_1
#undef   TEST_BUILD_DISTRIBUTIONS
#endif

#undef   DEBUG_MEASUREMENTS_TABLE_ALLOC
#undef   DEBUG_DISTRIBUTIONS_ALLOC
#undef   DEBUG_BUILD_VELOCITY_DISTRIBUTIONS 

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

Measurements *Alloc_Measurements_Table( int n_rows, int n_measurements )
{ Measurements *table = Guarded_Malloc( sizeof(Measurements)*n_rows, "allocate measurements table" );
  double *dataspace   = Guarded_Malloc( 2*sizeof(double)*n_measurements*n_rows, "allocate measurements table" ); 
  double *velocityspace  = dataspace + n_measurements*n_rows;
#ifdef DEBUG_MEASUREMENTS_TABLE_ALLOC
  printf("Measurements table alloc: data at %p\n",dataspace);
#endif
  while( n_rows-- )
  { Measurements *row = table     + n_rows;
    row->data     = dataspace     + n_rows*n_measurements;
    row->velocity = velocityspace + n_rows*n_measurements;
    row->row = n_rows;
    row->n = n_measurements;
  }
  return table;
}

void Free_Measurements_Table( Measurements *table )
{ if( !table ) return;
#ifdef DEBUG_MEASUREMENTS_TABLE_ALLOC
  printf("Measurements table free : data at %p\n",table[0].data - table[0].row * table[0].n);
#endif
  free( table[0].data - table[0].row * table[0].n); // frees the data/velocities for all rows
  free(table);
}

Distributions *Alloc_Distributions( int n_bins, int n_measures, int n_states )
{ Distributions *this = Guarded_Malloc( sizeof(Distributions), "allocate distributions" );
  int nvol = n_bins*n_measures*n_states;
  double *data = Guarded_Malloc( sizeof(double)*nvol, "allocate distributions - data block" );
  double *bindata = Guarded_Malloc( sizeof(double)*n_measures*2, "allocate distributions - bin block" );
#ifdef DEBUG_DISTRIBUTIONS_ALLOC
  printf("Alloc Distributions: nvol: %d\n",nvol);
  printf("\tdata: %p\n", data );
  printf("\tbindata: %p\n", bindata );
#endif
  this->n_measures = n_measures;
  this->n_states   = n_states;
  this->n_bins     = n_bins;
  this->data = data;
  this->bin_min = bindata;
  this->bin_delta = bindata + n_measures;
  return this;
}

void Free_Distributions( Distributions *this )
{ if( !this ) return;
#ifdef DEBUG_DISTRIBUTIONS_ALLOC
  printf("Free Distributions:\n");
  printf("\tdata: %p\n", this->data );
  printf("\tbindata: %p\n", this->bin_min );
#endif
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
    row->fid   = (int) rawrow[1];
    row->wid   = (int) rawrow[2];
    row->state = (int) rawrow[0];
    row->valid_velocity = 0;
    row->n     = n;
    memcpy( row->data, rawrow+3, sizeof(double)*n );
  }
  return table;
}

void Measurements_Table_Copy_Shape_Data( Measurements *table, int n_rows, double *buffer )
{ int n = table[0].n;
  memcpy( buffer, table[0].data - n*table[0].row, n*n_rows*sizeof(double) );
}

void Measurements_Table_Copy_Velocities( Measurements *table, int n_rows, double *buffer )
{ int n = table[0].n;
  memcpy( buffer, table[0].data - n*table[0].row + n*n_rows, n*n_rows*sizeof(double) );
}

// Returns the number of rows with the queried state
// Scans once over the table.
int Measurements_Table_Size_Select_State( Measurements *table, int n_rows, int state )
{ int count = 0;
  while(n_rows--)
    if( table[n_rows].state == state )
      count++;
  return count;
}

// Selects rows according to their state and returns the `time` and `valid_velocity` arrays.
// time and mask should be the appropriate size. See `Measurements_Table_Size_Select_State`
void Measurements_Table_Select_Time_And_Mask_By_State( Measurements *table, int n_rows, int state, double *time, int *mask )
{ int i=0;
  int j=0;
  int n = table[0].n;
  for( i=0; i<n_rows; i++ )
  { Measurements *row = table + i;
    if( row->state == state )
    { time[j  ] = row->fid;
      mask[j++] = row->valid_velocity;
    }
  } 
}

// Selects rows according to their state and returns velocities
// `data` should be the appropriate size. See `Measurements_Table_Size_Select_Velocities`
void Measurements_Table_Select_Velocities_By_State( Measurements *table, int n_rows, int state, double *data )
{ int i=0;
  int j=0;
  int n = table[0].n;
  for( i=0; i<n_rows; i++ )
  { Measurements *row = table + i;
    if( row->state == state )
      memcpy( data + (j++)*n, row->velocity, n*sizeof(double) );
  } 
}

// Selects rows according to their state and returns shape data
// `data` should be the appropriate size. See `Measurements_Table_Size_Select_Velocities`
void Measurements_Table_Select_Shape_By_State( Measurements *table, int n_rows, int state, double *data )
{ int i=0;
  int j=0;
  int n = table[0].n;
  for( i=0; i<n_rows; i++ )
  { Measurements *row = table + i;
    if( row->state == state )
      memcpy( data + (j++)*n, row->data, n*sizeof(double) );
  } 
}

FILE *Measurements_Table_To_File( FILE *fp, Measurements *table, int n_rows )
{ int n_measures = table[0].n;
  fwrite( &n_rows, sizeof(int), 1, fp );
  fwrite( &n_measures, sizeof(int), 1, fp );
  fwrite( table, sizeof(Measurements), n_rows, fp);
  fwrite( table[0].data - table[0].row*table[0].n, sizeof(double), 2*n_rows*n_measures, fp );
  return fp;
}

void Measurements_Table_To_Filename( const char *filename, Measurements *table, int n_rows )
{ FILE *fp = fopen( filename, "wb" );
  fp = Measurements_Table_To_File( fp, table, n_rows );
  fclose(fp);
}

Measurements *Measurements_Table_From_File( FILE *fp, int *n_rows)
{ Measurements *table;
  int n_measures,i;
  double *ref, *old;

  fread( n_rows, sizeof(int), 1, fp);
  fread( &n_measures, sizeof(int), 1, fp );

  table = Alloc_Measurements_Table( *n_rows, n_measures );
  ref = table[0].data; // head of newly allocated data block
  fread( table, sizeof(Measurements), *n_rows, fp); 
  old = table[0].data; // head of old (nonexistent) data block
  fread( ref, sizeof(double), 2*(*n_rows)*n_measures, fp );
  
  // update pointers to new data block
  i = *n_rows;
  while(i--)
  { Measurements *row = table + i;
    row->data     = ref + ( row->data - old );
    row->velocity = ref + ( row->velocity - old );
  }

  return table;
}

Measurements *Measurements_Table_From_Filename( const char *filename, int *n_rows)
{ Measurements *t = NULL;
  FILE *fp = fopen( filename, "rb" );
  t = Measurements_Table_From_File( fp, n_rows );
  fclose(fp);
  return t;
}

int test_Measurements_Table_FileIO( char* filename,  Measurements *table, int n_rows )
{ FILE *fp = fopen(filename,"wb");
  Measurements *t2;
  int nr2,i;

  Measurements_Table_To_File(fp, table, n_rows);
  fclose(fp);

  fp = fopen( filename, "rb" );
  t2 = Measurements_Table_From_File( fp, &nr2 );
  fclose(fp);

  if( nr2 != n_rows )
  { warning("Number of rows don't match: %d != %d\n",n_rows,nr2);
    goto Err;
  }
  
  while( n_rows-- )
  { Measurements *a = table + n_rows,
                *b = t2    + n_rows;
    if( (a->fid != b->fid) ||
        (a->wid != b->wid) ||
        (a->state != b->state) ||
        (a->valid_velocity != b->valid_velocity) ||
        (a->n != b->n) )
    { warning("Mismatch in row info\n");
      goto Err;
    }
    for( i=0; i<(a->n); i++ )
      if( a->data[i] != b->data[i] )
      { warning("Mismatch in shape data on row %d at column %d\n",n_rows,i);
        goto Err;
      }
    if( a->valid_velocity )
      for( i=0; i<(a->n); i++ )
        if( a->velocity[i] != b->velocity[i] )
        { warning("Mismatch in shape velocity on row %d at column %d\n",n_rows,i);
          goto Err;
        }
  }

  Free_Measurements_Table(t2);
  return 1;
Err:
  Free_Measurements_Table(t2);
  return 0;
}

int cmp_sort_state_time( const void* a, const void* b )
{ Measurements *rowa = (Measurements*)a,
               *rowb = (Measurements*)b;
  int dstate = (rowa->state) - (rowb->state);  
  return (dstate==0) ? (rowa->fid - rowb->fid) : dstate;
}

int cmp_sort_time( const void* a, const void* b )
{ Measurements *rowa = (Measurements*)a,
               *rowb = (Measurements*)b;
  return (rowa->fid - rowb->fid);
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
{ return (a - b);
}

// Assumes `sorted_table` is sorted by Sort_Measurements_Table_State_Time
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
      b->valid_velocity = 1; 
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
Distributions *Build_Distributions( Measurements *sorted_table, int n_rows, int n_bins )
{ int minstate,maxstate;
  Distributions *d = Alloc_Distributions_For_Sorted_Table( sorted_table, n_rows, n_bins, &minstate, &maxstate );
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
    d->bin_delta[j] = (n_bins) / (mx[j]*(1.001) - mn[j]);
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
Distributions *Build_Velocity_Distributions( Measurements *sorted_table, int n_rows, int n_bins )
{ int minstate,maxstate;
  Distributions *d = Alloc_Distributions_For_Sorted_Table( sorted_table, n_rows, n_bins, &minstate, &maxstate );
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
    { double v = _diff( this->data[j], last->data[j] ); 
      mx[j] = v;
      mn[j] = v;
    }

    while( next - sorted_table < n_rows )
    {
#ifdef DEBUG_BUILD_VELOCITY_DISTRIBUTIONS 
      printf("        fid     n\n");
      printf("last: %5d %5d\n",last->fid,nlast);
      printf("this: %5d %5d\n",this->fid,nthis);
      printf("next: %5d    \n",next->fid);
      printf("\n");
#endif

      for(i=0; i<nlast; i++)
      { double *ldata = last[i].data;
        for(j=0; j<nthis; j++)  
        { double *tdata = this[j].data;
          for(k=0; k<n; k++)
          { double diff = _diff( tdata[k], ldata[k] ); 
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
  } // end context - get extents

  for( j=0; j < n; j++ )
  { d->bin_min[j] = mn[j];
    d->bin_delta[j] = (n_bins) / (mx[j]*(1.001) - mn[j]); 
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
  static int maxn = 0;
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
                          int n_rows,
                         Distributions *shape, 
                         Distributions *velocity,
                         Measurements *start,
                         Measurements *end,
                         int minstate)
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
  int nlast, nthis;

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
  { int j;
    for(j=0; j<nthis; j++)  
    { double *tdata = this[j].data;
      double p = Eval_Transition_Likelihood_Log2( velocity, ldata, tdata, istate );
      if( p > valmax )
      { valmax = p;
        path[istep] = this;
      }
    }
    ldata = path[istep++]->data; // replace argmax 

    this = next; // advance `this` to next frame

    fid = this->fid;
    while( (++next)->fid == fid && next - sorted_table < n_rows); //advance `next` to next-next frame
    nthis = next-this;
  }

  return path;
}

// This function takes a table of measurements where some subset of the frames
// in the movie have been labeled.  That is, a subset of the rows have a
// `state` field that is different than -1.
//
// From this labeled subset two sets of histograms are computed conditioned on 
// the labelled state (for states different than -1).
//
// For unlabelled frames (gray areas), Find_Path will be called to link the labeled
// observations on either side of the gray area.
void Solve( Measurements *table, int n_rows, int n_bins )
{ Distributions *shape, *velocity;
  int minstate, maxstate, nstates;
  int nframes;
  int i,j;
  
  Sort_Measurements_Table_State_Time( table, n_rows );
  nstates = _count_n_states( table, n_rows, 1, &minstate, &maxstate );
  Compute_Velocities( table, n_rows );
  shape    = Build_Distributions( table, n_rows, n_bins );
  velocity = Build_Velocity_Distributions( table, n_rows, n_bins ); // This changes the sort order, now table is in time order
  nframes = table[n_rows-1].fid + 1;

  // Find the gray areas:
  // A frame is in a `gray area` if all observations in that frame have
  // state==-1.
  // This iterates over the sorted table building a sequence of start/stop
  // frame_id pairs marking the (inclusive) boundaries of gray areas.
  { int *gray_areas = Guarded_Malloc(nframes * sizeof(int), "in solve - alloc gray_areas");
    int ngray = 0;
    memset(gray_areas,0,sizeof(int)*nframes);
    // Compute whether frames have any rows with state > -1
    for(i=0;i<n_rows;i++)
    { Measurements *row = table+i;
      if(row->state > -1 )
        gray_areas[row->fid] = 1;
    }

    // Compute the inclusive boundaries of the gray areas in place
    if(gray_areas[0]==0)
      ngray++; //equiv to: gray_areas[ngray++] = current_frame; because current_frame=0
    for(i=1;i<nframes;i++)
    { int d = gray_areas[i] - gray_areas[i-1]; 
      if( d == -1 )     // Opening a gray area
        gray_areas[ ngray++ ] = i;
      else if( d == 1 ) // Closing a gray area
        gray_areas[ ngray++ ] = i-1;
    }
    
    // Compute trajectories - 
    // Each is an array, nframes long, of pointers into the table
    { Measurements **trajs = Guarded_Malloc( nstates * nframes * sizeof(Measurements*), "in solve, alloc trajectories");
      Measurements *row = table;
      memset(trajs,0, nstates * nframes * sizeof(Measurements*));
      // initialize
      for( ; row < table + n_rows; row++ )
        trajs[ ( row->state - minstate )*nframes + row->fid ] = row;

      // fill in gray areas
      for( i=minstate; i <= maxstate; i++ )
      { Measurements** t = trajs + i*nframes;
        for( j=0; j<ngray; j+=2 )
        { Measurements *start = t[  gray_areas[j  ] - 1  ],   //start
                     *end   = t[  gray_areas[j+1] + 1  ] ;  //end
          Measurements **path = Find_Path( table, n_rows, shape, velocity, start, end, minstate );
          int npath = end->fid - start->fid;
          memcpy( t + gray_areas[j], path, sizeof(Measurements*)*npath); 
          free(path);
        }
      }

      // Commit trajectories to table/output trajectories
      for( i=minstate; i <= maxstate; i++ )
      { Measurements** t = trajs + i*nframes;
        for(j=0; j<nframes; j++ )
          t[j]->state = i;
      }

      free(trajs);
    } // end context - trajs

    free( gray_areas );
  }

  Free_Distributions( shape );
  Free_Distributions( velocity );
}

#ifdef TEST_MEASUREMENT_TABLE_IO_1
int main(int argc, char *argv[])
{ int nrows = 40000;
  int ret;
  printf("Test: Write an empty table to file and read it back in.\n");
  Measurements *table = Alloc_Measurements_Table(nrows,6);
  ret = test_Measurements_Table_FileIO( "tmp.measurements", table, nrows );
  Free_Measurements_Table(table);
  if(ret)
    printf("\tTest passed.\n");
  else
    printf("\tTest FAILED.\n");
  return ret;
}
#endif // TEST_MEASUREMENT_TABLE_IO_1

#ifdef TEST_BUILD_DISTRIBUTIONS
char *Spec[] = {"<filename:string>", NULL};
int main(int argc, char *argv[])
{ Measurements *table;
  int n_rows = 0;
  int n_bins = 32;
  int ret = 1;
  
  printf("Test: Load measurements and compute distributions.\n");
  Process_Arguments(argc,argv,Spec,0);
  table = Measurements_Table_From_Filename( Get_String_Arg("filename"), &n_rows);

  { Distributions *shape, *velocity;
    int minstate, maxstate, nstates;
    int nframes;
    int i,j;

    Sort_Measurements_Table_State_Time( table, n_rows );
    nstates = _count_n_states( table, n_rows, 1, &minstate, &maxstate );
    Compute_Velocities( table, n_rows );
    shape    = Build_Distributions( table, n_rows, n_bins );
    velocity = Build_Velocity_Distributions( table, n_rows, n_bins ); // This changes the sort order, now table is in time order
    nframes = table[n_rows-1].fid + 1;
  }

  if(ret)
    printf("\tTest passed.\n");
  else
    printf("\tTest FAILED.\n");
  return ret;
}
#endif // TEST_BUILD_DISTRIBUTIONS
