// TODO: change the name of this file to something else, like identity.c
// TODO: make the .h

#include "compat.h"
#include "traj.h"
#include <assert.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "common.h"
#include "error.h"
#include "utilities.h"

#include "viterbi.h"

// TESTS
#if 0  
#undef   TEST_MEASUREMENT_TABLE_IO_1
#undef   TEST_BUILD_DISTRIBUTIONS
#undef   TEST_SOLVE_GRAY_AREAS
#endif

// DEBUG OUTPUT
#if 0
#define  DEBUG_SOLVE_GRAY_AREAS
#define  DEBUG_FIND_PATH
#define  DEBUG_DISTRIBUTIONS_ALLOC
#define  DEBUG_BUILD_VELOCITY_DISTRIBUTIONS 
#define  DEBUG_BUILD_DISTRIBUTIONS 
#define  DEBUG_MEASUREMENTS_TABLE_ALLOC
#define  DEBUG_MEASUREMENTS_TABLE_FROM_FILE
#endif

SHARED_EXPORT
Measurements *Alloc_Measurements_Table( int n_rows, int n_measurements )
{ Measurements *table = Guarded_Malloc( sizeof(Measurements)*n_rows, "allocate measurements table" );
  double *dataspace   = Guarded_Malloc( 2*sizeof(double)*n_measurements*n_rows, "allocate measurements table" ); 
  double *velocityspace  = dataspace + n_measurements*n_rows;
#ifdef DEBUG_MEASUREMENTS_TABLE_ALLOC
  printf("\nMeasurements table alloc: %d rows of data at %p\n",n_rows,dataspace);
#endif
  while( n_rows-- )
  { Measurements *row = table     + n_rows;
    row->data     = dataspace     + n_rows*n_measurements;
    row->velocity = velocityspace + n_rows*n_measurements;
    row->row = n_rows;
    row->n = n_measurements;
    row->valid_velocity = 0;
    row->face_x = 0;
    row->face_y = 0;
    row->col_follicle_x = 0;
    row->col_follicle_y = 0;
  }
  return table;
}

SHARED_EXPORT
void Free_Measurements_Table( Measurements *table )
{ if( !table ) return;
#ifdef DEBUG_MEASUREMENTS_TABLE_ALLOC
  printf("Measurements table free : data at %p\n",table[0].data - table[0].row * table[0].n);
  printf("\tdata: %p\n",table[0].data);
  printf("\t row: %d\n",table[0].row );
  printf("\t   n: %d\n",table[0].n   );
#endif
  free( table[0].data - table[0].row * table[0].n); // frees the data/velocities for all rows
  free(table);
}

SHARED_EXPORT
Distributions *Alloc_Distributions( int n_bins, int n_measures, int n_states )
{ Distributions *this = Guarded_Malloc( sizeof(Distributions), "allocate distributions" );
  int nvol = n_bins*n_measures*n_states;
  double *data = Guarded_Malloc( sizeof(double)*nvol, "allocate distributions - data block" );
  double *bindata = Guarded_Malloc( sizeof(double)*n_measures*2, "allocate distributions - bin block" );
#ifdef DEBUG_DISTRIBUTIONS_ALLOC
  printf("\nAlloc Distributions: nvol: %d\n",nvol);
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

SHARED_EXPORT
void Free_Distributions( Distributions *this )
{ if( !this ) return;
#ifdef DEBUG_DISTRIBUTIONS_ALLOC
  printf("\nFree Distributions:\n");
  printf("\tdata: %p\n", this->data );
  printf("\tbindata: %p\n", this->bin_min );
#endif
  if( this->bin_min ) free( this->bin_min ); // also frees bin_delta
  if( this->data    ) free( this->data    );
  free(this);
}

SHARED_EXPORT
void Copy_Distribution_To_Doubles( Distributions *this, double *destination )
{ memcpy( destination, this->data, (this->n_states * this->n_measures * this->n_bins) * sizeof(double) );
}

SHARED_EXPORT
void Distributions_Bins_To_Doubles( Distributions *this, double *destination )
{ int stride = this->n_measures;
  int i,j;
  for(i=0; i<this->n_measures; i++)
  { double mn  = this->bin_min[i],
           del = this->bin_delta[i];
    double *row = destination + stride*i;
    j = this->n_bins;
    while(j--)
      row[j] = mn + j * del;
  }
}

SHARED_EXPORT
void Measurements_Table_Set_Constant_Face_Position( Measurements *table, int n_rows, int x, int y )
{ while(n_rows--)
  { table[n_rows].face_x = x;
    table[n_rows].face_y = y;
  }
}

SHARED_EXPORT
void Measurements_Table_Set_Follicle_Position_Indices( Measurements *table, int n_rows, int ix, int iy )
{ while(n_rows--)
  { table[n_rows].col_follicle_x = ix;
    table[n_rows].col_follicle_y = iy;
  }
}

// Use this to copy in from e.g. matlab, python
// Assumes first three columns are label,fid,wid (all ints) followed by data
// so Measurements.n = n_cols - 3 
//
SHARED_EXPORT
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
    row->face_x = 0;
    row->face_y = 0;
    row->col_follicle_x = 0;
    row->col_follicle_y = 0;
    memcpy( row->data, rawrow+3, sizeof(double)*n );
  }
  return table;
}

// Use this to copy in to e.g. matlab, python
// Assumes first three columns are label,fid,wid followed by data
// Velocity data is not copied
SHARED_EXPORT
void Measurements_Table_Data_To_Doubles(Measurements *table, int n_rows, double *raw)
{ int n,n_cols;
  assert( n_rows > 0 );
  n = table[0].n;
  n_cols = n+3;
  while( n_rows-- )
  { double *rawrow = raw + n_cols*n_rows;
    Measurements *row = table + n_rows;
    rawrow[0] = row->state;
    rawrow[1] = row->fid;  
    rawrow[2] = row->wid;  
    memcpy( rawrow+3, row->data, sizeof(double)*n );
  }
}

// Use this to copy in to e.g. matlab, python
// Assumes first three columns are label,fid,wid followed by data
// Shape data is not copied
// Invalid velocities are set to 0
SHARED_EXPORT
void Measurements_Table_Velocity_To_Doubles(Measurements *table, int n_rows, double *raw)
{ int n,n_cols;
  assert( n_rows > 0 );
  n = table[0].n;
  n_cols = n+3;
  while( n_rows-- )
  { double *rawrow = raw + n_cols*n_rows;
    Measurements *row = table + n_rows;
    rawrow[0] = row->state;
    rawrow[1] = row->fid;  
    rawrow[2] = row->wid;  
    if( row->valid_velocity )
      memcpy( rawrow+3, row->velocity, sizeof(double)*n );
    else
      memset( rawrow, 0, sizeof(double)*n );
  }
}

SHARED_EXPORT
void Measurements_Table_Copy_Shape_Data( Measurements *table, int n_rows, double *buffer )
{ int n = table[0].n;
  memcpy( buffer, table[0].data - n*table[0].row, n*n_rows*sizeof(double) );
}

SHARED_EXPORT
void Measurements_Table_Copy_Velocities( Measurements *table, int n_rows, double *buffer )
{ int n = table[0].n;
  memcpy( buffer, table[0].data - n*table[0].row + n*n_rows, n*n_rows*sizeof(double) );
}

// Returns the number of rows with the queried state
// Scans once over the table.
SHARED_EXPORT
int Measurements_Table_Size_Select_State( Measurements *table, int n_rows, int state )
{ int count = 0;
  while(n_rows--)
    if( table[n_rows].state == state )
      count++;
  return count;
}

// Selects rows according to their state and returns the `time` and `valid_velocity` arrays.
// time and mask should be the appropriate size. See `Measurements_Table_Size_Select_State`
SHARED_EXPORT
void Measurements_Table_Select_Time_And_Mask_By_State( Measurements *table, int n_rows, int state, double *time, int *mask )
{ int i=0;
  int j=0;
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
SHARED_EXPORT
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
SHARED_EXPORT
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

SHARED_EXPORT
FILE *Measurements_Table_To_File( FILE *fp, Measurements *table, int n_rows )
{ int n_measures = table[0].n;
  fwrite( &n_rows, sizeof(int), 1, fp );
  fwrite( &n_measures, sizeof(int), 1, fp );
  fwrite( table, sizeof(Measurements), n_rows, fp);
  fwrite( table[0].data - table[0].row*table[0].n, sizeof(double), 2*n_rows*n_measures, fp );
  return fp;
}

SHARED_EXPORT
void Measurements_Table_To_Filename( const char *filename, Measurements *table, int n_rows )
{ FILE *fp = fopen( filename, "wb" );
  fp = Measurements_Table_To_File( fp, table, n_rows );
  fclose(fp);
}

SHARED_EXPORT
Measurements *Measurements_Table_From_File( FILE *fp, int *n_rows)
{ Measurements *table;
  int n_measures,i;
  double *ref, *old;

  fread( n_rows, sizeof(int), 1, fp);
  fread( &n_measures, sizeof(int), 1, fp );

  table = Alloc_Measurements_Table( *n_rows, n_measures );
  ref = table[0].data; // head of newly allocated data block
#ifdef DEBUG_MEASUREMENTS_TABLE_FROM_FILE
  printf("\tLoad -            ref: %p\n",ref);
#endif
  fread( table, sizeof(Measurements), *n_rows, fp); 
  old = table[0].data - table[0].row * n_measures; // head of old (nonexistent) data block
  fread( ref, sizeof(double), 2*(*n_rows)*n_measures, fp );
  
  // update pointers to new data block
  i = *n_rows;
  while(i--)
  { Measurements *row = table + i;
    row->data     = ref + ( row->data     - old );
    row->velocity = ref + ( row->velocity - old );
  }
#ifdef DEBUG_MEASUREMENTS_TABLE_FROM_FILE
  printf("\tLoad - table[0].data: %p\n",table[0].data);
  printf("\tLoad - table[0].row : %d\n",table[0].row );
#endif

  return table;
}

SHARED_EXPORT
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
    
inline double _cmp_sort_face_order__angle_wrt_face( Measurements *a )
{ int ix = a->col_follicle_x, 
      iy = a->col_follicle_y;
  return atan2( a->data[iy] - a->face_y,
                a->data[ix] - a->face_x );
}

int cmp_sort_face_order( const void* a, const void* b )
{ Measurements *rowa = (Measurements*)a,
               *rowb = (Measurements*)b;
  double anglea = _cmp_sort_face_order__angle_wrt_face( rowa ),
         angleb = _cmp_sort_face_order__angle_wrt_face( rowb );
  static const double rad2quanta = 100.0 * 180.0 / M_PI; // Multiply 180/pi by 100 to set tolerance to 0.01 deg 
  return (int) ((anglea-angleb)*rad2quanta);
}

int cmp_sort_time_face_order( const void* a, const void* b )
{ Measurements *rowa = (Measurements*)a,
               *rowb = (Measurements*)b;
  int dt = rowa->fid - rowb->fid;
  if(dt==0)
  { double anglea = _cmp_sort_face_order__angle_wrt_face( rowa ),
           angleb = _cmp_sort_face_order__angle_wrt_face( rowb );
    static const double rad2quanta = 100.0 * 180.0 / M_PI; // Multiply 180/pi by 100 to set tolerance to 0.01 deg 
    return (int) ((anglea-angleb)*rad2quanta);
  } 
  return dt;
}

int cmp_sort_time_state_face_order( const void* a, const void *b )
{ Measurements *rowa = (Measurements*)a,
               *rowb = (Measurements*)b;
  int d = rowa->fid - rowb->fid;
  if(d==0)
  { d = rowa->state - rowb->state;
    if(d==0)
    { double anglea = _cmp_sort_face_order__angle_wrt_face( rowa ),
            angleb = _cmp_sort_face_order__angle_wrt_face( rowb );
      static const double rad2quanta = 100.0 * 180.0 / M_PI; // Multiply 180/pi by 100 to set tolerance to 0.01 deg 
      return (int) ((anglea-angleb)*rad2quanta);
    }
  }
  return d;
}

SHARED_EXPORT
void Enumerate_Measurements_Table( Measurements *table, int nrows )
{ int i = nrows;
  while(i--)
    table[i].row = i;
}

SHARED_EXPORT
void Sort_Measurements_Table_State_Time( Measurements *table, int nrows )
{ qsort( table, nrows, sizeof(Measurements), cmp_sort_state_time );
}

SHARED_EXPORT
void Sort_Measurements_Table_Time( Measurements *table, int nrows )
{ qsort( table, nrows, sizeof(Measurements), cmp_sort_time );
}

SHARED_EXPORT
void Sort_Measurements_Table_Time_Face( Measurements *table, int nrows )
{ qsort( table, nrows, sizeof(Measurements), cmp_sort_time_face_order );
}

SHARED_EXPORT
void Sort_Measurements_Table_Time_State_Face( Measurements *table, int nrows )
{ qsort( table, nrows, sizeof(Measurements), cmp_sort_time_state_face_order );
}

inline double _diff(double a, double b) 
{ return (a - b);
}

// Assumes `sorted_table` is sorted by Sort_Measurements_Table_State_Time
SHARED_EXPORT
void Measurements_Table_Compute_Velocities( Measurements *sorted_table, int n_rows )
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
SHARED_EXPORT
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

SHARED_EXPORT
Distributions *Alloc_Distributions_For_State_Sorted_Table( Measurements *sorted_table, int n_rows, int n_bins, int *minstate, int* maxstate )
{ return Alloc_Distributions( n_bins,                                                   // n_bins
                              sorted_table[0].n,                                        // n_measures
                              _count_n_states( sorted_table, n_rows, 1, minstate, maxstate ) ); // n_states
}

// The range of these histograms covers the state space sampled by the movie
SHARED_EXPORT
Distributions *Build_Distributions( Measurements *sorted_table, int n_rows, int n_bins )
{ int minstate,maxstate;
  Distributions *d = Alloc_Distributions_For_State_Sorted_Table( sorted_table, n_rows, n_bins, &minstate, &maxstate );
  int i,j,k;
  int n = sorted_table[0].n;
  double *mn, *mx;
  double *delta;
  int measure_stride = d->n_bins,
      state_stride   = d->n_bins * d->n_measures,
      dvol           = d->n_bins * d->n_measures * d->n_states; 

#ifdef DEBUG_BUILD_DISTRIBUTIONS 
  printf("\n\n********************** DEBUG_BUILD_DISTRIBUTIONS\n\n");
#endif

  mn = (double*) Guarded_Malloc( 2 * sizeof(double) * n, "Build distributions - alloc mn and mx" );
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
      mx[j] = MAX( mx[j], v );
    }
  }
  for( j=0; j < n; j++ )
  { d->bin_min[j] = mn[j];
    d->bin_delta[j] = (mx[j]*(1.001) - mn[j]) / ((double) n_bins);
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
    { int ibin = (int) floor(  (data[j] - mn[j]) / delta[j]  );
      hist[ j*measure_stride + ibin ] ++;
#ifdef DEBUG_BUILD_DISTRIBUTIONS 
      if(  !( ibin >= 0 && ibin < n_bins ) )
      { printf("ibin:  %d\n",ibin);
        printf("nbins: %d\n",n_bins);
        printf(" data[%d]: %f\n", j,  data[j] );
        printf("   mn[%d]: %f\n", j,    mn[j] );
        printf("   mx[%d]: %f\n", j,    mx[j] );
        printf("delta[%d]: %f\n", j, delta[j] );
      }
      assert( ibin >= 0 && ibin < n_bins );
#endif
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
      for( k=0; k < d->n_bins; k++ )  // Apply norm, take log2
        h[k] = log2(h[k]) - norm;
    }
  }
  free(mn);
#ifdef DEBUG_BUILD_DISTRIBUTIONS 
  printf("Returning %p\n",d);
  printf("**** Leaving ********* DEBUG_BUILD_DISTRIBUTIONS\n\n");
#endif
  return d;
}

// Builds histograms using the precompted valid velocties.
// This changes the sort order of the table.  The input should be sorted in
//   state,time order.
// These histograms cover the required state space
SHARED_EXPORT
Distributions *Build_Velocity_Distributions( Measurements *sorted_table, int n_rows, int n_bins )
{ int minstate,maxstate;
  Distributions *d = Alloc_Distributions_For_State_Sorted_Table( sorted_table, n_rows, n_bins, &minstate, &maxstate );
  int i,j,k;
  int n = sorted_table[0].n;
  double *mn, *mx;
  double *delta;
  int measure_stride = d->n_bins,
      state_stride   = d->n_bins * d->n_measures,
      dvol           = d->n_bins * d->n_measures * d->n_states; 

#ifdef DEBUG_BUILD_VELOCITY_DISTRIBUTIONS 
  printf("\n\n********************** DEBUG_BUILD_VELOCITY_DISTRIBUTIONS  \n\n");
#endif

  mn = (double*) Guarded_Malloc( 2 * sizeof(double) * n, "Build distributions - alloc mn and mx" );
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
    d->bin_delta[j] = (mx[j]*(1.001) - mn[j]) / ((double)n_bins); 
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
      { int ibin = floor(  (data[j] - mn[j]) / delta[j]  );
        hist[ j*measure_stride + ibin  ] ++;
#ifdef DEBUG_BUILD_VELOCITY_DISTRIBUTIONS
        if(  !( ibin >= 0 && ibin < n_bins ) )
        { printf("ibin:  %d\n",ibin);
          printf("nbins: %d\n",n_bins);
          printf(" data[%d]: %f\n", j,  data[j] );
          printf("   mn[%d]: %f\n", j,    mn[j] );
          printf("   mx[%d]: %f\n", j,    mx[j] );
          printf("delta[%d]: %f\n", j, delta[j] );
        }
        assert( ibin >= 0 && ibin < n_bins );
#endif
      }
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

void Dilate_Distributions( Distributions* dist )
{ int stride = dist->n_bins;
}

// vec must be an array of length dist->n_measures
// Assumes distributions encodes densities as log2 probability
SHARED_EXPORT
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
SHARED_EXPORT
double Eval_Transition_Likelihood_Log2( Distributions *dist, double *prev, double *next, int istate )
{ static double *vec = NULL;
  static int maxn = 0;
  int i = dist->n_measures;
  vec = (double*) request_storage( vec, &maxn, sizeof(double), i, "eval transitions");
  while(i--)
    vec[i] = _diff( next[i], prev[i] );
  
  return Eval_Likelihood_Log2(dist,vec,istate);
}

// sorted_table must be sorted in ascending time order (i.e. ascending fid)
// Start and end should have the same `state` property.
// Algorithm here is to just take the transition with maximum liklihood at 
// each step.  This will be the most probable markov path joining the start
// and end states. 
//
// Returns a vector of pointers into the table that trace out the best path
// This vector will need to be freed later.
//
// FIXME: This algorithm is stupid
//        Should do some kind of path finding...this is just greedy.
SHARED_EXPORT
Measurements **Find_Path_old( Measurements *sorted_table, 
                          int n_rows,
                         Distributions *shape, 
                         Distributions *velocity,
                         Measurements *start,
                         Measurements *end,
                         int minstate,
                         int *npath)
{ int lastfid;
  int path_length;
  Measurements **path;
  double valmax = -DBL_MAX;
  int istep = 0;
  int fid;
  int istate = start->state - minstate; 
  double *ldata = start->data;
  Measurements *last = sorted_table,
               *this = sorted_table,
               *next = NULL;
  int nlast, nthis;
#ifdef DEBUG_FIND_PATH
  printf("*****************************     DEBUG_FIND_PATH\n");
#endif


  lastfid = end->fid;
  path_length = end->fid - start->fid - 1;

  assert(path_length > 0);
  *npath = path_length;
#ifdef DEBUG_FIND_PATH
  printf("Path length: %d \n", path_length );
#endif
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

  while( ((next - sorted_table) < n_rows) && ( fid < lastfid ) )
  { int j;
    valmax = -DBL_MAX;
    for(j=0; j<nthis; j++)  
    { double *tdata = this[j].data;
      double p = Eval_Transition_Likelihood_Log2( velocity, ldata, tdata, istate );
      if( p > valmax )
      { valmax = p;
        path[istep] = this + j;
#ifdef DEBUG_FIND_PATH
        printf("\t%3d: Updating argmax: p = %7.7f (%5d,%2d)\n",istep,p,this[j].fid,this[j].wid);
#endif
      }
    }
    ldata = path[istep++]->data; // replace argmax 

    this = next; // advance `this` to next frame

    fid = this->fid;
    while( (++next)->fid == fid && next - sorted_table < n_rows); //advance `next` to next-next frame
    nthis = next-this;
  }

#ifdef DEBUG_FIND_PATH
  printf("Path:\n");
  printf("(%5d,%2d)<-", end->fid, end->wid );
  while(path_length--)
    printf("(%5d,%2d)<-", path[path_length]->fid, path[path_length]->wid );
  printf("(%5d,%2d)\n\n", start->fid, start->wid );
#endif

#ifdef DEBUG_FIND_PATH
  printf("***  Leaving  ***************     DEBUG_FIND_PATH\n");
#endif

  return path;
}

void dump_doubles(char *filename, double *array, int n)
{ FILE *fp = fopen(filename,"wb");
  fwrite(array,sizeof(double),n,fp);
  fclose(fp);
}

// sorted_table must be sorted in ascending time order (i.e. ascending fid)
// Start and end should have the same `state` property.  Uses the viterbi
// algorithm to find the most probable markov path joining the start and end
// states. 
//
// Returns a vector of pointers into the table that trace out the best path
// This vector will need to be freed later.
//
SHARED_EXPORT
Measurements **Find_Path( Measurements *sorted_table, 
                          int n_rows,
                         Distributions *shape, 
                         Distributions *velocity,
                         Measurements *start,
                         Measurements *end,
                         int minstate,
                         int *npath)
{
  int pathlength = end->fid - start->fid - 1;
  int *sequence  = calloc( pathlength+1, sizeof(int) );
  Measurements *first, *last; //marks edge of gray area in sorted_table
  int target = start->state;
  int nstate;
  static const double baseline_log2p = -1e7;
  Measurements **path = (Measurements**) Guarded_Malloc( sizeof(Measurements*)*(pathlength+1), "Find_Path");
#ifdef DEBUG_FIND_PATH 
  assert(start->state == end->state );
  assert(pathlength>0);   // end should come after start
  assert(start<end);      // table should be sorted
#endif

  if(npath) *npath = pathlength;
  
  // count number of nodes (includes start and end)
  { Measurements *row = start;
    while( row->fid == start->fid ) row++; // scroll to first frame after
    first = row;
    while( row->fid != end->fid   ) row++; // scroll to last item in gray area
    last = row - 1;
  }
  nstate = last - first + 2; //one extra for the end state

  // setup up for generic forward viterbi (inefficient!)
  { double *sprob = malloc( sizeof(double) * nstate );
    double *tprob = malloc( sizeof(double) * nstate * nstate );
    double *eprob = malloc( sizeof(double) * nstate );
    int i,j;

    //transition prob - init
    for(i=0;i<nstate;i++)
      for(j=0;j<nstate;j++)
        tprob[ i*nstate + j ] = baseline_log2p;
    //compute start, transition and emission probs
    { Measurements *row = first,
                   *a, *b, *c;
      // sprob
      for(a = row; a->fid==row->fid; a++)
        sprob[a-first] = 
          Eval_Transition_Likelihood_Log2( velocity, 
                                           start->data, 
                                           a->data, 
                                           target );
      for(i=a-first;i<nstate;i++)
        sprob[i] = baseline_log2p;

      // eprob, tprob
      while( row <= last )
      { c = a = row;
        while( a->fid == row->fid) a++;
        for( ; row < a; row ++ )
        { int off = (row-first)*nstate;
          eprob[row-first] = Eval_Likelihood_Log2( shape, 
                                                   row->data, 
                                                   target );
          if( a->fid != end->fid )
            for(b=a ; b->fid == a->fid; b++ )
              tprob[ off + b-first ] =
                Eval_Transition_Likelihood_Log2( velocity, 
                                                row->data, 
                                                b->data, 
                                                target );
        }
      }
      // do probs for end
      eprob[nstate-1] = 0; //log2(1.0)
      for( ; c <= last; c++ )
        tprob[ (c-first)*nstate + nstate - 1 ] = 
          Eval_Transition_Likelihood_Log2( velocity, 
                                           c->data, 
                                           end->data, 
                                           target );
    }
      
    { ViterbiResult *res = Forward_Viterbi_Log2( sequence, pathlength+1, sprob, tprob, eprob, 1, nstate );
#ifdef DEBUG_FIND_PATH
      assert( res->n == pathlength + 1 );
      printf("Viterbi Result:\n"
             "        Length: %d\n"
             "          Prob: %f\n"
             "         Total: %f\n"
             "    Likelihood: %g\n", res->n, res->prob, res->total, 1.0-pow(2.0,res->prob-res->total) );
      {int x = res->n; while(--x) assert( first[res->sequence[x]].fid == first[res->sequence[x-1]].fid + 1 ); }
#endif
      for( i=0; i<res->n; i++ )
        path[i] = first + res->sequence[i];
      Free_Viterbi_Result(res);
    }

    free(sprob);
    free(tprob);
    free(eprob);
  }


  free(sequence);
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
SHARED_EXPORT
void Solve( Measurements *table, int n_rows, int n_bins )
{ Distributions *shape, *velocity;
  int minstate, maxstate, nstates;
  int nframes;
  int i,j;
#ifdef DEBUG_SOLVE_GRAY_AREAS
  printf("*****************************     DEBUG_SOLVE_GRAY_AREAS\n");
#endif
  
  Sort_Measurements_Table_State_Time( table, n_rows );
  nstates = _count_n_states( table, n_rows, 1, &minstate, &maxstate );
  Measurements_Table_Compute_Velocities( table, n_rows );
  shape    = Build_Distributions( table, n_rows, n_bins );
  velocity = Build_Velocity_Distributions( table, n_rows, n_bins ); // !! Changes the sort order: now table is in time order
  nframes = table[n_rows-1].fid + 1;

#ifdef DEBUG_SOLVE_GRAY_AREAS
  { int x = n_rows; while(--x) assert( (table[x].fid - table[x-1].fid) >= 0 ); } // check sort order 
#endif

  { int *gray_areas = Guarded_Malloc(nframes * sizeof(int), "in solve - alloc gray_areas");
    int ngray = 0;
    
    // Compute trajectories - 
    // Each is an array, nframes long, of pointers into the table
    // FIXME: BUG: What about the edge cases?
    //        For gray areas starting at the beginning or ending at the end of
    //        the movie, the path doesn't have both.
    { Measurements **trajs = Guarded_Malloc( nstates * nframes * sizeof(Measurements*), "in solve, alloc trajectories");
      Measurements *row = table;
      memset(trajs,0, nstates * nframes * sizeof(Measurements*));
      // initialize
      for( ; row < table + n_rows; row++ )                            // Build an index into table [ rows:time, cols:state ]
        trajs[ ( row->state - minstate )*nframes + row->fid ] = row;  //   Assumes (time,state) uniquely identifies whisker segment
      // fill in gray areas
      for( i=1; i <  nstates; i++ ) // forget about minstate (trash state)
      { Measurements** t = trajs + i*nframes;                          // get index relative to current state
  
        // Find the gray areas:
        // A frame is in a `gray area` if there's no appt. labelled segment
        //    Step 1: mask frames
        memset(gray_areas,0,sizeof(int)*nframes);
        for(j=0;j<nframes;j++)
          if(t[j])
            gray_areas[ t[j]->fid ] = 1;

        //    Step 2: Compute the inclusive boundaries of the gray areas in place
        ngray = 0;
        for(j=1;j<nframes;j++)
        { int d = gray_areas[j] - gray_areas[j-1]; 
          if( d == -1 )     // Opening a gray area
            gray_areas[ ngray ] = j;
          else if( d == 1 ) // Closing a gray area
          { gray_areas[ ngray+1 ] = j-1;
            ngray += 2;     // only incriment on closing
          }
        }
#ifdef DEBUG_SOLVE_GRAY_AREAS
        printf("Label %3d: Found %d gray areas\n"
               "           Solving for %d whiskers in %d frames.\n", i, ngray, nstates, nframes);
#endif

        for( j=0; j<ngray; j+=2 )
        { Measurements *start = t[  gray_areas[j  ] - 1  ],   //start  //start and end are pulled from the state == current state segments
                       *end   = t[  gray_areas[j+1] + 1  ];   //end
          if( gray_areas[j] != 0 && gray_areas[j+1] != nframes-1 )
          {
            int npath;
            //if( start && end )
            {
#ifdef DEBUG_SOLVE_GRAY_AREAS
              assert(start && end);
              printf("Running find path from frame %5d to %5d\n", start->fid, end->fid);
#endif
              Measurements **path = Find_Path( table, n_rows, shape, velocity, start, end, minstate, &npath );
              memcpy( t + gray_areas[j], path, sizeof(Measurements*)*npath); 
#ifdef DEBUG_SOLVE_GRAY_AREAS
              assert( start->fid == path[0]->fid      - 1 );
              assert( end->fid   == path[npath-1]->fid + 1);
              printf("\tCopyied solution for %5d to (%5d,%5d).  Size %d. \n", t[gray_areas[j]-1]->state, t[gray_areas[j]]->fid, t[gray_areas[j]]->wid, npath);
              { int x = npath; while(--x) assert( path[x]->fid == path[x-1]->fid + 1 ); } //check frames increment as aexpected
#endif
              free(path);
            }
          }
        }
      }

      // Commit trajectories to table/output trajectories
      for( i=1; i <  nstates; i++ )           // don't worry about minstate (trash state)
      { Measurements** t = trajs + i*nframes;
        for(j=0; j<nframes; j++ )
          if( t[j] )
            t[j]->state = minstate + i;
      }

      free(trajs);
    } // end context - trajs

    free( gray_areas );
  }
#ifdef DEBUG_SOLVE_GRAY_AREAS
  printf("***  Leaving  ***************     DEBUG_SOLVE_GRAY_AREAS\n");
#endif

  Free_Distributions( shape );
  Free_Distributions( velocity );
}

#ifdef TEST_MEASUREMENT_TABLE_IO_1
int main(int argc, char *argv[])
{ int nrows = 40000;
  int ret;
  Measurements *table = Alloc_Measurements_Table(nrows,6);
  printf("Test: Write an empty table to file and read it back in.\n");
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
    Measurements_Table_Compute_Velocities( table, n_rows );
    shape    = Build_Distributions( table, n_rows, n_bins );
    velocity = Build_Velocity_Distributions( table, n_rows, n_bins ); // This changes the sort order, now table is in time order
    nframes = table[n_rows-1].fid + 1;

    Free_Distributions( shape );
    Free_Distributions( velocity );
  }
  Free_Measurements_Table(table);

  if(ret)
    printf("\tTest passed.\n");
  else
    printf("\tTest FAILED.\n");
  return ret;
}
#endif // TEST_BUILD_DISTRIBUTIONS

#ifdef TEST_SOLVE_GRAY_AREAS
char *Spec[] = {"<source:string> <dest:string>", NULL};
int main(int argc, char *argv[])
{ Measurements *table;
  int n_rows = 0;
  int err = 0;
  
  printf("Test: Load measurements, run solve, and save result.\n");
  Process_Arguments(argc,argv,Spec,0);
  table = Measurements_Table_From_Filename( Get_String_Arg("source"), &n_rows);

  Solve( table, n_rows, 32 );
  Sort_Measurements_Table_State_Time( table, n_rows );
  Measurements_Table_Compute_Velocities( table, n_rows );
  
  { int minstate, maxstate, istate;
    int nframes = table[n_rows-1].fid + 1;
    _count_n_states( table, n_rows, 1, &minstate, &maxstate );
    for( istate = minstate; istate <= maxstate; istate++ )
    { int sz = Measurements_Table_Size_Select_State( table, n_rows, istate );
      printf("State %3d has size %d\n",istate, sz );
      assert( istate<0 || sz <= nframes );
    }
  }

  Measurements_Table_To_Filename( Get_String_Arg("dest"), table, n_rows );
  Free_Measurements_Table(table);

  if(err)
    printf("\tTest FAILED.\n");
  else
    printf("\tTest passed.\n");
  return err;
}
#endif // TEST_SOLVE_GRAY_AREAS 
