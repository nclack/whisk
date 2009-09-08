// TODO: Refactor - what are labelled as `deletions` now should be called
// `insertions`
#include "compat.h"
#include <assert.h>
#include <string.h>
#include "error.h"
#include "utilities.h"
#include "traj.h"
#include "common.h"
#include "hmm-reclassify-lrmodel-w-deletions.h" 
#include "viterbi.h"

#define DEBUG_LRMODEL_W_DELETIONS_CONSTANT_TRANSITION_PROBABILITY 0.001

#if 0
#define DEBUG_LRMODEL_W_DELETIONS_ESTIMATE_TRANSITIONS
#define DEBUG_LRMODEL_W_DELETIONS_DISALLOW_DELETIONS
#define DEBUG_LRDELMODEL_COMPUTE_EMISSIONS_FOR_TWO_CLASSES_W_HISTORY_LOG2
#endif

//
// Build the transition state model
// --------------------------------
//
// This is a left-right hidden markov model conditioned on two classes
// (`whiskers` and `junk`) that allows for deletion of certain whiskers.  By
// deletion, I mean that a segment can get classified as a whisker but be
// excluded from the final labelling.  This is neccessary, for example, when
// one whisker is traced twice.  Although 4 whiskers are expected, five
// whisker-like segments are present.  Which four of these five should be
// identified as whiskers?
//
// For the left-right models considered here where the sequence (of labeling
// events) can start and stop anywhere in the state diagram, deletions are
// normally restricted to the beginning or end of the sequence.  By adding a
// `deletion` states between each `whisker` state, one or more deletions can be
// permitted in the middle of the sequence.
//
// `Whiskers` have three outputs: one to the next `whisker`, one to a `delete`
// state and one to the next `junk`.
//
// `Junk` have three outputs: one to self, one to the next `whisker`, and one
// to the last `deleted` state..  This really represents any hairs or
// microvibrissa in the space adjacent to a whisker.
//
// `Deleted` have two outputs: one to the next whisker, and one to the next
//  `junk`.
//
// The number of whisker states is set as input. `junk` and `deleted` states
// lie on either side of each whisker state like: 
//
// J0 W0 D0 J1 W1 D1 ... D(N-2) J(N-1) W(N-1) JN
//
// The state diagram for three whiskers looks like
//
//                D0    D1   (D3) -+                        
//               /| \  /| \        | emissions evaled as whiskers
//             W0-+--W1-+--W2     -+   (D3 is for convenience)
//            /  \| /  \| /  \  
//          J0    J1    J2    J3  -+ emissions evaled as junk/other
//
// So there are 3 N (+1 dummy) states.  For convenience, J states will be
// represented as 0(mod 3), W will be 1(mod 3), and J will be 2(mod 3).
//
// Conditioning emission distributions on two classes ('junk' and 'whiskers') 
// works for this approach, but conditioning on distinct whiskers isn't really
// natural.  One doesn't know apriori whether the 'delete' node should look 
// like the previous or next whisker.
//
int LRDelModel_State_Count( int nwhisk )
{ return 3*nwhisk+1; }

real *LRDelModel_Alloc_Transitions( int nwhisk )
{ int N = 3 * nwhisk + 1;
  return Guarded_Malloc( sizeof(real)*N*N, "transition state matrix" );
}

real *LRDelModel_Init_Uniform_Transitions( real *T, int nwhisk )
{ int i, N = 3 * nwhisk + 1; 
  static const double third = 1.0/3.0;
  memset( T, 0, sizeof(double)*N*N );

  // First row (i=0) source: junk
  T[1] = 0.5;   //      to next whisker
  T[0] = 0.5;   //      to self          

  // Following rows
  for( i=1; i<N-3; i+=3 ) // whisker
  { 
    T[i*N + i+2] = third;   //      to next junk     
    T[i*N + i+3] = third;   //      to next whisker
#ifdef DEBUG_LRMODEL_W_DELETIONS_DISALLOW_DELETIONS
    T[i*N + i+1]=0;         //erase to this delete
#elif defined(DEBUG_LRMODEL_W_DELETIONS_CONSTANT_TRANSITION_PROBABILITY)
    T[i*N + i+1] = third * DEBUG_LRMODEL_W_DELETIONS_CONSTANT_TRANSITION_PROBABILITY; // to this delete
#else
    T[i*N + i+1] = third;   //      to this delete
#endif
  }
  T[i*N + i+2] = 1.0;       //      to end  junk     // Notice there is no path to the end delete state
  
  for( i=2; i<N-2; i+=3 ) // delete
  { T[i*N + i+1] = third;   //      to next junk
    T[i*N + i+2] = third;   //      to next whisker
  }
  T[i*N+i+1] = 1.0;         //      to end  junk

  for( i=3; i<N-1; i+=3 ) // junk
  { T[i*N + i+1] = third; //      to next whisker
    T[i*N + i  ] = third; //      to self          
    T[i*N + i-1] = third; //      to last delete
#ifdef DEBUG_LRMODEL_W_DELETIONS_DISALLOW_DELETIONS
    T[i*N + i-1]=0;        //erase to last delete
#elif defined(DEBUG_LRMODEL_W_DELETIONS_CONSTANT_TRANSITION_PROBABILITY)
    T[i*N + i-1] = third * DEBUG_LRMODEL_W_DELETIONS_CONSTANT_TRANSITION_PROBABILITY; // to this delete
#else
    T[i*N + i-1] = third;        //      to last delete
#endif
  }
  T[i*N + i  ] = 1.0;     //      to self          
  
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
  }
#ifdef DEBUG_LRMODEL_W_DELETIONS_ESTIMATE_TRANSITIONS
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
  return T;
}

void LRDelModel_Estimate_Transitions(real *T, int nwhisk, Measurements *table, int nrows )
  // TODO: How to estimate transitions to/from deletions?
  //       Copy outbound transitions from appt whisker  (e.g. D0->J1 == W0->J1)
  //       Inbound??
  //
  // FIXME
{ Measurements *row;
  int N = 3 * nwhisk + 1;

  memset( T, 0, sizeof(double)*N*N );
  Sort_Measurements_Table_Time_Face( table, nrows );

  row = table;
  while( row < table+nrows )
  { Measurements *bookmark = row;             //remember the frame's start
    int cur = row->fid;               //1. check if any are labelled
    for(; row->fid == cur; row++)             //iterate through segments in frame
    { if( row->state != -1 )          //2. if found labelled, then run through labelling process
      { int state = (bookmark->state != -1 ); // start at state=1 if first is a whisker (not junk)
        int last = state;                     // last is the last class (junk<0> or whisker<1>)
#ifdef DEBUG_LRMODEL_W_DELETIONS_ESTIMATE_TRANSITIONS
          progress("Frame: %5d  Whisker: %3d  State: %3d \n", 
              bookmark->fid, bookmark->wid, bookmark->state);                         // Deltas    last->c  2*last+c 
#endif                                                                                // j->w   1     0  1   1       
        for( row = bookmark+1; row->fid == cur; row++)                                // w->j   2     1  0   2       
        { int c = (row->state != -1); // either junk<0> or whisker<1>                 // j->j   0     0  0   0       
          int delta = (last<<1) + c;    //delta state (1 for cases j->w and w->j)     // w->w   3     1  1   3                 
#ifdef DEBUG_LRMODEL_W_DELETIONS_ESTIMATE_TRANSITIONS
          progress("Frame: %5d  Whisker: %3d  State: %3d Transition: %2d -> %2d\n",            
              row->fid, row->wid, row->state, state, state+delta );
          assert( (state!=(N-1)) || (delta==0) );
          assert( (state%3 == 0) || (state%3 == 1) );
          assert( ((state+delta)%3 == 0) || ((state+delta)%3 == 1) );
#endif
          T[N*state +  state+delta] ++;  // accumulate
          state+=delta;                  // Note: `state` is a natural number
          last = c;                      // Note: `last`, `c` are 0 (junk) or 1(whisker) 
        }
        break;
      }
    }
  }

  //increment all allowable transitions once.
  // don't want to disallow non-observed transitions
  { int i;
    // First row (i=0) source: junk
    T[1]++;       //      to next whisker
    T[0]++;       //      to self          
    // Following rows
    for( i=1; i<N-3; i+=3 ) // whisker
    { 
      T[i*N + i+2]++;        //      to next junk     
      T[i*N + i+3]++;        //      to next whisker
#ifdef DEBUG_LRMODEL_W_DELETIONS_DISALLOW_DELETIONS
      T[i*N + i+1]=0;        //erase to this delete
#elif defined(DEBUG_LRMODEL_W_DELETIONS_CONSTANT_TRANSITION_PROBABILITY)
      T[i*N + i+1] = T[i*N + (i+3)] * DEBUG_LRMODEL_W_DELETIONS_CONSTANT_TRANSITION_PROBABILITY; // to this delete
#else
      T[i*N + i+1]++;        //      to this delete
#endif
    }
    T[i*N + i+2]++;           //      to end  junk     // Notice there is no path to the end delete state

    for( i=2; i<N-2; i+=3 ) // delete - copy exit probs from this whisker
    { T[i*N + i+1] = T[(i-1)*N + i+1];        //      to next junk
      T[i*N + i+2] = T[(i-1)*N + i+2];        //      to next whisker
    }
    T[i*N+i+1]++;             //      to end  junk

    for( i=3; i<N-1; i+=3 ) // junk
    { T[i*N + i+1]++;      //      to next whisker
      T[i*N + i  ]++;      //      to self          
#ifdef DEBUG_LRMODEL_W_DELETIONS_DISALLOW_DELETIONS
      T[i*N + i-1]=0;        //erase to last delete
#elif defined(DEBUG_LRMODEL_W_DELETIONS_CONSTANT_TRANSITION_PROBABILITY)
      T[i*N + i-1] = T[i*N + (i+1)] * DEBUG_LRMODEL_W_DELETIONS_CONSTANT_TRANSITION_PROBABILITY; // to this delete
#else
      T[i*N + i-1]++;        //      to last delete
#endif
    }
    T[i*N + i  ]++;         //      to self          
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
#ifdef DEBUG_LRMODEL_W_DELETIONS_ESTIMATE_TRANSITIONS
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

void LRDelModel_Log2_Transitions( real *T, int nwhisk, real baseline_log2 )
{ int N = nwhisk*3 + 1;
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

real *LRDelModel_Alloc_Starts( int nwhisk )
{ int N = nwhisk*3 + 1;
  return Guarded_Malloc( N*sizeof(real), " LRModel_Alloc_Starts " );
}

void LRDelModel_Compute_Starts_For_Two_Classes_Log2( real *S, real *T, int nwhisk, Measurements *first, Distributions *shp_dists )
{ int N = 3 * nwhisk + 1;
  double v[3] = { Eval_Likelihood_Log2( shp_dists, first->data, 0 ),
                  Eval_Likelihood_Log2( shp_dists, first->data, 1 ),
                  -500.0                                            }; 
  double t[3] = { T[0], T[1], T[1] };
  double log2p_missing = -10.0; // ~1/1000
  while(N--)
    S[N] = v[ N%3 ] + t[ N%3 ] - (N/3)*log2p_missing;
}

void LRDelModel_Compute_Starts_For_Distinct_Whiskers_Log2( real *S, real *T, int nwhisk, Measurements *first, Distributions *shp_dists )
{ int N = 3 * nwhisk + 1;
  int i,iwhisk;
  double *shp = first->data;
  // Assumes the following encoding of conditionals for the distributions:
  //   Junk                    -> translates to state 0
  //   Whiskers                -> translates to state 1..nwhisk (inclusive)
  { real log2p = Eval_Likelihood_Log2( shp_dists, shp, 0 );
    for(i=0;i<N;i+=3)
      S[i] = log2p;
    for(i=1,iwhisk=1;i<N;i+=3,iwhisk++)
    { log2p  = Eval_Likelihood_Log2( shp_dists, shp, iwhisk ); 
      S[i]   = log2p;
      S[i+1] = log2p;
    }
  }
}

real *LRDelModel_Alloc_Emissions( int nwhisk, int nobs )
{ int N = 3 * nwhisk + 1;
  return Guarded_Malloc( sizeof(real)*N*nobs, "LRModel_Alloc_Emissions" );
}

real *LRDelModel_Request_Static_Resizable_Emissions( int nwhisk, int nobs )
{ int N = 3 * nwhisk + 1; 
  static size_t maxsize = 0;
  static real *E = NULL;
  E = request_storage(E, &maxsize, sizeof(real), nobs*N*N, "LRModel_Request_Static_Resizable_Emissions");
  return E;
}

void LRDelModel_Compute_Emissions_For_Two_Classes_Log2( real *E, int nwhisk, Measurements *obs, int nobs, Distributions *shp_dists )
{ int N = 3 * nwhisk + 1;
  int i,j;
  for(i=0;i<N;i++)
  { real *row = E + i*nobs;
    int state = (i%3)!=0;
    for(j=0;j<nobs;j++)
      row[j] = Eval_Likelihood_Log2( shp_dists, obs[j].data, state );
  }
}

void  LRDelModel_Compute_Emissions_For_Two_Classes_W_History_Log2(
    real *E, 
    int nwhisk, 
    Measurements *obs, 
    int nobs, 
    Measurements** history, 
    int nhist, 
    Distributions *shp_dists,
    Distributions *vel_dists )
{ int N = 3 * nwhisk + 1;
  int i,j;
  real max_logp_delta = -FLT_MAX;
  Measurements *prev;

  // over all observables and all states
  // compute max delta probability.
  // ...this will be used as the default delta prob when a
  //    previous observation for a state is missing
  { int any = 0;
    for (i=0; i<N; i++ )
    { double logp_delta;
      int st = LRDelModel_State_Decode(i);
      if( (st>-1) && (prev = history[ st ]) )
      { any = 1;
        for(j=0;j<nobs;j++)
        { logp_delta = Eval_Velocity_Likelihood_Log2( vel_dists,
                                                      prev->data,
                                                      obs[j].data,
                                                      (i%3)!=0 );
          max_logp_delta = MAX( max_logp_delta, logp_delta );
        }
      }
    }
    if( !any )
      max_logp_delta = 0;
  }

#ifdef DEBUG_LRDELMODEL_COMPUTE_EMISSIONS_FOR_TWO_CLASSES_W_HISTORY_LOG2
  debug("  j st   shp      vel\n"
        " -- -- -------  ------\n");
#endif
  for(i=0;i<N;i++)
  { real *row = E + i*nobs;
    real shp,vel;
    int state = (i%3)!=0;
    int st = LRDelModel_State_Decode(i); 
    if( st > -1 && (prev = history[ st ]) ) 
    { for(j=0;j<nobs;j++)
      { shp = Eval_Likelihood_Log2( shp_dists, obs[j].data, state );
        vel = Eval_Velocity_Likelihood_Log2( vel_dists,
                                              prev->data,
                                              obs[j].data,
                                              state );
        row[j] = shp + vel;
#ifdef DEBUG_LRDELMODEL_COMPUTE_EMISSIONS_FOR_TWO_CLASSES_W_HISTORY_LOG2
        debug(" %2d %2d % 5.3f % 5.3f\n",j,st,shp,vel);
#endif
      }
    } else // missing a previous
    { for(j=0;j<nobs;j++)
      { shp = Eval_Likelihood_Log2( shp_dists, obs[j].data, state );
        vel = max_logp_delta;
        row[j] = shp + vel;
#ifdef DEBUG_LRDELMODEL_COMPUTE_EMISSIONS_FOR_TWO_CLASSES_W_HISTORY_LOG2
        debug(" %2d %2d % 5.3f % 5.3f * \n",j,st,shp,vel);
#endif
      }
    }
  }
}

void LRDelModel_Compute_Emissions_For_Distinct_Whiskers_Log2( real *E, int nwhisk, Measurements *obs, int nobs, Distributions *shp_dists ) 
{ int N = 3 * nwhisk + 1;
  int i,j, iwhisk;
  // Assumes the following encoding of conditionals for the distributions:
  //   Junk     N%3==0         -> translates to state 0
  //   Whiskers N%3!=0         -> translates to state 1..nwhisk (inclusive)
  
  // first do junk
  for(j=0;j<nobs;j++)
  { real logp = Eval_Likelihood_Log2( shp_dists, obs[j].data, 0 );  
    for(i=0;i<N;i+=3)  
      E[i*nobs+j] = logp;
  }

  // now whiskers
  for(i=1,iwhisk=1;i<N;i+=3,iwhisk++)
  { real *row = E + i*nobs;
    for(j=0;j<nobs;j++)
    { double log2p = Eval_Likelihood_Log2( shp_dists, obs[j].data, iwhisk );
      row[j] = log2p;
      row[j+1] = log2p;
    }
  }
}

int LRDelModel_State_Decode( int s )
{ int sm = s%3;
  return (sm==1) ? (s-1)/3 : -1 ;// decode viterbi state to whisker label {-1:junk, 0..n:whiskers}  
}
