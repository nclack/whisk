//
// TODO: FIXME: how to handle duplicate long segments in the middle of the sequence?
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

// Make one model the default
#ifndef TEST_HMM_RECLASSIFY_LR_MODEL_W_DELETIONS
#define TEST_HMM_RECLASSIFY_LR_MODEL
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

//
// DEBUG DEFINES
//
#if 0
#define DEBUG_HMM_RECLASSIFY
#define DEBUG_HMM_RECLASSIFY_EXTRA
#endif


typedef int   (*Tpf_State_Count)                                       ( int nwhisk );
typedef real* (*Tpf_Alloc_Transitions)                                 ( int nwhisk );
typedef real* (*Tpf_Init_Uniform_Transitions)                          ( real *T, int nwhisk );
typedef void  (*Tpf_Estimate_Transitions)                              ( real *T, int nwhisk, Measurements *table, int nrows );
typedef void  (*Tpf_Log2_Transitions)                                  ( real *T, int nwhisk, real baseline_log2 );
typedef real* (*Tpf_Alloc_Starts)                                      ( int nwhisk );
typedef void  (*Tpf_Compute_Starts_For_Two_Classes_Log2)               ( real *S, real *T, int nwhisk, Measurements *first, Distributions *shp_dists );
typedef void  (*Tpf_Compute_Starts_For_Distinct_Whiskers_Log2)         ( real *S, real *T, int nwhisk, Measurements *first, Distributions *shp_dists );
typedef real* (*Tpf_Alloc_Emissions)                                   ( int nwhisk, int nobs );
typedef real* (*Tpf_Request_Static_Resizable_Emissions)                ( int nwhisk, int nobs );
typedef void  (*Tpf_Compute_Emissions_For_Two_Classes_Log2)            ( real *E, int nwhisk, Measurements *obs, int nobs, Distributions *shp_dists );
typedef void  (*Tpf_Compute_Emissions_For_Distinct_Whiskers_Log2)      ( real *E, int nwhisk, Measurements *obs, int nobs, Distributions *shp_dists );
typedef void  (*Tpf_Compute_Emissions_For_Two_Classes_W_History_Log2)  ( real *E, int nwhisk, Measurements *obs, int nobs, Measurements *prev, int nprev, Measurements** history, int nhist, Distributions *shp_dists, Distributions *vel_dists );
typedef int   (*Tpf_State_Decode)                                      ( int state );

int* _static_range( int n )
{ static size_t maxsize = 0;
  static int *buf = NULL;
  buf = request_storage( buf, &maxsize, sizeof(int), n, "_static_range" );
  while(n--)
    buf[n] = n;
  return buf;
}

#ifdef TEST_HMM_RECLASSIFY_LR_MODEL
#include "hmm-reclassify-lrmodel.h"
Tpf_State_Count                                       pf_State_Count                                       =  LRModel_State_Count;
Tpf_Alloc_Transitions                                 pf_Alloc_Transitions                                 =  LRModel_Alloc_Transitions;
Tpf_Init_Uniform_Transitions                          pf_Init_Uniform_Transitions                          =  LRModel_Init_Uniform_Transitions;
Tpf_Estimate_Transitions                              pf_Estimate_Transitions                              =  LRModel_Estimate_Transitions;
Tpf_Log2_Transitions                                  pf_Log2_Transitions                                  =  LRModel_Log2_Transitions;
Tpf_Alloc_Starts                                      pf_Alloc_Starts                                      =  LRModel_Alloc_Starts;
Tpf_Compute_Starts_For_Two_Classes_Log2               pf_Compute_Starts_For_Two_Classes_Log2               =  LRModel_Compute_Starts_For_Two_Classes_Log2;
Tpf_Compute_Starts_For_Distinct_Whiskers_Log2         pf_Compute_Starts_For_Distinct_Whiskers_Log2         =  LRModel_Compute_Starts_For_Distinct_Whiskers_Log2;
Tpf_Alloc_Emissions                                   pf_Alloc_Emissions                                   =  LRModel_Alloc_Emissions;
Tpf_Request_Static_Resizable_Emissions                pf_Request_Static_Resizable_Emissions                =  LRModel_Request_Static_Resizable_Emissions;
Tpf_Compute_Emissions_For_Two_Classes_Log2            pf_Compute_Emissions_For_Two_Classes_Log2            =  LRModel_Compute_Emissions_For_Two_Classes_Log2;
Tpf_Compute_Emissions_For_Distinct_Whiskers_Log2      pf_Compute_Emissions_For_Distinct_Whiskers_Log2      =  LRModel_Compute_Emissions_For_Distinct_Whiskers_Log2;
Tpf_Compute_Emissions_For_Two_Classes_W_History_Log2  pf_Compute_Emissions_For_Two_Classes_W_History_Log2  =  LRModel_Compute_Emissions_For_Two_Classes_W_History_Log2;
Tpf_State_Decode                                      pf_State_Decode                                      =  LRModel_State_Decode;
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
    Measurements *row,
                 *prev = NULL,
                 **last = Guarded_Malloc( sizeof(Measurements*)*nwhisk, "alloc array for last identified whiskers" );
    int nprev=0;
    memset(last,0, sizeof(Measurements*)*nwhisk );
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
          debug("\t%p", last[i]);
        debug("\n");
      }
#endif
      if(!prev)
        (*pf_Compute_Emissions_For_Two_Classes_Log2)( E, nwhisk, bookmark, nobs, shp_dists );
      else
        (*pf_Compute_Emissions_For_Two_Classes_W_History_Log2)( E,            
                                                                nwhisk, 
                                                                bookmark, nobs, 
                                                                prev, nprev,
                                                                last, nwhisk, 
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
          memset(last,0, sizeof(Measurements*)*nwhisk );
          while(i--)
          { int s = seq[i],
                lbl = (*pf_State_Decode)(s);
            bookmark[i].state = lbl;   // decode viterbi state to whisker label {-1:junk, 0..n:whiskers} 
            if(lbl>-1)
              last[lbl] = bookmark+i;
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
      prev = bookmark;
      nprev = nobs;
#ifdef DEBUG_HMM_RECLASSIFY
      assert(row!=bookmark);
#endif
    } // end loop over observations
    free(last);
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

// TODO: fix root index is 1 not 0 errors in heap
//
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
    //??? Faster?: SWAP( data[i], data[argmax] );
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

inline real *heap_pop_and_replace_head( heap *h, real *v )
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
  static const real tol = 1e-10;

  p = likelihood + nframes;
  while(p-- > likelihood+1)
    if(    (p[ 0] - p[-1]) > tol
        && (p[ 0] - p[ 1]) > tol )
      count++;

  // 2. alloc queue - a binary tree
  //    children of i are at i<<1,i<<1+1
  q = heap_alloc( 2*count );

  // 3. place local minima  (tolerance suppresses a few)
  //    a.  throw them, unordered, onto the heap
  //    b.  use heap_build, O(count)
  //    Repeated heap_insert would be O( count x log(count) )
  { real **minima = q->data;
    p = likelihood + nframes;
    while(p-- > likelihood+1)
      if(    (p[ 0] - p[-1]) > tol
          && (p[ 0] - p[ 1]) > tol ) 
        *(minima++) = p;
    q->size = minima - q->data;
    heap_build(q);
  }

  return q;
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
    Measurements *row,
                 **last = Guarded_Malloc( sizeof(Measurements*)*nwhisk, "alloc array for last identified whiskers" );
    memset(last,0, sizeof(Measurements*)*nwhisk );
    row = table;
    // Advance over rows
    while( row < table+nrows )
    { Measurements *bookmark = row;
      int fid = row->fid;
      int nobs;
      // Advance over chunks of rows in same frame 
      // (since sorted, this is the whole frame)
      while(row < table+nrows && row->fid == fid  ) 
      { 
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
          debug("Frame: %5d  Whisker: %3d  State: %3d \n", row->fid, row->wid, row->state);
#endif
        ++row;
      }
      nobs = row - bookmark;
      
      // 
      // Compute starts and emissions for frame
      //
      (*pf_Compute_Starts_For_Two_Classes_Log2)( S, T, nwhisk, bookmark, shp_dists );
      E = (*pf_Request_Static_Resizable_Emissions)( nwhisk, nobs );
#ifdef DEBUG_HMM_RECLASSIFY_EXTRA
      { int i;
        debug("Last\n");
        for(i=0; i<nwhisk; i++)
          debug("\t%p", last[i]);
        debug("\n");
      }
#endif
      (*pf_Compute_Emissions_For_Two_Classes_Log2)( E, nwhisk, bookmark, nobs, shp_dists );

      // 
      // Process Frame
      //
      {

        // XXX: to be continued...

      }

#ifdef DEBUG_HMM_RECLASSIFY
      assert(row!=bookmark);
#endif
    } // end loop over observations
    free(last);
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
