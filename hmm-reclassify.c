#include "compat.h"
#include <assert.h>
#include <string.h>
#include "error.h"
#include "utilities.h"
#include "traj.h"
#include "viterbi.h"
#include "common.h"

#define DEBUG_HMM_RECLASSIFY
#define DEBUG_LRMODEL_ESTIMATE_TRANSITIONS
#if 0
#endif

#define HMM_RECLASSIFY_DISTS_NBINS   (32)
#define HMM_RECLASSIFY_BASELINE_LOG2 (-500.0)

void _doubles_to_filename(char* filename, double* a, int n)
{ FILE *fp = fopen(filename,"wb");
  fwrite(a, sizeof(double), n, fp);
  fclose(fp);
}

//
// Build the transition state model
// --------------------------------
//
// This is a left-right model with two types of states: `whiskers` and `junk`
// The sequence (of labeling events) can start and stop anywhere in the
// state diagram.  
//
// `Whiskers` have two outputs: one to the next `whisker`, and one to the next
// `junk`
//
// `Junk` also have two outputs: one to self, and one to the next `whisker`.
// This really represents any hairs or microvibrissa in the space adjacent to
// a whisker.
//
// The number of whisker states is set by the number of states given by the
// approximate/incomplete input trajectories. `junk` states lie on either
// side of each whisker state like: J0 W0 J1 W1 ... J(N-1) W(N-1) JN
//
// So there are 2 N + 1 states.  For convenience, J states will be
// represented as even integers and W states as odd.
//

real *LRModel_Alloc_Transitions( int nwhisk )
{ int N = 2 * nwhisk + 1;
  return Guarded_Malloc( sizeof(real)*N*N, "transition state matrix" );
}

real *LRModel_Init_Uniform_Transitions( real *T, int nwhisk )
{ int i, N = 2 * nwhisk + 1; 
  memset( T, 0, sizeof(double)*N*N );
  for( i=0; i<N; i+=2 ) // junk
  { T[i, i  ] = 0.5;    //      to self
    T[i, i+1] = 0.5;    //      to next whisker
  }
  for( i=1; i<N; i+=2 ) // whisker
  { T[i, i+1] = 0.5;    //      to next junk
    T[i, i+2] = 0.5;    //      to next whisker
  }
  return T;
}

void LRModel_Estimate_Transitions(real *T, int nwhisk, Measurements *table, int nrows )
{ Measurements *row;
  int N = 2 * nwhisk + 1; 

  memset( T, 0, sizeof(double)*N*N );
  Sort_Measurements_Table_Time_Face( table, nrows );

  for( row = table; row < table+nrows; row++ )
  { Measurements *bookmark = row;             //remember the frame's start
    int cur = row->fid;               //1. check if any are labelled
    for(; row->fid == cur; row++)             //iterate through segments in frame
    { if( row->state != -1 )          //2. if found labelled, then run through labelling process
      { int state = (bookmark->state != -1 ); // start at state=1 if first is a whisker (not junk)
        int last = state;                     // last is the last class (junk<0> or whisker<1>)
        for( row = bookmark+1; row->fid == cur; row++) 
        { int c = (row->state != -1);
          int delta = 1;              //delta state (1 for cases j->w and w->j)
          if( c == last ) //j->j or w->w
            delta = 2*( c&1 );  // 0 or 2
          T[N*state +  state+delta] ++;           // aggregate
          state+=delta;
          last = c;
        }
        row--;
      }
    }
  }
  //row-wise norm - make T a stochastic matrix
  { real *row;
    for(row = T+N*(N-1); row >= T; row -= N )
    { real *s,*t; 
      real sum = 0.0;
      s = t = row+N;
      while(t-- > row) sum += *t;
      if( sum )
        while(s-- > row) *s /= sum;
    }
#ifdef DEBUG_LRMODEL_ESTIMATE_TRANSITIONS
    // Assert properly normed
    { real *t,sum = 0.0;
      for( row=T; row < t + N*N; row++ )
      { if( (row-T)%N )
        { sum += *row;
        } else {
          assert( fabs(sum-1.0) < 1e-3 );
          sum = 0.0;
        }
      }
    } 
#endif
  }
  return;
}

void LRModel_Log2_Transitions( real *T, int nwhisk, real baseline_log2 )
{ int N = nwhisk*2 + 1;
  real s,*t; 
  real thresh = pow(2.0,baseline_log2);
  t = T+N*N;
  while(t-- > T) 
  { s = *t;
    if( s <= thresh )
      *t = baseline_log2;
    else
      *t = log2(s);
  }
}

real *LRModel_Alloc_Starts( int nwhisk )
{ int N = nwhisk*2 + 1;
  return Guarded_Malloc( N*sizeof(real), " LRModel_Alloc_Starts " );
}

void LRModel_Compute_Starts_For_Two_Classes_Log2( real *S, int nwhisk, Measurements *first, Distributions *shp_dists )
{ int N = 2 * nwhisk + 1;
  while(N--)
    S[N] = Eval_Likelihood_Log2( shp_dists, first->data, N&1 );
}

void LRModel_Compute_Starts_For_Distinct_Whiskers_Log2( real *S, int nwhisk, Measurements *first, Distributions *shp_dists )
{ int N = 2 * nwhisk + 1;
  int i,iwhisk;
  double *shp = first->data;
  // Assumes the following encoding of conditionals for the distributions:
  //   Junk is when N even     -> translates to state 0
  //   Whiskers are when N odd -> translates to state 1..nwhisk (inclusive)
  { real log2p = Eval_Likelihood_Log2( shp_dists, shp, 0 );
    for(i=0;i<N;i+=2)
      S[i] = log2p;
    for(i=1,iwhisk=1;i<N;i+=2,iwhisk++)
      S[i] = Eval_Likelihood_Log2( shp_dists, shp, iwhisk ); 
  }
}

real *LRModel_Alloc_Emissions( int nwhisk, int nobs )
{ int N = 2 * nwhisk + 1;
  return Guarded_Malloc( sizeof(real)*N*nobs, "LRModel_Alloc_Emmisions" );
}

real *LRModel_Request_Static_Resizable_Emissions( int nwhisk, int nobs )
{ int N = 2 * nwhisk + 1; 
  static int maxsize = 0;
  static real *E = NULL;
  E = request_storage(E, &maxsize, sizeof(real), nobs*N*N, "LRModel_Request_Static_Resizable_Emissions");
  return E;
}

void LRModel_Compute_Emissions_For_Two_Classes_Log2( real *E, int nwhisk, Measurements *obs, int nobs, Distributions *shp_dists )
{ int N = 2 * nwhisk + 1;
  int i,j;
  for(i=0;i<N;i++)
  { real *row = E + i*nobs;
    int state = i&1;
    for(j=0;j<nobs;j++)
      row[j] = Eval_Likelihood_Log2( shp_dists, obs[j].data, state );
  }
}

void LRModel_Compute_Emissions_For_Distinct_Whiskers_Log2( real *E, int nwhisk, Measurements *obs, int nobs, Distributions *shp_dists ) 
{ int N = 2 * nwhisk + 1;
  int i,j, iwhisk;
  // Assumes the following encoding of conditionals for the distributions:
  //   Junk is when N even     -> translates to state 0
  //   Whiskers are when N odd -> translates to state 1..nwhisk (inclusive)
  
  // first do junk
  for(j=0;j<nobs;j++)
  { real logp = Eval_Likelihood_Log2( shp_dists, obs[j].data, 0 );  
    for(i=0;i<N;i+=2)  
      E[i*nobs+j] = logp;
  }

  // now whiskers
  for(i=1,iwhisk=1;i<N;i+=2,iwhisk++)
  { real *row = E + i*nobs;
    for(j=0;j<nobs;j++)
      row[j] = Eval_Likelihood_Log2( shp_dists, obs[j].data, iwhisk );
  }
}

int *_static_range( int n )
{ static int maxsize = 0,
            *buf = NULL;
  buf = request_storage( buf, &maxsize, sizeof(int), n, "_static_range" );
  while(n--)
    buf[n] = n;
  return buf;
}

#ifdef TEST_HMM_RECLASSIFY_1

char *Spec[] = {"[-h|--help] <source:string> <dest:string>",NULL};
int main(int argc, char*argv[])
{ int nrows;
  int nwhisk;
  Measurements *table;
  Distributions *shp_dists, *vel_dists;
  real *T;

  Process_Arguments( argc, argv, Spec, 0 );
  
  help( Is_Arg_Matched("-h") || Is_Arg_Matched("--help"),
    "--------------\n"
    "HMM-Reclassify\n"
    "--------------\n"
    " <source> should be the filename of a `measurements` file where an initial guess has been made as\n"
    " to the identity of many of the whiskers.  These initial assignments are used to build a \n"
    " probabalistic model of the process by which whiskers are identified as one travels along the face\n"
    "\n"
    " <dest> should be the destination filename.  After applying the probibalistic model to identify\n"
    " whiskers in each frame, the results are saved to this file.\n" );

  table = Measurements_Table_From_Filename( Get_String_Arg("source"), &nrows );
  Sort_Measurements_Table_State_Time(table, nrows);

  //
  // Compute velocities using approximate/incomplete labelling
  //
  Measurements_Table_Compute_Velocities(table,nrows);
  
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
  //

  T = LRModel_Alloc_Transitions(nwhisk);
  LRModel_Estimate_Transitions( T, nwhisk, table, nrows );
  LRModel_Log2_Transitions(T,nwhisk, HMM_RECLASSIFY_BASELINE_LOG2);

  //
  // Want to condition distributions over whether it's a whisker or not,
  // so forget input identities.
  //
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
  // Now build distributions.  Notice that the velocities were computed
  // using the approximate/incomplete trajectories.
  //

  Sort_Measurements_Table_State_Time(table, nrows);
  shp_dists = Build_Distributions( table, nrows, HMM_RECLASSIFY_DISTS_NBINS );
  vel_dists = Build_Velocity_Distributions( table, nrows, HMM_RECLASSIFY_DISTS_NBINS );

  //
  // Process frames independantly
  //
  Sort_Measurements_Table_Time_Face( table, nrows );
  { real *E = LRModel_Request_Static_Resizable_Emissions( 
                  nwhisk, 
                  (nrows/table[nrows-1].fid)*2); // initial alloc for twice the average sequence length
    real *S = LRModel_Alloc_Starts( nwhisk );
    Measurements *row;

    for( row = table; row < table+nrows; row++ )
    { Measurements *bookmark = row;
      int fid = row->fid;
      int nobs;
      while( (row++)->fid == fid );
      nobs = row - bookmark - 1;
      
      LRModel_Compute_Starts_For_Two_Classes_Log2( S, nwhisk, bookmark, shp_dists );

      E = LRModel_Request_Static_Resizable_Emissions( nwhisk, nobs );
      LRModel_Compute_Emissions_For_Two_Classes_Log2( E, nwhisk, bookmark, nobs, shp_dists );

      { int N = 2*nwhisk + 1;
        ViterbiResult *result = Forward_Viterbi_Log2( _static_range(nobs), nobs, S, T, E, nobs, N );

        // Commit the result
#ifdef DEBUG_HMM_RECLASSIFY
        progress("[%5d/%5d]: total: %+5.5f prob: %+5.5f (delta: %+5.5f)\n", 
            fid,                              // frame id
            table[nrows-1].fid+1,             // total frames
            result->total,                    // log2 prob(obs|model)           //?
            result->prob,                     // log2 prob(path|obs)            //?
            result->total - result->prob );                                     //?
        assert( nobs == result->n );
        { int i = nobs;  //result should be strictly increasing
          while(--i) assert( result->sequence[i] - result->sequence[i-1] >=0 );
        }
#endif
        { int i = nobs;
          int *seq = result->sequence;
          while(i--)
          { int s = seq[i];
            bookmark[i].state = (s&1) ? (s-1)/2  : -1; // decode viterbi state to whisker label {-1:junk, 0..n:whiskers} 
          }
        } // end commit viterbi result
        Free_Viterbi_Result(result);  // FIXME: thrashing the heap
      } // end viterbi solution
#ifdef DEBUG_HMM_RECLASSIFY
      assert(row!=bookmark);
#endif
    } // end loop over observations
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
  Free_Distributions(vel_dists);
  Free_Distributions(shp_dists);
  Free_Measurements_Table(table);

  return 0;
}

#endif
