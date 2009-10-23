//
// TODO: What are termed "deletions" should be "insertions."  Also "junk"
//       states are essentially repeated insertions states.  Maybe there is a
//       better name than "junk" (i.e. other)
//
#include "compat.h"
#include <assert.h>
#include <string.h>
#include "error.h"
#include "utilities.h"
#include "traj.h"
#include "viterbi.h"
#include "common.h"
#include "hmm-reclassify.h"

// Make one model the default
#ifndef TEST_HMM_RECLASSIFY_LR_MODEL_W_DELETIONS
#define TEST_HMM_RECLASSIFY_LR_MODEL
#endif

//
// DEBUG DEFINES
//
#if 0
#define DEBUG_HMM_RECLASSIFY
#define DEBUG_HMM_RECLASSIFY_EXTRA
#endif

#if 1 // setup tests

#if defined( TEST_HMM_RECLASSIFY_1 )
#define TEST_HMM_RECLASSIFY_LR_MODEL
#define  TEST_HMM_RECLASSIFY_NO_DELTAS

#elif defined( TEST_HMM_RECLASSIFY_2 )
#define TEST_HMM_RECLASSIFY_LR_MODEL_W_DELETIONS
#define TEST_HMM_RECLASSIFY_NO_DELTAS

#elif defined( TEST_HMM_RECLASSIFY_3 )
#define TEST_HMM_RECLASSIFY_LR_MODEL
#define TEST_HMM_RECLASSIFY_W_DELTAS

#elif defined( TEST_HMM_RECLASSIFY_4 )
#define TEST_HMM_RECLASSIFY_LR_MODEL_W_DELETIONS
#define TEST_HMM_RECLASSIFY_W_DELTAS

#elif defined( TEST_HMM_RECLASSIFY_5 )
#define TEST_HMM_RECLASSIFY_LR_MODEL
#define TEST_HMM_RECLASSIFY_WATERSHED
#endif

#else

#define TEST_HMM_RECLASSIFY_LR_MODEL
#define TEST_HMM_RECLASSIFY_W_DELTAS

#endif

#define HMM_RECLASSIFY_SHP_DISTS_NBINS   (16)
#define HMM_RECLASSIFY_VEL_DISTS_NBINS   (8096)
#define HMM_RECLASSIFY_BASELINE_LOG2 (-500.0)


typedef int   (*Tpf_State_Count)                                             ( int nwhisk );
typedef real* (*Tpf_Alloc_Transitions)                                       ( int nwhisk );
typedef real* (*Tpf_Init_Uniform_Transitions)                                ( real *T, int nwhisk );
typedef void  (*Tpf_Estimate_Transitions)                                    ( real *T, int nwhisk, Measurements *table, int nrows );
typedef void  (*Tpf_Log2_Transitions)                                        ( real *T, int nwhisk, real baseline_log2 );
typedef real* (*Tpf_Alloc_Starts)                                            ( int nwhisk );
typedef void  (*Tpf_Compute_Starts_For_Two_Classes_Log2)                     ( real *S, real *T, int nwhisk, Measurements *first, Distributions *shp_dists );
typedef void  (*Tpf_Compute_Starts_For_Distinct_Whiskers_Log2)               ( real *S, real *T, int nwhisk, Measurements *first, Distributions *shp_dists );
typedef real* (*Tpf_Alloc_Emissions)                                         ( int nwhisk, int nobs );
typedef real* (*Tpf_Request_Static_Resizable_Emissions)                      ( int nwhisk, int nobs );
typedef void  (*Tpf_Compute_Emissions_For_Two_Classes_Log2)                  ( real *E, int nwhisk, Measurements *obs, int nobs, Distributions *shp_dists );
typedef void  (*Tpf_Compute_Emissions_For_Distinct_Whiskers_Log2)            ( real *E, int nwhisk, Measurements *obs, int nobs, Distributions *shp_dists );
typedef void  (*Tpf_Compute_Emissions_For_Two_Classes_W_History_Log2)        ( real *E, int nwhisk, Measurements *obs, int nobs, Measurements_Reference *prev, Distributions *shp_dists, Distributions *vel_dists );
typedef void  (*Tpf_Compute_Emissions_For_Two_Classes_W_Prev_And_Next_Log2)  ( real *E, int nwhisk, Measurements *obs, int nobs, Measurements_Reference *prev, Measurements_Reference *next, Distributions *shp_dists, Distributions *vel_dists );
typedef int   (*Tpf_State_Decode)                                            ( int state );


#ifdef TEST_HMM_RECLASSIFY_LR_MODEL
#include "hmm-reclassify-lrmodel.h"
Tpf_State_Count                                             pf_State_Count                                            =  LRModel_State_Count;
Tpf_Alloc_Transitions                                       pf_Alloc_Transitions                                      =  LRModel_Alloc_Transitions;
Tpf_Init_Uniform_Transitions                                pf_Init_Uniform_Transitions                               =  LRModel_Init_Uniform_Transitions;
Tpf_Estimate_Transitions                                    pf_Estimate_Transitions                                   =  LRModel_Estimate_Transitions;
Tpf_Log2_Transitions                                        pf_Log2_Transitions                                       =  LRModel_Log2_Transitions;
Tpf_Alloc_Starts                                            pf_Alloc_Starts                                           =  LRModel_Alloc_Starts;
Tpf_Compute_Starts_For_Two_Classes_Log2                     pf_Compute_Starts_For_Two_Classes_Log2                    =  LRModel_Compute_Starts_For_Two_Classes_Log2;
Tpf_Compute_Starts_For_Distinct_Whiskers_Log2               pf_Compute_Starts_For_Distinct_Whiskers_Log2              =  LRModel_Compute_Starts_For_Distinct_Whiskers_Log2;
Tpf_Alloc_Emissions                                         pf_Alloc_Emissions                                        =  LRModel_Alloc_Emissions;
Tpf_Request_Static_Resizable_Emissions                      pf_Request_Static_Resizable_Emissions                     =  LRModel_Request_Static_Resizable_Emissions;
Tpf_Compute_Emissions_For_Two_Classes_Log2                  pf_Compute_Emissions_For_Two_Classes_Log2                 =  LRModel_Compute_Emissions_For_Two_Classes_Log2;
Tpf_Compute_Emissions_For_Distinct_Whiskers_Log2            pf_Compute_Emissions_For_Distinct_Whiskers_Log2           =  LRModel_Compute_Emissions_For_Distinct_Whiskers_Log2;
Tpf_Compute_Emissions_For_Two_Classes_W_History_Log2        pf_Compute_Emissions_For_Two_Classes_W_History_Log2       =  LRModel_Compute_Emissions_For_Two_Classes_W_History_Log2;
Tpf_Compute_Emissions_For_Two_Classes_W_Prev_And_Next_Log2  pf_Compute_Emissions_For_Two_Classes_W_Prev_And_Next_Log2 =  LRModel_Compute_Emissions_For_Two_Classes_W_Prev_And_Next_Log2;
Tpf_State_Decode                                            pf_State_Decode                                           =  LRModel_State_Decode;
#endif

//#ifdef  TEST_HMM_RECLASSIFY_LR_MODEL_W_DELETIONS
//#include "hmm-reclassify-lrmodel-w-deletions.h"
//Tpf_State_Count                                       pf_State_Count                                       =  LRDelModel_State_Count;
//Tpf_Alloc_Transitions                                 pf_Alloc_Transitions                                 =  LRDelModel_Alloc_Transitions;
//Tpf_Init_Uniform_Transitions                          pf_Init_Uniform_Transitions                          =  LRDelModel_Init_Uniform_Transitions;
//Tpf_Estimate_Transitions                              pf_Estimate_Transitions                              =  LRDelModel_Estimate_Transitions;
//Tpf_Log2_Transitions                                  pf_Log2_Transitions                                  =  LRDelModel_Log2_Transitions;
//Tpf_Alloc_Starts                                      pf_Alloc_Starts                                      =  LRDelModel_Alloc_Starts;
//Tpf_Compute_Starts_For_Two_Classes_Log2               pf_Compute_Starts_For_Two_Classes_Log2               =  LRDelModel_Compute_Starts_For_Two_Classes_Log2;
//Tpf_Compute_Starts_For_Distinct_Whiskers_Log2         pf_Compute_Starts_For_Distinct_Whiskers_Log2         =  LRDelModel_Compute_Starts_For_Distinct_Whiskers_Log2;
//Tpf_Alloc_Emissions                                   pf_Alloc_Emissions                                   =  LRDelModel_Alloc_Emissions;
//Tpf_Request_Static_Resizable_Emissions                pf_Request_Static_Resizable_Emissions                =  LRDelModel_Request_Static_Resizable_Emissions;
//Tpf_Compute_Emissions_For_Two_Classes_Log2            pf_Compute_Emissions_For_Two_Classes_Log2            =  LRDelModel_Compute_Emissions_For_Two_Classes_Log2;
//Tpf_Compute_Emissions_For_Distinct_Whiskers_Log2      pf_Compute_Emissions_For_Distinct_Whiskers_Log2      =  LRDelModel_Compute_Emissions_For_Distinct_Whiskers_Log2;
//Tpf_Compute_Emissions_For_Two_Classes_W_History_Log2  pf_Compute_Emissions_For_Two_Classes_W_History_Log2  =  LRDelModel_Compute_Emissions_For_Two_Classes_W_History_Log2;
//Tpf_State_Decode                                      pf_State_Decode                                      =  LRDelModel_State_Decode;
//#endif

//
// Frame index
//
typedef struct _frame_index
{ Measurements *first;
  int n;
} frame_index;

// Table should be sorted by time
frame_index *build_frame_index(Measurements *table, int nrows)
{ int nframes = table[nrows-1].fid+1;
  Measurements *row  = table + nrows,
               *last = row-1;
  frame_index *index = Guarded_Malloc( sizeof(frame_index)*nframes, "alloc frame index" );

  { int fid = 0;
    while(row-- > table )
    { if( row->fid != fid )
      { index[fid].first = row + 1;
        index[fid].n     = last - row;
        last = row;
        fid = row->fid;
      }
    }
    index[0].first = table;
    index[0].n     = last - table + 1;
  }

  return index;
}

void free_frame_index(frame_index *idx)
{ if(idx) free(idx);
}

//
// static range
// ...analagous to python's range()
//
int* _static_range( int n )
{ static size_t maxsize = 0;
  static int *buf = NULL;
  buf = request_storage( buf, &maxsize, sizeof(int), n, "_static_range" );
  while(n--)
    buf[n] = n;
  return buf;
}

//
// Measurements_Reference
//
Measurements_Reference *Measurements_Reference_Alloc(int nwhisk)
{ Measurements_Reference *this = Guarded_Malloc(sizeof(Measurements_Reference), "Measurements_Reference_Alloc");
  this->whiskers = Guarded_Malloc(sizeof(Measurements*)*nwhisk,"Measurements_Reference_Alloc");
  memset(this->whiskers,0,sizeof(Measurements*)*nwhisk);
  this->frame = NULL;
  this->nwhiskers = nwhisk;
  return this;
}

void Measurements_Reference_Free( Measurements_Reference* this )
{ if(this)
  { if( this->whiskers )
      free(this->whiskers);
    free(this);
  }
}

inline void Measurements_Reference_Reset( Measurements_Reference *this)
{ Measurements **w = this->whiskers;
  memset( w, 0, this->nwhiskers * sizeof(Measurements*) );
}

void Measurements_Reference_Build( Measurements_Reference *this, Measurements *row, int nseg )
{ Measurements **w = this->whiskers;
  this->frame  = row;
  this->nframe = nseg;
  while(nseg--)
    if(row[nseg].state >= 0)
      w[row[nseg].state] = row + nseg;
}

inline int Measurements_Reference_Has_Full_Count( Measurements_Reference *this )
{ int n = this->nwhiskers;
  Measurements **w = this->whiskers;
  while(n--)
    if( !w[n] )
      return 0;
  return 1;
}

//
// Measurements_Apply_Model
//  - returns 1 if nwhisk segments were labelled as whiskers
//            0 otherwise
//

int  Measurements_Apply_Model( frame_index *index, int fid, int nframes, int nwhisk, //input
                               real *S, real *T, real *E,                            //model parameters
                               real *likelihood )                                    //framewise likelihood (may be NULL)
{ int N = (*pf_State_Count)(nwhisk),
      nobs = index[fid].n,
      whisker_count = 0;

  ViterbiResult *result = Forward_Viterbi_Log2( _static_range(nobs), nobs, S, T, E, nobs, N );

  // Commit the result
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
  debug("[%5d/%5d]: total: %+5.5f prob: %+5.5f (delta: %+5.5f)\n",
      fid,                              // frame id
      nframes,                          // total frames
      result->total,                    // log2 prob(obs|model)           //?
      result->prob,                     // log2 prob(path|obs)            //?
      result->total - result->prob );                                     //?
  assert( nobs == result->n );
#endif
  if( likelihood )
    likelihood[fid] = result->prob - result->total;
#ifdef DEBUG_HMM_RECLASSIFY
  assert((result->prob - result->total)<=1e-7); //log2 likelikehoods should be less than zero
#endif
  { int i = nobs;
    int *seq = result->sequence;
    Measurements *bookmark = index[fid].first;
    while(i--)
    { int s = seq[i],
      lbl = (*pf_State_Decode)(s);
      bookmark[i].state = lbl;   // decode viterbi state to whisker label {-1:junk, 0..n:whiskers}
      if( lbl > -1 )
        whisker_count++;
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
      debug("Frame: %5d  Whisker: %3d  State: %3d Count: %3d Identity: %3d\n",
          bookmark[i].fid,
          bookmark[i].wid,
          s,
          whisker_count,
          bookmark[i].state);
#endif
    }
  } // end commit viterbi result
  Free_Viterbi_Result(result);  // FIXME: thrashing the heap
  return whisker_count == nwhisk;
} // end viterbi solution

void HMM_Reclassify_No_Deltas_W_Likelihood(
    Measurements *table, int nrows,        // observables
    Distributions *shp_dists,              // shape (static) distributions
    int nwhisk,                            // number of whiskers to expect
    real *S,                               // must be preallocated  - starts
    real *T,                               // must be precomputed   - transitions
    real *E,                               // must be preallocated  - emissions
    real *likelihood)                      // must be preallocated or NULL
{
  Measurements *row;

  row = table;
  while( row < table+nrows )
  { Measurements *bookmark = row;
    int fid = row->fid;
    int nobs;
    while( row->fid == fid && row < table+nrows )
    {
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
      debug("Frame: %5d  Whisker: %3d  State: %3d \n", row->fid, row->wid, row->state);
#endif
      ++row;
    }
    nobs = row - bookmark;

    (*pf_Compute_Starts_For_Two_Classes_Log2)( S, T, nwhisk, bookmark, shp_dists );

    E = (*pf_Request_Static_Resizable_Emissions)( nwhisk, nobs );
    (*pf_Compute_Emissions_For_Two_Classes_Log2)( E, nwhisk, bookmark, nobs, shp_dists );

    { int N = (*pf_State_Count)(nwhisk);
      ViterbiResult *result = Forward_Viterbi_Log2( _static_range(nobs), nobs, S, T, E, nobs, N );

      // Commit the result
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
      debug("[%5d/%5d]: total: %+5.5f prob: %+5.5f (delta: %+5.5f)\n",
          fid,                              // frame id
          table[nrows-1].fid+1,             // total frames
          result->total,                    // log2 prob(obs|model)           //?
          result->prob,                     // log2 prob(path|obs)            //?
          result->total - result->prob );                                     //?
      assert( nobs == result->n );
#endif
      if( likelihood )
        likelihood[fid] = result->prob - result->total;
      { int i = nobs;
        int *seq = result->sequence;
        while(i--)
        { int s = seq[i];
          bookmark[i].state = (*pf_State_Decode)(s);   // decode viterbi state to whisker label {-1:junk, 0..n:whiskers}
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
          debug("Frame: %5d  Whisker: %3d  State: %3d Identity: %3d\n",
              bookmark[i].fid,
              bookmark[i].wid,
              s,
              bookmark[i].state);
#endif
        }
      } // end commit viterbi result
      Free_Viterbi_Result(result);  // FIXME: thrashing the heap
    } // end viterbi solution
#ifdef DEBUG_HMM_RECLASSIFY
    assert(row!=bookmark);
#endif
  } // end loop over observations
}


#ifdef TEST_HMM_RECLASSIFY_NO_DELTAS
char *Spec[] = {"[-h|--help] | (<source:string> <dest:string> [-n <int>])",NULL};
int main(int argc, char*argv[])
{ int nrows;
  int nwhisk;
  Measurements *table;
  Distributions *shp_dists;
  real *T;

  Process_Arguments( argc, argv, Spec, 0 );

  help( Is_Arg_Matched("-h") || Is_Arg_Matched("--help"),
    "--------------\n"
    "HMM-Reclassify\n"
    "--------------\n"
    " <source> should be the filename of a `measurements` file where an initial guess has been made as\n"
    "          to the identity of many of the whiskers.  These initial assignments are used to build a \n"
    "          probabalistic model of the process by which whiskers are identified as one travels along\n"
    "          the face\n"
    "\n"
    " <dest>   should be the destination filename.  After applying the probibalistic model to identify\n"
    "          whiskers in each frame, the results are saved to this file.\n"
    "\n"
    " -n <int> Optionally specify the number of whiskers to identify.  The default behavior is to use\n"
    "          the initial guess provided by <source>.  Specifying a number less than one results in\n"
    "          the default behavior.\n"
    "\n");
  // Load Table
  table = Measurements_Table_From_Filename( Get_String_Arg("source"), &nrows );
  Sort_Measurements_Table_State_Time(table, nrows);

  // Get expected number of whiskers
  nwhisk = -1;
  if( Is_Arg_Matched("-n") )
    nwhisk = Get_Int_Arg("-n");
  if( nwhisk < 1 )
  { int nstate,minstate,maxstate;
    nstate = _count_n_states(table,nrows,1,&minstate,&maxstate);
    nwhisk = nstate - 1; //subtract the dummy state, which is minstate
#ifdef DEBUG_HMM_RECLASSIFY
    assert(minstate<maxstate);
    assert(minstate==-1);
    assert(maxstate>=0);
    assert(nstate>0);
#endif
  }

  //
  // Compute transitions matrix for model
  //   -  assumes there's a prior guess for the identities

  T = (*pf_Alloc_Transitions)(nwhisk);
  //(*pf_Init_Uniform_Transitions)(T,nwhisk);
  (*pf_Estimate_Transitions)( T, nwhisk, table, nrows );

#ifdef DEBUG_HMM_RECLASSIFY
  // Assert properly normed
  { real sum = 1.0;
    double *row;
    int N = nwhisk*2 + 1;
    for( row=T; row < T + N*N; row++ )
    {
      if( ! (row-T)%N )
      { assert( fabs(sum-1.0) < 1e-3 );
        sum = 0.0;
      }
      sum += *row;
    }
  }
#endif

  (*pf_Log2_Transitions)(T,nwhisk, HMM_RECLASSIFY_BASELINE_LOG2);

#ifdef DEBUG_HMM_RECLASSIFY
  // Assert properly normed
  { real sum = 1.0;
    double *row;
    int N = nwhisk*2 + 1;
    for( row=T; row < T + N*N; row++ )
    {
      if( ! (row-T)%N )
      { assert( fabs(sum-1.0) < 1e-3 );
        sum = 0.0;
      }
      sum += *row;
    }
  }
#endif

  //
  // Want to condition distributions over whether it's a whisker or not,
  // so encode input identities.
  //   - this block must be after calculation of transitions
  //   - erases the loaded (guess) identities
  { int i = nrows;
    while(i--)
    { Measurements *row = table + i;
      row->state = (int) row->state!=-1;
    }
  }

#ifdef DEBUG_HMM_RECLASSIFY
  { int i = nrows;
    while(i--)
      assert( table[i].state == 0 || table[i].state == 1 );
  }
#endif

  //
  // Now build distributions.
  //

  Sort_Measurements_Table_State_Time(table, nrows);
  shp_dists = Build_Distributions( table, nrows, HMM_RECLASSIFY_SHP_DISTS_NBINS );
  Distributions_Dilate( shp_dists );
  Distributions_Normalize( shp_dists );
  Distributions_Apply_Log2( shp_dists );

  //
  // Process frames
  //
  Sort_Measurements_Table_Time_Face( table, nrows );
  { real *E = (*pf_Request_Static_Resizable_Emissions)(
                  nwhisk,
                  (nrows/table[nrows-1].fid)*2); // initial alloc for twice the average sequence length
    real *S = (*pf_Alloc_Starts)( nwhisk );

    //
    // Reclassify all frames
    //
    HMM_Reclassify_No_Deltas_W_Likelihood(
        table, nrows,
        shp_dists,
        nwhisk,
        S,T,E,
        NULL /*likelihood*/);

    free(S);
    free(E);
  } // end re-classification

  //
  // Save results
  //

  Measurements_Table_To_Filename( Get_String_Arg("dest"), table, nrows );

  //
  // Cleanup
  //
  free(T);
  Free_Distributions(shp_dists);
  Free_Measurements_Table(table);

  return 0;
}
#endif

#ifdef TEST_HMM_RECLASSIFY_W_DELTAS
char *Spec[] = {"[-h|--help] | ( [-n <int>] <source:string> <dest:string> )",NULL};
int main(int argc, char*argv[])
{
  int nrows;
  int nwhisk;
  Measurements *table, *row;
  Distributions *shp_dists, *vel_dists;
  real *T;

  Process_Arguments( argc, argv, Spec, 0 );

  help( Is_Arg_Matched("-h") || Is_Arg_Matched("--help"),
    "----------------------------\n"
    "HMM-Reclassify (with deltas)\n"
    "----------------------------\n"
    " <source> should be the filename of a `measurements` file where an initial guess has been made as\n"
    "          to the identity of many of the whiskers.  These initial assignments are used to build a \n"
    "          probabalistic model of the process by which whiskers are identified as one travels along\n"
    "          the face\n"
    "\n"
    " <dest>   should be the destination filename.  After applying the probibalistic model to identify\n"
    "          whiskers in each frame, the results are saved to this file.\n"
    "\n"
    " -n <int> Optionally specify the number of whiskers to identify.  The default behavior is to use\n"
    "          the initial guess provided by <source>.  Specifying a number less than one results in\n"
    "          the default behavior.\n"
    "\n");

  table = Measurements_Table_From_Filename( Get_String_Arg("source"), &nrows );

  //
  // Compute velocities using approximate/incomplete labelling
  //
  Sort_Measurements_Table_State_Time(table, nrows);
  Measurements_Table_Compute_Velocities(table,nrows);

  nwhisk = -1;
  if( Is_Arg_Matched("-n") )
    nwhisk = Get_Int_Arg("-n");
  if( nwhisk < 1 )
  { int nstate,minstate,maxstate;
    nstate = _count_n_states(table,nrows,1,&minstate,&maxstate);
    nwhisk = nstate - 1; //subtract the dummy state, which is minstate
#ifdef DEBUG_HMM_RECLASSIFY
    assert(minstate<maxstate);
    assert(minstate==-1);
    assert(maxstate>=0);
    assert(nstate>0);
#endif
  }

#ifdef DEBUG_HMM_RECLASSIFY
	debug("nwhisk: %d\n",nwhisk);
#endif
  //
  // Compute transitions matrix for model
  //

  T = (*pf_Alloc_Transitions)(nwhisk);
  //(*pf_Init_Uniform_Transitions)(T,nwhisk);
  (*pf_Estimate_Transitions)( T, nwhisk, table, nrows );
  (*pf_Log2_Transitions)(T,nwhisk, HMM_RECLASSIFY_BASELINE_LOG2);

  //
  // Want to condition distributions over whether it's a whisker or not,
  // so encode input labels
  //
  row = table + nrows;
  while(row-- > table)
    row->state = (int) row->state!=-1;

  //
  // Now build distributions. (Conditioned on whisker or not)
  //

  Sort_Measurements_Table_State_Time(table, nrows);
  shp_dists = Build_Distributions( table, nrows, HMM_RECLASSIFY_SHP_DISTS_NBINS );
  vel_dists = Build_Velocity_Distributions( table, nrows, HMM_RECLASSIFY_VEL_DISTS_NBINS );
  Distributions_Dilate( shp_dists );
  Distributions_Dilate( vel_dists );
  Distributions_Normalize( shp_dists );
  Distributions_Normalize( vel_dists );
  Distributions_Apply_Log2( shp_dists );
  Distributions_Apply_Log2( vel_dists );

  //
  // Process frames
  //
  Sort_Measurements_Table_Time_Face( table, nrows );
  { real *E = (*pf_Request_Static_Resizable_Emissions)(
                  nwhisk,
                  (nrows/table[nrows-1].fid)*2); // initial alloc for twice the average sequence length
    real *S = (*pf_Alloc_Starts)( nwhisk );
    Measurements *row;
    Measurements_Reference *last = Measurements_Reference_Alloc(nwhisk);

    row = table;
    while( row < table+nrows )
    { Measurements *bookmark = row;
      int fid = row->fid;
      int nobs;
      while(row < table+nrows && row->fid == fid  )
      {
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
          debug("Frame: %5d  Whisker: %3d  State: %3d \n", row->fid, row->wid, row->state);
#endif
        ++row;
      }
      nobs = row - bookmark;

      (*pf_Compute_Starts_For_Two_Classes_Log2)( S, T, nwhisk, bookmark, shp_dists );

      E = (*pf_Request_Static_Resizable_Emissions)( nwhisk, nobs );
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
      { int i;
        debug("Last\n");
        for(i=0; i<nwhisk; i++)
          debug("\t%p", last->whiskers[i]);
        debug("\n");
      }
#endif
      if(!last->frame)
        (*pf_Compute_Emissions_For_Two_Classes_Log2)( E, nwhisk, bookmark, nobs, shp_dists );
      else
        (*pf_Compute_Emissions_For_Two_Classes_W_History_Log2)( E,
                                                                nwhisk,
                                                                bookmark, nobs,
                                                                last,
                                                                shp_dists,
                                                                vel_dists );

      { int N = (*pf_State_Count)(nwhisk);
        ViterbiResult *result = Forward_Viterbi_Log2( _static_range(nobs), nobs, S, T, E, nobs, N );

        // Commit the result
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
        debug("[%5d/%5d]: total: %+5.5f prob: %+5.5f (delta: %+5.5f)\n",
            fid,                              // frame id
            table[nrows-1].fid+1,             // total frames
            result->total,                    // log2 prob(obs|model)           //?
            result->prob,                     // log2 prob(path|obs)            //?
            result->total - result->prob );                                     //?
        assert( nobs == result->n );
#endif
        { int i = nobs;
          int *seq = result->sequence;
          memset(last->whiskers,0, sizeof(Measurements*)*nwhisk );
          while(i--)
          { int s = seq[i],
                lbl = (*pf_State_Decode)(s);
            bookmark[i].state = lbl;   // decode viterbi state to whisker label {-1:junk, 0..n:whiskers}
            if(lbl>-1)
              last->whiskers[lbl] = bookmark+i;
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
            debug("Frame: %5d  Whisker: %3d  State: %3d Identity: %3d\n",
                bookmark[i].fid,
                bookmark[i].wid,
                s,
                bookmark[i].state);
#endif
          }
        } // end commit viterbi result
        Free_Viterbi_Result(result);  // FIXME: thrashing the heap
      } // end viterbi solution
      last->frame  = bookmark;
      last->nframe = nobs;
#ifdef DEBUG_HMM_RECLASSIFY
      assert(row!=bookmark);
#endif
    } // end loop over observations
    Measurements_Reference_Free(last);
    free(S);
  } // end re-classification

  //
  // Save results
  //

  Measurements_Table_To_Filename( Get_String_Arg("dest"), table, nrows );

  //
  // Cleanup
  //
  //free(T);
  Free_Distributions(vel_dists);
  Free_Distributions(shp_dists);
  Free_Measurements_Table(table);

  return 0;
}
#endif

#ifdef TEST_HMM_RECLASSIFY_WATERSHED


typedef struct _heap
{ real **data;
  size_t size;
  size_t maxsize;
} heap;

#define heap_left(i)   ((i)<<1)
#define heap_right(i)  (((i)<<1)+1)
#define heap_parent(i) ((i)/2)

heap *heap_alloc( size_t count )
{ heap *h = Guarded_Malloc( sizeof(heap), "alloc priority queue" );
  h->data = (real**) Guarded_Malloc( sizeof(real*)*count, "alloc priority queue for hmm-watershed" );
  //memset(h->data,0,sizeof(real*)*2*count); // shouldn't be neccessary
  h->size    = 0;
  h->maxsize = count;
  return h;
}

void heap_free( heap *h )
{ if(h)
  { if( h->data ) free(h->data);
    free(h);
  }
}

inline void heapify( heap* h, int i )
{ int left  = heap_left(i),
      right = heap_right(i),
      argmax = i;
  real **data  = h->data-1, // head is at index 1...that is, want data[1] <==> h->data[0]
       max = *data[i];
#ifdef DEBUG_HEAPIFY
  assert(i>=1);
#endif
  if(  ( left < h->size )&&( *data[left] > max ) )
  { argmax = left;
    max = *data[left];
  }
  if(  ( right < h->size )&&( *data[right] > max ) )
    argmax = right;

  if( argmax != i )
  { real *a = data[argmax];     // swap contents
    data[argmax] = data[i];
    data[i] = a;
    heapify( h, argmax );       // call on subtree
  }
}

// Organizes an unorganized heap in O(h->size)
inline void heap_build( heap *h )
{ int i = h->size/2;
  while(i-- > 1)
    heapify(h,i);
}

inline real *heap_pop_head( heap *h )
{ real *max,
      **data = h->data - 1;
  if( h->size < 1 )
    error("heap underflow\n");
  max = data[1];
  data[1] = data[h->size--];
  heapify(h,1);
  return max;
}

inline real *heap_pop_and_insert( heap *h, real *v )
{ real *max,
      **data = h->data - 1;
  if( h->size < 1 )
    error("heap underflow\n");
  max = data[1];
  data[1] = v;
  heapify(h,1);
  return max;
}

inline void heap_insert( heap *h, real *v )
{ real **data = h->data - 1;
  size_t i,parent;
  if( h->size+1 > h->maxsize )
    error("Heap overflow.\n"
          "\tRequested : %d\n"
          "\tAvailable : %d\n", h->size, h->maxsize);
  i = ++(h->size);
  data[i] = v;
  while( i>1 && *data[parent = heap_parent(i)] < *data[i] )
  { real* a = data[i];
    data[i] = data[parent];
    data[parent] = a;
    i = parent;
  }

}

heap *Priority_Queue_Init( real *likelihood, int nframes)
{ // 1. count local maxima
  //    since we want to start with high-likelihood this "watershed" will be
  //    high to low.
  int count = 0;
  real *p;          // cursor into the likelihood array
  heap *q;          // priority queue
  static const real tol = 0.0;

  p = likelihood + nframes;
  while(p-- > likelihood+1)
    if(    (p[ 0] - p[-1]) >= tol
        && (p[ 0] - p[ 1]) >= tol )
      count++;
  count += 2; //will also add first and last

  // 2. alloc queue - a binary tree
  //    children of i are at i<<1,i<<1+1
  q = heap_alloc( count );

  // 3. place local minima  (tolerance suppresses a few)
  //    a.  throw them, unordered, onto the heap
  //    b.  use heap_build, O(count)
  //    Repeated heap_insert would be O( count x log(count) )
  { real **minima = q->data;
    p = likelihood + nframes;
    while(p-- > likelihood+1)
      if(    (p[ 0] - p[-1]) >= tol
          && (p[ 0] - p[ 1]) >= tol )
        *(minima++) = p;
    *(minima++) = likelihood;               //add first
    *(minima++) = likelihood + nframes - 1; //add last
    q->size = minima - q->data;
    heap_build(q);
  }

  return q;
}

int HMM_Reclassify_Frame_W_Neighbors(      // returns 1 if likelihood changed, otherwise 0
    frame_index *index, int nframes,       // frame index
    Distributions *shp_dists,              // shape (static) distributions
    Distributions *vel_dists,              // shape (static) distributions
    int nwhisk,                            // number of whiskers to expect
    real *S,                               // must be preallocated  - starts
    real *T,                               // must be precomputed   - transitions
    real *E,                               // must be preallocated  - emissions
    real **visited,
    real *likelihood,                      //
    int fid,                               // frame to reclassify
    int propigate)                         // Recursively propigate to neighbors?
{ int prev  = fid - 1,
      next  = fid + 1,
      ref,
      nobs = index[fid].n;
  static const real tol = 1e-3;
  static Measurements_Reference *hist  = NULL;
  static Measurements_Reference *hist2 = NULL;

  if(!hist)
  { hist  = Measurements_Reference_Alloc(nwhisk);
    hist2 = Measurements_Reference_Alloc(nwhisk);
  }
  Measurements_Reference_Reset(hist);
  Measurements_Reference_Reset(hist2);


  // Mark out unreliable neighbors
  if(    prev < 0        || !visited[prev] )  prev = -1;
  if(    next >= nframes || !visited[next] )  next = -1;
  if( (prev==-1) && (next==-1) ) //no good neighbors for update
    return 0;

  (*pf_Compute_Starts_For_Two_Classes_Log2)( S, T, nwhisk, index[fid].first, shp_dists );
  E = (*pf_Request_Static_Resizable_Emissions)( nwhisk, nobs );

  if( prev!=-1 && next !=-1 ) // two neighbors, so update and that's it
  { Measurements_Reference_Build( hist, index[prev].first, index[prev].n );
    Measurements_Reference_Build( hist2, index[next].first, index[next].n );
    (*pf_Compute_Emissions_For_Two_Classes_W_Prev_And_Next_Log2)( E,
        nwhisk,
        index[fid].first, nobs,
        hist, hist2,
        shp_dists, vel_dists );
    return Measurements_Apply_Model( index, fid, nframes, nwhisk,
                                     S,T,E,
                                     NULL /*likelihood*/ );
  }

  if( prev == -1 )
    ref = next;
  else
    ref = prev;
  { Measurements_Reference_Build( hist, index[ref].first, index[ref].n );
    (*pf_Compute_Emissions_For_Two_Classes_W_History_Log2)( E,
        nwhisk,
        index[fid].first, nobs,
        hist,
        shp_dists,
        vel_dists );
  }

  if( !Measurements_Apply_Model( index, fid, nframes, nwhisk, S,T,E,NULL ) )
  { visited[fid] = NULL;
    return 0;
  }

  if(propigate)
  { prev = fid-1;
    next = fid+1;
    if(likelihood)
    { if(    prev < 0
          || visited[prev]
          || likelihood[prev] - likelihood[fid] > tol)
        prev = -1;
      if(    next >= nframes
          || visited[next]
          || likelihood[next] - likelihood[fid] > tol)
        next = -1;
    } else //no likelihood provided so ignore
    { if(    prev < 0        || visited[prev] )
        prev = -1;
      if(    next >= nframes || visited[next] )
        next = -1;
    }

    if( prev > -1 )
    { visited[prev] = visited[fid];
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
      debug("\tPropigate <<< to %5d\n",prev);
#endif
      HMM_Reclassify_Frame_W_Neighbors( index, nframes,
                                        shp_dists, vel_dists,
                                        nwhisk,
                                        S,T,E,
                                        visited,
                                        likelihood,
                                        prev,
                                        1 /*propigate*/ );
    }
    if( next > -1 )
    { visited[next] = visited[fid];
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
      debug("\tPropigate >>> to %5d\n",next);
#endif
      HMM_Reclassify_Frame_W_Neighbors( index, nframes,
                                        shp_dists, vel_dists,
                                        nwhisk,
                                        S,T,E,
                                        visited,
                                        likelihood,
                                        next,
                                        1 /*propigate*/ );
    }
    Measurements_Reference_Reset(hist);
    Measurements_Reference_Reset(hist2);
    if( prev > -1 )
      Measurements_Reference_Build( hist, index[prev].first, index[prev].n );
    if( next > -1 )
      Measurements_Reference_Build( hist2, index[next].first, index[next].n );

    { int ok_prev = Measurements_Reference_Has_Full_Count(hist),
          ok_next = Measurements_Reference_Has_Full_Count(hist2);
      if( ok_next || ok_prev )  // ,then recompute emissions and reapply
      { if( ok_next && ok_prev )
        { (*pf_Compute_Emissions_For_Two_Classes_W_Prev_And_Next_Log2)( E,
              nwhisk,
              index[fid].first, nobs,
              hist, hist2,
              shp_dists, vel_dists );
        } else if( ok_prev )
        { (*pf_Compute_Emissions_For_Two_Classes_W_History_Log2)( E,
              nwhisk,
              index[fid].first, nobs,
              hist,
              shp_dists, vel_dists );
        } else if( ok_next )
        { (*pf_Compute_Emissions_For_Two_Classes_W_History_Log2)( E,
              nwhisk,
              index[fid].first, nobs,
              hist2,
              shp_dists, vel_dists );
        }
        return Measurements_Apply_Model( index, fid, nframes, nwhisk,
                                  S,T,E,
                                  NULL /*likelihood*/ );
      }
    }
  }
  return 1;
}

int HMM_Reclassify_Jump_Gap(               // returns frame id to jump to
    frame_index *index, int nframes,       // frame index
    Distributions *shp_dists,              // shape (static) distributions
    Distributions *vel_dists,              // shape (static) distributions
    int nwhisk,                            // number of whiskers to expect
    real *S,                               // must be preallocated  - starts
    real *T,                               // must be precomputed   - transitions
    real *E,                               // must be preallocated  - emissions
    real **visited,
    real *likelihood,                      //
    Measurements_Reference *ref,           // Reference labelling to use for deltas
    int fid,                               // frame to start from/query
    int propigate)                         // Direction to search  (-1 or +1)
{ assert(propigate != 0);
  while( fid>=0 && fid<nframes && !visited[fid] )
  { (*pf_Compute_Emissions_For_Two_Classes_W_History_Log2)( E,
      nwhisk,
      index[fid].first, index[fid].n,
      ref,
      shp_dists, vel_dists );
    if( Measurements_Apply_Model( index, fid, nframes, nwhisk, S,T,E, likelihood ) )
      return fid;
    // else didn't get the right number of whiskers...move on to the next frame
    fid += propigate;
  }
  return fid;
}

int HMM_Reclassify_Fast_Forward(           // returns frame id to jump to
    frame_index *index, int nframes,       // frame index
    Distributions *shp_dists,              // shape (static) distributions
    Distributions *vel_dists,              // shape (static) distributions
    int nwhisk,                            // number of whiskers to expect
    real *S,                               // must be preallocated  - starts
    real *T,                               // must be precomputed   - transitions
    real *E,                               // must be preallocated  - emissions
    real **visited,
    real *likelihood,                      //
    Measurements_Reference *ref,           // Reference labelling to use for deltas
    int fid)                               // frame to start from/query
{ real *mark = visited[fid];
  assert(fid!=0); // need some left frame with known
  while( fid>0 && fid<nframes && !visited[fid] )
  { Measurements_Reference_Build( ref, index[fid-1].first, index[fid-1].n );
    (*pf_Compute_Emissions_For_Two_Classes_W_History_Log2)( E,
      nwhisk,
      index[fid].first, index[fid].n,
      ref,
      shp_dists, vel_dists );
    Measurements_Apply_Model( index, fid, nframes, nwhisk, S,T,E, likelihood );
    visited[fid] = mark;
    fid += 1;
  }
  return fid;
}

int HMM_Reclassify_Fill_Gap(
    frame_index *index, int nframes,       // frame index
    Distributions *shp_dists,              // shape (static) distributions
    Distributions *vel_dists,              // shape (static) distributions
    int nwhisk,                            // number of whiskers to expect
    real *S,                               // must be preallocated  - starts
    real *T,                               // must be precomputed   - transitions
    real *E,                               // must be preallocated  - emissions
    real **visited,
    real *likelihood,                      //
    int fid_left)                          // frame to start from (left edge of gap)
{ static Measurements_Reference *left  = NULL;
  static Measurements_Reference *right = NULL;
  int fid_right;

  if(!left)
  { left  = Measurements_Reference_Alloc(nwhisk);
    right = Measurements_Reference_Alloc(nwhisk);
  }
  Measurements_Reference_Reset(left);
  Measurements_Reference_Reset(right);

  // find other edge of gap
  fid_right = fid_left;
  while( fid_right < nframes && !visited[fid_right] ) fid_right++;

  if( fid_right == nframes ) // no other edge...fast forward to end and return
  { if( fid_left != 0 )
    { visited[fid_left] = visited[fid_left-1];
      Measurements_Reference_Build( left, index[fid_left-1].first, index[fid_left-1].n );
      HMM_Reclassify_Fast_Forward( index, nframes, shp_dists, vel_dists, nwhisk, S,T,E, visited, NULL, left, fid_left );
    }
    return fid_right;
  }

  // check likelihood of extension from left or right
  // extend using lowest likelihood side
  // update and repeat

  // Init
  fid_right--; //back up one, so left and right are just inside gap
  Measurements_Reference_Build( left, index[fid_left-1].first, index[fid_left-1].n );
  Measurements_Reference_Build( right, index[fid_right+1].first, index[fid_right+1].n );
  //left
  (*pf_Compute_Emissions_For_Two_Classes_W_History_Log2)( E,nwhisk,
              index[fid_left].first, index[fid_left].n, left,
              shp_dists, vel_dists );
  Measurements_Apply_Model( index, fid_left, nframes, nwhisk, S,T,E, likelihood );
  //right
  (*pf_Compute_Emissions_For_Two_Classes_W_History_Log2)( E,nwhisk,
              index[fid_right].first, index[fid_right].n, right,
              shp_dists, vel_dists );
  Measurements_Apply_Model( index, fid_right, nframes, nwhisk, S,T,E, likelihood );

  // Reduce gap
  while( fid_right - fid_left >= 0)
  { if( likelihood[ fid_left ] < likelihood[ fid_right ] ) // then right wins
    { visited[fid_right] = visited[fid_right+1];
      fid_right--;
      Measurements_Reference_Build( right, index[fid_right+1].first, index[fid_right+1].n );
      (*pf_Compute_Emissions_For_Two_Classes_W_History_Log2)( E,nwhisk,
                  index[fid_right].first, index[fid_right].n, right,
                  shp_dists, vel_dists );
      Measurements_Apply_Model( index, fid_right, nframes, nwhisk, S,T,E, likelihood );
    } else                                                 // the left wins
    { visited[fid_left] = visited[fid_left-1];
      fid_left++;
      Measurements_Reference_Build( left, index[fid_left-1].first, index[fid_left-1].n );
      (*pf_Compute_Emissions_For_Two_Classes_W_History_Log2)( E,nwhisk,
                  index[fid_left].first, index[fid_left].n, left,
                  shp_dists, vel_dists );
      Measurements_Apply_Model( index, fid_left, nframes, nwhisk, S,T,E, likelihood );
    }
  }

  // now left with a single frame gap XXX

  return fid_right;
}

int _count_labelled_as_whiskers(frame_index *index, int fid)
{ int count = 0,     // Pass if this minima didn't label all whiskers
      i,
      n = index[fid].n,
  minstate = -1;
  Measurements *row = index[fid].first;
  for(i=0; i<n; i++ )
    if( row[i].state > minstate )
      count ++;
  return count;
}

char *Spec[] = {"[-h|--help] | ( [-n <int>] <source:string> <dest:string> )",NULL};
int main(int argc, char*argv[])
{
  int nrows;
  int nwhisk;
  Measurements *table, *row;
  Distributions *shp_dists, *vel_dists;
  real *T;

  Process_Arguments( argc, argv, Spec, 0 );

  help( Is_Arg_Matched("-h") || Is_Arg_Matched("--help"),
    "----------------------------\n"
    "HMM-Reclassify ( Watershed )\n"
    "----------------------------\n"
    " <source> should be the filename of a `measurements` file where an initial guess has been made as\n"
    "          to the identity of many of the whiskers.  These initial assignments are used to build a \n"
    "          probabalistic model of the process by which whiskers are identified as one travels along\n"
    "          the face\n"
    "\n"
    " <dest>   should be the destination filename.  After applying the probibalistic model to identify\n"
    "          whiskers in each frame, the results are saved to this file.\n"
    "\n"
    " -n <int> Optionally specify the number of whiskers to identify.  The default behavior is to use\n"
    "          the initial guess provided by <source>.  Specifying a number less than one results in\n"
    "          the default behavior.\n"
    "\n");

  table = Measurements_Table_From_Filename( Get_String_Arg("source"), &nrows );

  //
  // Compute velocities using approximate/incomplete labelling
  //
  Sort_Measurements_Table_State_Time(table, nrows);
  Measurements_Table_Compute_Velocities(table,nrows);

  nwhisk = -1;
  if( Is_Arg_Matched("-n") )
    nwhisk = Get_Int_Arg("-n");
  if( nwhisk < 1 )
  { int nstate,minstate,maxstate;
    nstate = _count_n_states(table,nrows,1,&minstate,&maxstate);
    nwhisk = nstate - 1; //subtract the dummy state, which is minstate
#ifdef DEBUG_HMM_RECLASSIFY
    assert(minstate<maxstate);
    assert(minstate==-1);
    assert(maxstate>=0);
    assert(nstate>0);
#endif
  }

#ifdef DEBUG_HMM_RECLASSIFY
	debug("nwhisk: %d\n",nwhisk);
#endif
  //
  // Compute transitions matrix for model
  //

  T = (*pf_Alloc_Transitions)(nwhisk);
  //(*pf_Init_Uniform_Transitions)(T,nwhisk);
  (*pf_Estimate_Transitions)( T, nwhisk, table, nrows );
  (*pf_Log2_Transitions)(T,nwhisk, HMM_RECLASSIFY_BASELINE_LOG2);

  //
  // Want to condition distributions over whether it's a whisker or not,
  // so encode input labels
  //
  row = table + nrows;
  while(row-- > table)
    row->state = (int) row->state!=-1;

  //
  // Now build distributions. (Conditioned on whisker or not)
  //

  Sort_Measurements_Table_State_Time(table, nrows);
  shp_dists = Build_Distributions( table, nrows, HMM_RECLASSIFY_SHP_DISTS_NBINS );
  vel_dists = Build_Velocity_Distributions( table, nrows, HMM_RECLASSIFY_VEL_DISTS_NBINS );
  Distributions_Dilate( shp_dists );
  Distributions_Dilate( vel_dists );
  Distributions_Normalize( shp_dists );
  Distributions_Normalize( vel_dists );
  Distributions_Apply_Log2( shp_dists );
  Distributions_Apply_Log2( vel_dists );

  //
  // Process frames
  //
  Sort_Measurements_Table_Time_Face( table, nrows );
  { real *E = (*pf_Request_Static_Resizable_Emissions)(
                  nwhisk,
                  (nrows/table[nrows-1].fid)*2); // initial alloc for twice the average sequence length
    real *S = (*pf_Alloc_Starts)( nwhisk );
    int nframes = table[nrows-1].fid+1;
    real **visited    = Guarded_Malloc( sizeof(real*)*nframes, "alloc visited"    );
    real *likelihood = Guarded_Malloc( sizeof(real)*nframes, "alloc likelihood" );
    frame_index *index = build_frame_index(table,nrows);
    heap *q;
#ifdef DEBUG_HMM_RECLASSIFY
    FILE *fp = fopen("visited.raw","w+b");
#endif

    //  Initialize
    //   - Get initial likelihoods
    //   - Build priority queue
    //
    memset( visited, 0, sizeof(real*)*nframes );
    HMM_Reclassify_No_Deltas_W_Likelihood( table, nrows, shp_dists, nwhisk, S,T,E, likelihood );
#ifdef DEBUG_HMM_RECLASSIFY
    { FILE *fp_likelihood = fopen("likelihood.raw","w+b");
      fwrite( likelihood, sizeof(real), nframes, fp_likelihood );
      fclose(fp_likelihood);
    }
#endif
    q = Priority_Queue_Init( likelihood, nframes );
    //
    // Process frames according to priority queue
    //
    { static const real min_likelihood = -1e-6;
      if( *q->data[0] < min_likelihood )
        warning("Best labeling has likelihood lower that threshold.\n"
                "\tThis indicates the model doesn't work well for this data.\n"
                "\tPerhaps you specified the wrong  number of whiskers to look?\n"
                "\n"
                "\t\tBest likelihood: %f\n"
                "\t\t      Threshold: %f\n", *q->data[0], min_likelihood);
      while( q->size > 0 )
      { int fid = q->data[0] - likelihood,
        prev  = fid - 1,
        next  = fid + 1;
        real depth = *(q->data[0]);

        if( visited[fid] )   // Pass if already visited
        { heap_pop_head(q);
          continue;
        }

        // Pass if this minima didn't label all whiskers
        if( _count_labelled_as_whiskers(index,fid) != nwhisk )
        { heap_pop_head(q);
          continue;
        }

        // Don't worry about extending from unlikely minima
        if( likelihood[fid] < min_likelihood )
          break;

        visited[fid] = heap_pop_head(q);

#ifdef DEBUG_HMM_RECLASSIFY
        debug("Frame: %5d Q size: %5d/%-5d likelihood: %g\n", fid, q->size, q->maxsize, likelihood[fid]);
        fwrite( visited, sizeof(real*), nframes, fp );
#endif

        // fill local interval
        if(   prev >= 0
            && !visited[prev] )
        { visited[prev] = visited[fid];
          HMM_Reclassify_Frame_W_Neighbors(
              index, nframes,
              shp_dists, vel_dists,
              nwhisk,
              S,T,E,
              visited,
              NULL, //likelihood,
              prev,
              1 /*propigate*/ );
        }
        if(   next < nframes
            && !visited[next] )
        { visited[next] = visited[fid];
          HMM_Reclassify_Frame_W_Neighbors(
              index, nframes,
              shp_dists, vel_dists,
              nwhisk,
              S,T,E,
              visited,
              NULL, //likelihood,
              next,
              1 /*propigate*/ );
        }
      }
    }

    //
    // Merge
    // Motivation
    //  - extend trusted intervals over untrusted one
    //  - beware propigating past big mistakes.

#ifdef DEBUG_HMM_RECLASSIFY
    // unlabel segments on all unvisited intervals
    { int i;
      for(i=0; i< nframes; i++)
      { if( !visited[i] )
        { int j,n = index[i].n;
          Measurements *row = index[i].first;
          for(j=0;j<n;j++)
            row[j].state = -1;
        }
      }
    }
#endif

    //
    // Try to jump gaps and refill
    //

    { Measurements_Reference *left  = Measurements_Reference_Alloc(nwhisk),
                             *right = Measurements_Reference_Alloc(nwhisk);
      int fid=0,
          left_ok = 0,  left_jump,
          right_ok = 0, right_jump,
          temp;
      do
      { fid=0;
        while( fid < nframes )
        { left_ok = right_ok = 0;
          while( visited[fid] && fid<nframes ) fid++; // find the left side of the next gap
          if( fid == nframes )
          { break;  // no gaps...so all done
          } else if( left_ok = (fid != 0) )
          { Measurements_Reference_Build( left, index[fid-1].first, index[fid-1].n );
            left_jump = HMM_Reclassify_Jump_Gap( index, nframes, shp_dists, vel_dists, nwhisk, S,T,E, visited, likelihood,
                                                left, fid, 1 );
            left_ok &= left_jump < nframes;
          }
          while( !visited[fid] && fid<nframes ) fid++; // find the right side of this gap
          if( right_ok = (fid != nframes) )
          { Measurements_Reference_Build( right, index[fid].first, index[fid].n );
            right_jump = HMM_Reclassify_Jump_Gap( index, nframes, shp_dists, vel_dists, nwhisk, S,T,E, visited, likelihood,
                                                  right, fid-1, -1 );
            right_ok &= right_jump >= 0;
          }
          /* if gap-jump hit the other side (that is, it didn't find anything)
           * mark as not ok */
          temp = left_ok;
          //left_ok   &= ( !right_ok || right->frame->fid != left_jump );
          //right_ok  &= ( !temp     || left->frame->fid  != right_jump );
          left_ok  = left_ok  && ( !right_ok || right->frame->fid != left_jump );  
          right_ok = right_ok && ( !temp     || left->frame->fid  != right_jump ); 
          /* choose the one with the smaller
          * jump, on tie choose left */
          left_ok  = left_ok && ( !right_ok || (left_jump-(left->frame->fid) <= (right->frame->fid)-right_jump ) );
          right_ok &= !left_ok; // ensure only left or right (or none) is chosen

          if( left_ok )
          { real *mark = visited[left->frame->fid];
            visited[ left_jump ] = visited[ left_jump+1 ] = visited[left_jump-1] = mark;
            HMM_Reclassify_Frame_W_Neighbors(index, nframes, shp_dists, vel_dists, nwhisk, S,T,E, visited, NULL,
                                            left_jump+1,1);
            HMM_Reclassify_Frame_W_Neighbors(index, nframes, shp_dists, vel_dists, nwhisk, S,T,E, visited, NULL,
                                            left_jump-1,1);
          }
          if( right_ok )
          { real *mark = visited[right->frame->fid];
            visited[ right_jump ] = visited[ right_jump+1 ] = visited[right_jump-1] = mark;
            HMM_Reclassify_Frame_W_Neighbors(index, nframes, shp_dists, vel_dists, nwhisk, S,T,E, visited, NULL,
                                            right_jump-1,1);
            HMM_Reclassify_Frame_W_Neighbors(index, nframes, shp_dists, vel_dists, nwhisk, S,T,E, visited, NULL,
                                            right_jump+1,1);
          }
#ifdef DEBUG_HMM_RECLASSIFY
          debug("Gap jump: left: (%2d) %5d jump to %5d   delta: %5d\n"
                "         right: (%2d) %5d jump to %5d   delta: %5d\n"
                "           fid:      %5d \n",
                left_ok,  (left->frame)? left->frame->fid : -1, left_jump, ( left->frame)? left_jump   - left->frame->fid : -1,
                right_ok, (right->frame)? right->frame->fid : -1, right_jump,(right->frame)? right->frame->fid - right_jump : -1,
                fid);
          fwrite( visited, sizeof(real*), nframes, fp );
#endif

        }
      } while(left_ok||right_ok);


      Measurements_Reference_Free(left);
      Measurements_Reference_Free(right);
    }

    //
    // Indescriminately fill in the rest
    //
    { int fid=0;
      while( fid < nframes )
      { while( visited[fid] && fid<nframes ) fid++; // find the left side of the next gap
        fid = HMM_Reclassify_Fill_Gap( index, nframes, shp_dists, vel_dists, nwhisk, S,T,E, visited, likelihood, fid);
#ifdef DEBUG_HMM_RECLASSIFY
        debug("Frame: %5d likelihood: %g\n", fid, likelihood[fid]);
        fwrite( visited, sizeof(real*), nframes, fp );
#endif
      }
    }


#ifdef DEBUG_HMM_RECLASSIFY
    fclose(fp);
#endif
    //
    // Cleanup
    //
    heap_free(q);
    free_frame_index(index);
    free(likelihood);
    free(visited);
    free(S);
    free(E);
  } // end re-classification

  //
  // Save results
  //

  Measurements_Table_To_Filename( Get_String_Arg("dest"), table, nrows );

  //
  // Cleanup
  //
  //free(T);
  Free_Distributions(vel_dists);
  Free_Distributions(shp_dists);
  Free_Measurements_Table(table);

  return 0;
}
#endif
