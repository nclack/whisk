/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : May 2009
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
// TODO: change the name of this file to something else, like identity.c
// TODO: make the .h
#pragma warning(disable : 4996)


#include "compat.h"
#include "traj.h"
#include "measurements_io.h"
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

#include "parameters/param.h"

//#define  IDENTITY_SOLVER_VELOCITY_NBINS   8096
//#define  IDENTITY_SOLVER_SHAPE_NBINS      16

// TESTS
#if 0
#undef   TEST_MEASUREMENT_TABLE_IO_1
#undef   TEST_BUILD_DISTRIBUTIONS
#undef   TEST_SOLVE_GRAY_AREAS
#endif

// DEBUG OUTPUT

#if 0
#define  DEBUG_BUILD_VELOCITY_DISTRIBUTIONS
#define  DEBUG_BUILD_VELOCITY_DISTRIBUTIONS_VERBOSE
#define  DEBUG_EVAL_LIKELIHOOD_LOG2
#define  DEBUG_MEASUREMENTS_TABLE_ALLOC
#define  DEBUG_MEASUREMENTS_TABLE_FROM_FILE
#define  DEBUG_MEASUREMENTS_TABLE_COMPUTE_VELOCITIES
#define  DEBUG_FIND_PATH
#define  DEBUG_TEST_SOLVE_GRAY_AREAS
#define  DEBUG_SOLVE_GRAY_AREAS
#define  DEBUG_DISTRIBUTIONS_ALLOC
#define  DEBUG_BUILD_DISTRIBUTIONS
#endif

SHARED_EXPORT
Measurements *Alloc_Measurements_Table( int n_rows, int n_measurements )
{ Measurements *table = Guarded_Malloc( sizeof(Measurements)*n_rows, "allocate measurements table" );
  double *dataspace   = Guarded_Malloc( 2*sizeof(double)*n_measurements*n_rows, "allocate measurements table" );
  double *velocityspace  = dataspace + n_measurements*n_rows;
#ifdef DEBUG_MEASUREMENTS_TABLE_ALLOC
  debug("\nMeasurements table alloc: %d rows of data at %p\n",n_rows,dataspace);
#endif
  if(!dataspace) return NULL;
  if(!table) return NULL;
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

#define TRY(expr) \
  if(!(expr)) error("%s(%d):\n\tExpression evaluated as false.\n\t%s\n",__FILE__,__LINE__,#expr)

SHARED_EXPORT
Measurements *Realloc_Measurements_Table(Measurements *old, int n_rows_old, int n_rows_new )
{ Measurements *table;
  double *dataspace;
  double *velocityspace;
  int n_measurements,c;
  TRY(old); // pointer may not be NULL
  n_measurements=old->n;
  TRY(table         = realloc(old, sizeof(Measurements)*n_rows_new));
  TRY(dataspace     = realloc(table->data-table->n*table->row, 2*sizeof(double)*n_measurements*n_rows_new));
  velocityspace = dataspace + n_measurements*n_rows_new;
  memmove(velocityspace,
          dataspace+n_measurements*n_rows_old,
          sizeof(double)*n_measurements*n_rows_old);

#ifdef DEBUG_MEASUREMENTS_TABLE_ALLOC
  debug("\nMeasurements table alloc: %d rows of data at %p\n",n_rows,dataspace);
#endif
  if(!dataspace) return NULL;
  if(!table) return NULL;
  c = n_rows_old;
  for(c=0;c<n_rows_old;++c) // fixup pointers/offsets
  { Measurements *row = table     + c;
    row->data     = dataspace     + c*n_measurements;
    row->velocity = velocityspace + c*n_measurements;
    row->row = c;
  }
  for(;c<n_rows_new;++c) // init the rest
  { Measurements *row = table + c;
    row->row = c;
    row->fid = 0;
    row->wid = 0;
    row->state = -1;
    row->face_x = table->face_x;    // copy the first row's attributes
    row->face_y = table->face_y;
    row->col_follicle_x = table->col_follicle_x;
    row->col_follicle_y = table->col_follicle_y;
    row->valid_velocity = table->valid_velocity;
    row->n = n_measurements;
    row->face_axis = table->face_axis;
    row->data      = dataspace     + c*n_measurements;
    row->velocity  = velocityspace + c*n_measurements;
  }
  return table;
}

SHARED_EXPORT
void Free_Measurements_Table( Measurements *table )
{ if( !table ) return;
#ifdef DEBUG_MEASUREMENTS_TABLE_ALLOC
  debug("Measurements table free : data at %p\n",table[0].data - table[0].row * table[0].n);
  debug("\tdata: %p\n",table[0].data);
  debug("\t row: %d\n",table[0].row );
  debug("\t   n: %d\n",table[0].n   );
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
  debug("\nAlloc Distributions:\n"
        "\tnvol   : %d\n"
        "\tdata   : %p\n"
        "\tbindata: %p\n",nvol,data, bindata );
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
  debug("\nFree Distributions:\n");
  debug("\tdata: %p\n", this->data );
  debug("\tbindata: %p\n", this->bin_min );
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

// This does not initialize memory in added rows
// Velocities need to be recomputed after this
SHARED_EXPORT
void Measurements_Table_Append_Columns_In_Place( Measurements *table, int n_rows, int n_cols_to_add )
{ int n    = table[0].n,        //original number of columns
      ncol = n + n_cols_to_add; //new      number of columns
  double *buffer = table[0].data - n*table[0].row; //row gives offset from head of data buffer

  if( !n_cols_to_add ) return; //noop
  assert( n_cols_to_add > 0 );

  // 0. Realloc data buffer - invalidates all table[i].data and velocity pointers!
  buffer = Guarded_Realloc( buffer,
      sizeof(double)*(n+n_cols_to_add)*n_rows,
      "Measurements_Table_Append_Columns_In_Place" );

  // 1. copy data from back to front through expanded buffer
  { double *row = buffer +    n*n_rows,
           *dst = buffer + ncol*n_rows;
    while( (row-=n) >= buffer )
      memcpy( dst -= ncol, row, n*sizeof(double) );
  }

  // 2. update row pointers and column count
  { Measurements *row = table  + n_rows;
    while( row-- > table )
    { int offset = row->row;
      row->data     = buffer +          offset * ncol;
      row->velocity = buffer + (n_rows+offset) * ncol;
      row->n        = ncol;
    }
  }
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

int test_Measurements_Table_FileIO( char* filename,  Measurements *table, int n_rows )
{ Measurements *t2;
  int nr2,i;

  Measurements_Table_To_Filename( filename, NULL, table, n_rows );

  t2 = Measurements_Table_From_Filename( filename, NULL, &nr2 );
  if(!t2) error("Couldn't read %s\n",filename);

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

static int cmp_sort_state_time( const void* a, const void* b )
{ Measurements *rowa = (Measurements*)a,
               *rowb = (Measurements*)b;
  int dstate = (rowa->state) - (rowb->state);
  return (dstate==0) ? (rowa->fid - rowb->fid) : dstate;
}

static int cmp_sort_time( const void* a, const void* b )
{ Measurements *rowa = (Measurements*)a,
               *rowb = (Measurements*)b;
  return (rowa->fid - rowb->fid);
}

static int cmp_sort_segment_uid( const void* a, const void* b )
{ Measurements *rowa = (Measurements*)a,
               *rowb = (Measurements*)b;
  int d = (rowa->fid - rowb->fid);
  return (d!=0) ? d: (rowa->wid - rowb->wid);
}

/*
static inline double _cmp_sort_face_order__angle_wrt_face( Measurements *a )
{ int ix = a->col_follicle_x,
      iy = a->col_follicle_y,
       v = atan2( a->data[iy] - a->face_y,
                  a->data[ix] - a->face_x );
  //error("Don't use...use ccw test instead\n");  //why did I put this here :\
  return (v<0.0)?-1:((v>0.0)?1:0);
}
*/


static inline int _cmp_sort_face__ccw_test( Measurements *a, Measurements *b )
{ int ix = a->col_follicle_x,
      iy = a->col_follicle_y;
  double ax = a->data[ix] - a->face_x,
         ay = a->data[iy] - a->face_y,
         bx = b->data[ix] - b->face_x,
         by = b->data[iy] - b->face_y,
         v  = ax*by - ay*bx;
  debug("(%s:%d)\n\tDon't use...ccw test has is fragile when a and b are colinear\n",__FILE__,__LINE__);
  return (v<0.0)?-1:((v>0.0)?1:0);
}

static inline int _cmp_sort_face__angle_wrt_face( Measurements *a, Measurements *b )
{ int ix = a->col_follicle_x,
      iy = a->col_follicle_y;
  double ax = a->data[ix] - a->face_x, // translate origin to face pos
         ay = a->data[iy] - a->face_y,
         bx = b->data[ix] - b->face_x,
         by = b->data[iy] - b->face_y,
         a_angle,
         b_angle;
  { // compute follicle angles wrt face orientation (face is 0 deg)
    // assert(a->face_axis == b->face_axis);
    switch(a->face_axis)
    { case 'h':
      case 'x':
        a_angle = atan2(ay,ax);
        b_angle = atan2(by,bx);
        break;
      case 'v':
      case 'y':
        a_angle = atan2(ax,ay);
        b_angle = atan2(bx,by);
        break;
      default:
        debug("In %s:Line %d \n"
              "\tParameter <face_axis> must take on a value of 'x','h','v' or 'y'\n"
              "\tGot value %c\n"
              "\tUsing backup whisker ordering function.\n",__FILE__,__LINE__,a->face_axis);
        return _cmp_sort_face__ccw_test(a,b);
    }
  }
  return (a_angle<b_angle)?-1:((a_angle>b_angle)?1:0);
}

static int cmp_sort_face_order( const void* a, const void* b )
{
  return _cmp_sort_face__angle_wrt_face((Measurements*)a,(Measurements*)b);
  //return _cmp_sort_face__ccw_test((Measurements*)a,(Measurements*)b);
}

static int cmp_sort_time_face_order( const void* a, const void* b )
{ Measurements *rowa = (Measurements*)a,
               *rowb = (Measurements*)b;
  int dt = rowa->fid - rowb->fid;
  if(dt==0)
    return (int) cmp_sort_face_order(a,b);
  return dt;
}

static int cmp_sort_time_state_face_order( const void* a, const void *b )
{ Measurements *rowa = (Measurements*)a,
               *rowb = (Measurements*)b;
  int d = rowa->fid - rowb->fid;
  if(d==0)
  { d = rowa->state - rowb->state;
    if(d==0)
      return (int) cmp_sort_face_order(a,b);
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
void Sort_Measurements_Table_Segment_UID( Measurements *table, int nrows )
{ qsort( table, nrows, sizeof(Measurements), cmp_sort_segment_uid );
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
{ double c = a-b;
  return c*c;
}

// Assumes `sorted_table` is sorted by Sort_Measurements_Table_State_Time
// Computes velocities only for states >= 0
SHARED_EXPORT
void Measurements_Table_Compute_Velocities( Measurements *sorted_table, int n_rows )
{ int i;
  int n = sorted_table[0].n;
#ifdef DEBUG_MEASUREMENTS_TABLE_COMPUTE_VELOCITIES
  int nr = n_rows;
  debug("\nCompute velocities for table at %p with %d rows.\n", sorted_table, n_rows );
#endif
  while( n_rows-- > 1 )
  { Measurements *a = sorted_table + n_rows - 1,
                 *b = sorted_table + n_rows;
    if( (b->state >= 0) && (b->fid - a->fid) == 1 )
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
#ifdef DEBUG_MEASUREMENTS_TABLE_COMPUTE_VELOCITIES
  debug("\tCheck for any valid velocities after computation.\n");
  while(nr--)
    if( sorted_table[nr].valid_velocity )
      break;
  if( nr<0 ) warning("No valid velocities.\n");
  else       debug("\tok\n");
#endif
  // Handle the first row: (remember the sort)
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
  int i,j;
  int n = sorted_table[0].n;
  double *mn, *mx;
  double *delta;
  int measure_stride = d->n_bins,
      state_stride   = d->n_bins * d->n_measures,
      dvol           = d->n_bins * d->n_measures * d->n_states;

#ifdef DEBUG_BUILD_DISTRIBUTIONS
  debug("\n\n********************** DEBUG_BUILD_DISTRIBUTIONS\n\n");
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
      { debug("ibin:  %d\n",ibin);
        debug("nbins: %d\n",n_bins);
        debug(" data[%d]: %f\n", j,  data[j] );
        debug("   mn[%d]: %f\n", j,    mn[j] );
        debug("   mx[%d]: %f\n", j,    mx[j] );
        debug("delta[%d]: %f\n", j, delta[j] );
      }
      assert( ibin >= 0 && ibin < n_bins );
#endif
    }
  }

  free(mn);
#ifdef DEBUG_BUILD_DISTRIBUTIONS
  debug("Returning %p\n",d);
  debug("**** Leaving ********* DEBUG_BUILD_DISTRIBUTIONS\n\n");
#endif
  return d;
}

// Builds histograms using the precompted valid velocties.
// This changes the sort order of the table.  The input should be sorted in
//   state,time order.  The table is in time order after.
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
  debug("\n\n********************** DEBUG_BUILD_VELOCITY_DISTRIBUTIONS\n"
            "For table at %p with %d rows.  Will use %d bins.\n\n",
            sorted_table, n_rows, n_bins);
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

    while( (this - sorted_table < n_rows) && (this->fid == fid) )
      ++this;
    nlast = this-last;
    next = this;

    for( j=0; j < n; j++ )
    { double v = _diff( this->data[j], last->data[j] );
      mx[j] = v;
      mn[j] = v;
    }

    while( this - sorted_table < n_rows )
    {
      fid = this->fid;
      while( (next - sorted_table < n_rows) && (next->fid == fid) )
        next++;
      nthis = next-this;
#ifdef DEBUG_BUILD_VELOCITY_DISTRIBUTIONS_VERBOSE
      debug("        fid     n\n");
      debug("last: %5d %5d\n",last->fid,nlast);
      debug("this: %5d %5d\n",this->fid,nthis);
      if(next - sorted_table < n_rows)
        debug("next: %5d    \n",next->fid);
      else
        debug("next: at end\n");
      debug("row [%7d/%-7d]\n",next - sorted_table, n_rows);
      debug("\n");
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

    }
  } // end context - get extents

  for( j=0; j < n; j++ )
  { d->bin_min[j] = mn[j];
    d->bin_delta[j] = (mx[j]*(1.001) - mn[j]) / ((double)n_bins);
  }
  delta = d->bin_delta;

  // Accumulate
#ifdef DEBUG_BUILD_VELOCITY_DISTRIBUTIONS
  // Requires some valid velocities
  // (see: Measurements_Table_Compute_Velocities to compute)
  debug("Checking to see if there are any valid computed velocities to histogram\n");
  for( i=0; i < n_rows; i++ )
    if( sorted_table[i].valid_velocity )
      break;
  assert( i != n_rows ); // fails if no valid velocities
#endif
  memset( d->data, 0, sizeof(double) * dvol );
  for( i=0; i < n_rows; i++ )
  { Measurements *mrow = sorted_table + i;
    if( mrow->valid_velocity )
    { double *data = mrow->velocity;
      int istate   = mrow->state - minstate;
      double *hist = d->data + istate * state_stride;
      for( j=0; j<n; j++ )
      { int ibin = (int) floor(  (data[j] - mn[j]) / delta[j]  );
        hist[ j*measure_stride + ibin  ] ++;
#ifdef DEBUG_BUILD_VELOCITY_DISTRIBUTIONS
        if(  !( ibin >= 0 && ibin < n_bins ) )
        { debug("ibin:  %d\n",ibin);
          debug("nbins: %d\n",n_bins);
          debug(" data[%d]: %f\n", j,  data[j] );
          debug("   mn[%d]: %f\n", j,    mn[j] );
          debug("   mx[%d]: %f\n", j,    mx[j] );
          debug("delta[%d]: %f\n", j, delta[j] );
        }
        assert( ibin >= 0 && ibin < n_bins );
#endif
      }
    }
  }
  // Compute non-corresponding velocity distribution
  //     for following sort order must be by Time
  //     (enforced above)
  { Measurements *last = sorted_table,
                  *this = sorted_table,
                  *next = NULL;
    int nlast, nthis;
    int fid = last->fid;
    double *hist = d->data; //state == minstate

    while( (this - sorted_table < n_rows) && (this->fid == fid) )
      ++this;
    nlast = this-last;
    next = this;

    while( this - sorted_table < n_rows )
    { fid = this->fid;
      while( (next - sorted_table < n_rows) && (next->fid == fid) )
        next++;
      nthis = next-this;
#ifdef DEBUG_BUILD_VELOCITY_DISTRIBUTIONS_VERBOSE
      debug("        fid     n\n");
      debug("last: %5d %5d\n",last->fid,nlast);
      debug("this: %5d %5d\n",this->fid,nthis);
      if(next - sorted_table < n_rows)
        debug("next: %5d    \n",next->fid);
      else
        debug("next: at end\n");
      debug("row [%7d/%-7d]\n",next - sorted_table, n_rows);
      debug("\n");
#endif

      for(i=0; i<nlast; i++)
      { double *ldata = last[i].data;
        for(j=0; j<nthis; j++)
        { double *tdata = this[j].data;
          for(k=0; k<n; k++)
          { double diff = _diff( tdata[k], ldata[k] );
            int ibin = (int) floor(  (diff - mn[k]) / delta[k]  );
            hist[ k*measure_stride + ibin  ] ++;
#ifdef DEBUG_BUILD_VELOCITY_DISTRIBUTIONS
            if(  !( ibin >= 0 && ibin < n_bins ) )
            { debug("   ibin:  %d\n",ibin);
              debug("   nbins: %d\n",n_bins);
              debug("    data[%d]: %f\n", j,  diff    );
              debug("      mn[%d]: %f\n", j,    mn[k] );
              debug("      mx[%d]: %f\n", j,    mx[k] );
              debug("   delta[%d]: %f\n", j, delta[k] );
            }
            assert( ibin >= 0 && ibin < n_bins );
#endif
          }
        }
      }
      last = this;
      nlast = nthis;
      this = next;

    }
  } // end context - get extents

  return d;
}

void Distributions_Normalize( Distributions *d )
{ int i,j,k;
  int measure_stride = d->n_bins,
      state_stride   = d->n_bins * d->n_measures,
      dvol           = d->n_bins * d->n_measures * d->n_states;
  // Normalize
  for( i=0; i < d->n_states; i++ )
  { double *hists = d->data + i * state_stride;
    for( j=0; j < d->n_measures; j++ )
    { double *h = hists + j * measure_stride;
      double norm = 0;
      for( k=0; k < d->n_bins; k++ )  // Add one to each bin (want something like upper bound for probs)
        h[k]++;
      for( k=0; k < d->n_bins; k++ )  // Accumulate
        norm += h[k];
      for( k=0; k < d->n_bins; k++ )  // Apply norm
        h[k] = h[k] / norm;
    }
  }
}

void Distributions_Apply_Log2( Distributions *d )
{ double *data = d->data,
         *e = d->data + d->n_states * d->n_measures * d->n_bins;
  while(e-- > data)
    *e = log2(*e);
}

void Distributions_Dilate( Distributions* dist )
{ int stride = dist->n_bins;
  double *a = dist->data + dist->n_bins * dist->n_measures * dist->n_states;
  while( (a-=stride) > dist->data )
    maxfilt_centered_double_inplace( a, stride, 3 );
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
  int nbins = dist->n_bins;

  for(i=0; i < dist->n_measures; i++)
  { ibin = (int) floor( ( vec[i] - dist->bin_min[i] ) / ( dist->bin_delta[i] ) );
#ifdef DEBUG_EVAL_LIKELIHOOD_LOG2
    if( ibin<0 || ibin >= nbins )
      warning("In Eval_Likelihood_Log2\n"
              "\tibin out of range\n"
              "\t\tibin : %d\n"
              "\t\tnbins: %d\n", ibin,nbins);
#endif
    ibin = CLAMP(ibin,0,nbins-1);
    acc += hists[measure_stride*i + ibin]; // addition here bc probs are in log space
#ifdef DEBUG_EVAL_LIKELIHOOD_LOG2
    debug("\t%5d %f\n", ibin, hists[measure_stride*i + ibin]);
#endif
  }
#ifdef DEBUG_EVAL_LIKELIHOOD_LOG2
    debug("\t\t%f\n", acc);
#endif
  return acc;
}

// prev and next must be arrays of length dist->n_measures
// Assumes distributions encodes densities as log2 probability
// Distributions should be functions of the differences between next and prev
SHARED_EXPORT
double Eval_Velocity_Likelihood_Log2( Distributions *dist, double *prev, double *next, int istate )
{ static double *vec = NULL;
  static size_t maxn = 0;
  int i = dist->n_measures;
  vec = (double*) request_storage( vec, &maxn, sizeof(double), i, "eval transitions");
  while(i--)
    vec[i] = _diff( next[i], prev[i] );

  return Eval_Likelihood_Log2(dist,vec,istate);
}

// sorted_table must be sorted in ascending time order (i.e. ascending fid)
// Start and end should have the same `state` property.  Uses the Dijkstra
// algorithm (which is simplified for the lattice structure used here )
// to find the most likely markov path between start and end.
//
// Returns a vector of pointers into the table that trace out the best path
// This vector is statically allocated...which is a bit silly since we know
// where to put the data...should just pass in a pointer.  doing it this
// way requires a copy and additional memory management.
//

typedef struct _LatticeNode {
  Measurements        *row;
  struct _LatticeNode *argmax;
  double               max;
  struct _LatticeNode *children;    // these are in the lattice and all the children are
  unsigned int         nchildren;   //    sequential in memory
} LatticeNode;

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
  Measurements *first, *last; //marks edge of gray area in sorted_table
  Measurements *eot = sorted_table + n_rows; //end of table
  int target = start->state;
  int nnode;
  static const double baseline_log2p = -1e7;
  static LatticeNode *lattice = NULL;
  static size_t lattice_size = 0;
  static Measurements **result    = NULL;
  static size_t         result_size = 0;


#ifdef DEBUG_FIND_PATH
  assert(start->state == end->state );
  assert(pathlength>0);   // end should come after start
  assert(start<end);      // table should be sorted
#endif

  if(npath) *npath = pathlength;

  //
  // count number of nodes (includes start and end)
  //
  { Measurements *row = start;
    while( row->fid == start->fid && row < eot) row++; // scroll to first frame after
    first = row;
    while( row->fid != end->fid && row < eot ) row++; // scroll to last item in gray area
    last = row - 1;
#ifdef DEBUG_FIND_PATH
    assert( last > first );
#endif
  }
  nnode  = last - first + 3; //one extra for the end state, and one for the start

  lattice = request_storage( lattice, &lattice_size, sizeof(LatticeNode), nnode, "alloc lattice" );

  //
  // init lattice
  //
  { Measurements *row, *next, *nextnext;
    LatticeNode  *cur = lattice + 1;

    memset(lattice, 0, sizeof(lattice)*nnode);

    for( cur = lattice; cur < lattice + nnode; cur++ )
      cur->max = baseline_log2p;

    cur = lattice + 1;
    row = first;
    next = row;
    while( next->fid == row->fid && next < eot)
      next++; // scroll to first frame after

    lattice[nnode-1].row = end;
    lattice[0].row = start;
    lattice[0].children  = lattice + 1;
    lattice[0].nchildren = next - first;
    while(next <= last && next < eot)
    { nextnext = next;
      while( nextnext->fid == next->fid && nextnext < eot)
        nextnext++; // scroll to second frame after (for child count)

      for( ; row < next; row++ )
      { cur->row       = row;
        cur->children  = lattice + (next-first + 1);
        cur->nchildren = nextnext - next;
        cur++;
      }
#ifdef DEBUG_FIND_PATH
      assert( row == next );
#endif
      next = nextnext;
    }
    while(row <= last && row < eot) // last row all point to end
    { for( ; row < next; row++ )
      { cur->row       = row;
        cur->children  = lattice + nnode - 1; // (next-first + 1);
        cur->nchildren = 1;
        cur++;
      }
#ifdef DEBUG_FIND_PATH
      assert( next-first+1 == nnode-1 );
      assert( row == next );
#endif
      next = nextnext;
    }
  }

  //
  // Find best path through lattice
  // Note that nodes in array are already in topological order
  // Update children's parent pointer to point to the parent maximizing
  // probability.
  //
  { LatticeNode *cur;
    int st = start->state;
    for( cur = lattice; cur < lattice + nnode - 1; cur++ ) //resist the urge to do the end
    { Measurements *currow = cur->row;
      LatticeNode *child;
      double self_likelihood = Eval_Likelihood_Log2( shape, currow->data, st-minstate );
      { for(child = cur->children;
            child < cur->children + cur->nchildren;
            child++)
        { double logp;
          logp = Eval_Velocity_Likelihood_Log2( velocity, currow->data, child->row->data, st-minstate )
                + self_likelihood;
#ifdef DEBUG_FIND_PATH
          debug("State: %2d Max: %7.7f Cur: %7.7f %p %p\n", st, child->max, logp, currow->data, child->row->data );
#endif
          if( logp > child->max )
          { child->max    = logp;
            child->argmax = cur;
          }
        }
      }
    }
  }

  //
  // trace back
  //
  result = request_storage(
      result,
      &result_size,
      sizeof(Measurements*),
      pathlength,
      "alloc result in find paths (solve gray areas)" );
  { LatticeNode  *node = lattice + nnode - 1;
    Measurements **cur = result + pathlength;
    while( (node = node->argmax) != lattice )
      *(--cur) = node->row;
  }

  return result;

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
void Solve( Measurements *table, int n_rows, int n_shape_bins, int n_vel_bins )
{ Distributions *shape, *velocity;
  int minstate, maxstate, nstates;
  int nframes;
  int i,j;
#ifdef DEBUG_SOLVE_GRAY_AREAS
  debug("*****************************     DEBUG_SOLVE_GRAY_AREAS\n");
#endif

  Sort_Measurements_Table_State_Time( table, n_rows );
  nstates = _count_n_states( table, n_rows, 1, &minstate, &maxstate );
  Measurements_Table_Compute_Velocities( table, n_rows );
  shape    = Build_Distributions( table, n_rows, n_shape_bins );
  velocity = Build_Velocity_Distributions( table, n_rows, n_vel_bins ); // !! Changes the sort order: now table is in time order
  Distributions_Dilate( shape );
  Distributions_Dilate( velocity );
  Distributions_Normalize( shape );
  Distributions_Normalize( velocity );
  Distributions_Apply_Log2( shape );
  Distributions_Apply_Log2( velocity );
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
        debug("Label %3d: Found %d gray areas\n"
               "           Solving for %d whiskers in %d frames.\n", i, ngray, nstates, nframes);
#endif

        for( j=0; j<ngray; j+=2 )
        { Measurements *start = t[  gray_areas[j  ] - 1  ],   //start  //start and end are pulled from the state == current state segments
                       *end   = t[  gray_areas[j+1] + 1  ];   //end
          if( gray_areas[j] != 0 && gray_areas[j+1] != nframes-1 )
          {
            int npath;
            if( start && end )
            { Measurements **path;
#ifdef DEBUG_SOLVE_GRAY_AREAS
              assert(start && end);
              debug("Running find path from frame %5d to %5d\n", start->fid, end->fid);
#endif
              path = Find_Path( table, n_rows, shape, velocity, start, end, minstate, &npath );
              memcpy( t + gray_areas[j], path, sizeof(Measurements*)*npath);
#ifdef DEBUG_SOLVE_GRAY_AREAS
              assert( start->fid == path[0]->fid      - 1 );
              assert( end->fid   == path[npath-1]->fid + 1);
              debug("\tCopyied solution for %5d to (%5d,%5d).  Size %d. \n",
                  t[gray_areas[j]-1]->state,
                  t[gray_areas[j]]->fid,
                  t[gray_areas[j]]->wid,
                  npath);
              { int x = npath; while(--x) assert( path[x]->fid == path[x-1]->fid + 1 ); } //check frames increment as expected
#endif
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
  debug("***  Leaving  ***************     DEBUG_SOLVE_GRAY_AREAS\n");
#endif

  Free_Distributions( shape );
  Free_Distributions( velocity );
}

#ifdef TEST_MEASUREMENT_TABLE_IO_1
int main(int argc, char *argv[])
{ int nrows = 40000;
  int ret;
  Measurements *table = Alloc_Measurements_Table(nrows,6);
  debug("Test: Write an empty table to file and read it back in.\n");
  ret = test_Measurements_Table_FileIO( "tmp.measurements", table, nrows );
  Free_Measurements_Table(table);
  if(ret)
    debug("\tTest passed.\n");
  else
    debug("\tTest FAILED.\n");
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

  debug("Test: Load measurements and compute distributions.\n");
  Process_Arguments(argc,argv,Spec,0);
  table = Measurements_Table_From_Filename( Get_String_Arg("filename"), NULL, &n_rows);
  if(!table) error("Couldn't read %s\n",Get_String_Arg("filename"));

  { Distributions *shape, *velocity;
    int minstate, maxstate, nstates;
    int nframes;
    int i,j;

    Sort_Measurements_Table_State_Time( table, n_rows );
    nstates = _count_n_states( table, n_rows, 1, &minstate, &maxstate );
    Measurements_Table_Compute_Velocities( table, n_rows );
    shape    = Build_Distributions( table, n_rows, n_bins );
    velocity = Build_Velocity_Distributions( table, n_rows, n_bins ); // This changes the sort order, now table is in time order
    Distributions_Normalize( shape );
    Distributions_Normalize( velocity );
    Distributions_Apply_Log2( shape );
    Distributions_Apply_Log2( velocity );
    nframes = table[n_rows-1].fid + 1;

    Free_Distributions( shape );
    Free_Distributions( velocity );
  }
  Free_Measurements_Table(table);

  if(ret)
    debug("\tTest passed.\n");
  else
    debug("\tTest FAILED.\n");
  return ret;
}
#endif // TEST_BUILD_DISTRIBUTIONS

#ifdef TEST_SOLVE_GRAY_AREAS
char *Spec[] = {"<source:string> <dest:string>", NULL};
int main(int argc, char *argv[])
{ Measurements *table;
  int n_rows = 0;
  int err = 0;

  debug("Test: Load measurements, run solve, and save result.\n");
  Process_Arguments(argc,argv,Spec,0);
  table = Measurements_Table_From_Filename( Get_String_Arg("source"), NULL, &n_rows);
  if(!table) error("Couldn't read %s\n",Get_String_Arg("source"));

  Solve( table, n_rows, IDENTITY_SOLVER_SHAPE_NBINS, IDENTITY_SOLVER_VELOCITY_NBINS );
  Sort_Measurements_Table_State_Time( table, n_rows );
  Measurements_Table_Compute_Velocities( table, n_rows );

  { int minstate, maxstate, istate;
    int nframes = table[n_rows-1].fid + 1;
    _count_n_states( table, n_rows, 1, &minstate, &maxstate );
    for( istate = minstate; istate <= maxstate; istate++ )
    { int sz = Measurements_Table_Size_Select_State( table, n_rows, istate );
#ifdef DEBUG_TEST_SOLVE_GRAY_AREAS
      debug("State %3d has size %d\n",istate, sz );
      assert( istate<0 || sz <= nframes );
#endif
    }
  }

  Measurements_Table_To_Filename( Get_String_Arg("dest"), NULL, table, n_rows );
  Free_Measurements_Table(table);

//if(err)
//  debug("\tTest FAILED.\n");
//else
//  debug("\tTest passed.\n");
  return err;
}
#endif // TEST_SOLVE_GRAY_AREAS
