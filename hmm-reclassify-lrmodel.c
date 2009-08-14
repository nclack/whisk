#include "compat.h"
#include <assert.h>
#include <string.h>
#include "error.h"
#include "utilities.h"
#include "traj.h"
#include "viterbi.h"
#include "common.h"

#include "hmm-reclassify-lrmodel.h"

#define DEBUG_LRMODEL_ESTIMATE_TRANSITIONS
#if 0
#endif


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
// The state diagram for three whiskers looks like
//
//             W0----W1----W2     -+ emissions evaled as whisker
//            /  \  /  \  /  \  
//          J0    J1    J2    J3  -+ emissions evaled as junk/other

real *LRModel_Alloc_Transitions( int nwhisk )
{ int N = 2 * nwhisk + 1;
  return Guarded_Malloc( sizeof(real)*N*N, "transition state matrix" );
}

real *LRModel_Init_Uniform_Transitions( real *T, int nwhisk )
{ int i, N = 2 * nwhisk + 1; 
  memset( T, 0, sizeof(double)*N*N );
  for( i=0; i<N-1; i+=2 ) // junk
  { T[i*N + i  ] = 0.5;      //      to self
    T[i*N + i+1] = 0.5;      //      to next whisker
  }
  for( i=1; i<N-2; i+=2 ) // whisker
  { T[i*N + i+1] = 0.5;      //      to next junk
    T[i*N + i+2] = 0.5;      //      to next whisker
  }
  T[i*N+i+1] = 1;
  // take care of last row
  if( (N-1)%2 == 0 )
    T[ (N-1)*N + N-1 ] = 1;  
  return T;
}

void LRModel_Estimate_Transitions(real *T, int nwhisk, Measurements *table, int nrows )
{ Measurements *row;
  int N = 2 * nwhisk + 1; 

  memset( T, 0, sizeof(double)*N*N );
  Sort_Measurements_Table_Time_Face( table, nrows );

  row = table;
  //for( row = table; row < table+nrows; row++ )
  while( row < table+nrows )
  { Measurements *bookmark = row;             //remember the frame's start
    int cur = row->fid;               //1. check if any are labelled
    for(; row->fid == cur; row++)             //iterate through segments in frame
    { if( row->state != -1 )          //2. if found labelled, then run through labelling process
      { int state = (bookmark->state != -1 ); // start at state=1 if first is a whisker (not junk)
        int last = state;                     // last is the last class (junk<0> or whisker<1>)
#ifdef DEBUG_LRMODEL_ESTIMATE_TRANSITIONS
          progress("Frame: %5d  Whisker: %3d  State: %3d \n", 
              bookmark->fid, bookmark->wid, bookmark->state);
#endif
        for( row = bookmark+1; row->fid == cur; row++) 
        { int c = (row->state != -1);
          int delta = 1;              //delta state (1 for cases j->w and w->j)
          if( c == last ) //j->j or w->w
            delta = 2*( c&1 );  // 0 or 2
#ifdef DEBUG_LRMODEL_ESTIMATE_TRANSITIONS
          progress("Frame: %5d  Whisker: %3d  State: %3d Transition: %2d -> %2d\n", 
              row->fid, row->wid, row->state, state, state+delta );
          assert( (state!=(N-1)) || (delta==0) );
#endif
          T[N*state +  state+delta] ++;  // accumulate
          state+=delta;                  // Note: `state` is a natural number
          last = c;                      // Note: `last`, `c` are 0 (junk) or 1(whisker) 
        }
        //row--;
        break;
      }
    }
  }
  //increment all allowable transitions once.
  // don't want to disallow non-observed transitions
  { int i;
    for(i=0;i<N-1;i+=2)  //even rows (junk)
    { T[i*N + i   ]++;
      T[i*N + i+1 ]++;
    }
    for(i=1;i<N-2;i+=2)  //odd rows (whiskers)
    { T[i*N + i+1 ]++;
      T[i*N + i+2 ]++;
    }
    T[i*N + i+1]++; //last odd state
    // don't really need to do last row
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
  double v[2] = { Eval_Likelihood_Log2( shp_dists, first->data, 0 ),
                  Eval_Likelihood_Log2( shp_dists, first->data, 1 ) }; 
  while(N--)
    S[N] = v[ N%2 ];
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
  static size_t maxsize = 0;
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

