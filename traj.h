// TODO: change the name of this file to something else, like identity.[ch]
// DONE: make the .h
#ifndef H_TRAJ
#define H_TRAJ

#include "compat.h"
#include <stdio.h>

// FIXME: sizeof(Measurements) changes on 64 vs 32 bit machines ruining file
//        compatibility.
typedef struct _Measurements
{ int row;           // offset from head of data buffer ... Note: the type limits size of table
  int fid;
  int wid;
  int state;

  int face_x;        // used in ordering whiskers on the face...roughly, the center of the face
  int face_y;        //                                      ...does not need to be in image
  int col_follicle_x; // index of the column corresponding to the folicle x position
  int col_follicle_y; // index of the column corresponding to the folicle y position
                                                                           
  int valid_velocity;
  int n;
  double *data;     // array of n elements
  double *velocity; // array of n elements - change in data/time
} Measurements;

typedef struct _Distributions
{ int n_measures;
  int n_states;
  int n_bins;
  double *bin_min;    // array of n_measures elements
  double *bin_delta;  // array of n_measures elements
  double *data;       // array of holding histogram information with dimensions (n_bins,n_measures,n_states)
} Distributions;

SHARED_EXPORT Measurements  *Alloc_Measurements_Table                          ( int n_rows, int n_measurements );
SHARED_EXPORT void           Free_Measurements_Table                           ( Measurements *table );

SHARED_EXPORT Distributions *Alloc_Distributions                               ( int n_bins, int n_measures, int n_states );
SHARED_EXPORT void           Free_Distributions                                ( Distributions *this );
SHARED_EXPORT void           Copy_Distribution_To_Doubles                      ( Distributions *this, double *destination );
SHARED_EXPORT void           Distributions_Bins_To_Doubles                     ( Distributions *this, double *destination );

SHARED_EXPORT void           Measurements_Table_Set_Constant_Face_Position     ( Measurements *table, int n_rows, int x, int y );
SHARED_EXPORT void           Measurements_Table_Set_Follicle_Position_Indices  ( Measurements *table, int n_rows, int ix, int iy );

// Use this to copy in from e.g. matlab, python
// Assumes first three columns are label,fid,wid (all ints) followed by data
// so Measurements.n = n_cols - 3 
//
SHARED_EXPORT Measurements *Measurements_Table_From_Doubles( double *raw, int n_rows, int n_cols );

// Use this to copy in to e.g. matlab, python
// Assumes first three columns are label,fid,wid followed by data
// Velocity data is not copied
SHARED_EXPORT void Measurements_Table_Data_To_Doubles(Measurements *table, int n_rows, double *raw);

// Use this to copy in to e.g. matlab, python
// Assumes first three columns are label,fid,wid followed by data
// Shape data is not copied
// Invalid velocities are set to 0
SHARED_EXPORT void Measurements_Table_Velocity_To_Doubles(Measurements *table, int n_rows, double *raw);

SHARED_EXPORT void Measurements_Table_Copy_Shape_Data( Measurements *table, int n_rows, double *buffer );

SHARED_EXPORT void Measurements_Table_Copy_Velocities( Measurements *table, int n_rows, double *buffer );

SHARED_EXPORT void Measurements_Table_Append_Columns_In_Place( Measurements *table, int n_rows, int n_cols_to_add );

// Returns the number of rows with the queried state
// Scans once over the table.
SHARED_EXPORT int Measurements_Table_Size_Select_State( Measurements *table, int n_rows, int state );

// Selects rows according to their state and returns the `time` and `valid_velocity` arrays.
// time and mask should be the appropriate size. See `Measurements_Table_Size_Select_State`
SHARED_EXPORT void Measurements_Table_Select_Time_And_Mask_By_State( Measurements *table, int n_rows, int state, double *time, int *mask );

// Selects rows according to their state and returns velocities
// `data` should be the appropriate size. See `Measurements_Table_Size_Select_Velocities`
SHARED_EXPORT void Measurements_Table_Select_Velocities_By_State( Measurements *table, int n_rows, int state, double *data );

// Selects rows according to their state and returns shape data
// `data` should be the appropriate size. See `Measurements_Table_Size_Select_Velocities`
SHARED_EXPORT void Measurements_Table_Select_Shape_By_State( Measurements *table, int n_rows, int state, double *data );

SHARED_EXPORT void Enumerate_Measurements_Table( Measurements *table, int nrows );

SHARED_EXPORT void Sort_Measurements_Table_State_Time( Measurements *table, int nrows );

SHARED_EXPORT void Sort_Measurements_Table_Time( Measurements *table, int nrows );

SHARED_EXPORT void Sort_Measurements_Table_Segment_UID( Measurements *table, int nrows );

SHARED_EXPORT void Sort_Measurements_Table_Time_Face( Measurements *table, int nrows );

SHARED_EXPORT void Sort_Measurements_Table_Time_State_Face( Measurements *table, int nrows );

// Assumes `sorted_table` is sorted by Sort_Measurements_Table_State_Time
SHARED_EXPORT void Measurements_Table_Compute_Velocities( Measurements *sorted_table, int n_rows );

SHARED_EXPORT
int _count_n_states( Measurements *table, int n_rows, int sorted, int *minstate, int *maxstate );

SHARED_EXPORT Distributions *Alloc_Distributions_For_State_Sorted_Table( Measurements *sorted_table, int n_rows, int n_bins, int *minstate, int* maxstate );

// The range of these histograms covers the state space sampled by the movie
SHARED_EXPORT Distributions *Build_Distributions( Measurements *sorted_table, int n_rows, int n_bins );

SHARED_EXPORT void Distributions_Dilate( Distributions* dist );
SHARED_EXPORT void Distributions_Normalize( Distributions *d );
SHARED_EXPORT void Distributions_Apply_Log2( Distributions *d );

// Builds histograms using the precompted valid velocties.
// This changes the sort order of the table.  The input should be sorted in
//   state,time order.
// These histograms cover the required state space
SHARED_EXPORT Distributions *Build_Velocity_Distributions( Measurements *sorted_table, int n_rows, int n_bins );

// vec must be an array of length dist->n_measures
// Assumes distributions encodes densities as log2 probability
SHARED_EXPORT double Eval_Likelihood_Log2( Distributions *dist, double *vec, int istate );

// prev and next must be arrays of length dist->n_measures
// Assumes distributions encodes densities as log2 probability
// Distributions should be functions of the differences between next and prev
SHARED_EXPORT double Eval_Velocity_Likelihood_Log2( Distributions *dist, double *prev, double *next, int istate );

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
SHARED_EXPORT Measurements **Find_Path( Measurements *sorted_table, 
                                        int n_rows,              
                                        Distributions *shape,    
                                        Distributions *velocity, 
                                        Measurements *start,     
                                        Measurements *end,       
                                        int minstate,            
                                        int *npath);             

// This function takes a table of measurements where some subset of the frames
// in the movie have been labeled.  That is, a subset of the rows have a
// `state` field that is different than -1.
//
// From this labeled subset two sets of histograms are computed conditioned on 
// the labelled state (for states different than -1).
//
// For unlabelled frames (gray areas), Find_Path will be called to link the labeled
// observations on either side of the gray area.
SHARED_EXPORT void Solve( Measurements *table, int n_rows, int n_shape_bins, int n_vel_bins );

#endif//H_TRAJ
