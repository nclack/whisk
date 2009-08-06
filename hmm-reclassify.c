#include "compat.h"
#include <assert.h>
#include "error.h"
#include "utilities.h"
#include "traj.h"
#include "viterbi.h"

#define DEBUG_HMM_RECLASSIFY
#if 0
#endif

#define HMM_RECLASSIFY_DISTS_NBINS (32)

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
          T[state, state+delta] ++;           // aggregate
          state+=delta;
        }
        row--;
      }
    }
  }
  //norm
  { real sum = 0.0;
    real *s,*t; 
    s = t = T+N*N;
    while(t-- > T) sum += *t;
    while(s-- > T) *s /= sum;
  }
  return;
}

void LRModel_Log2_Transitions( real *T, int nwhisk, real baseline_log2 )
{ int N = nwhisk*2 + 1;
  real s,*t; 
  real thresh = pow(2.0,baseline);
  t = T+N*N;
  while(t-- > T) 
  { s = *t;
    if( s <= thresh )
      *t = baseline_log2;
    else
      *t = log2(s);
  }
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
  LRModel_Log2_Transitions(T,nwhisk);

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
      assert( table[i]->state == 0 || table[i]->state == 1 );
  }
#endif

  //
  // Now build distributions.  Notice that the velocities were computed
  // using the approximate/incomplete trajectories.
  //

  shp_dists = Build_Distributions( table, nrows, HMM_RECLASSIFY_DISTS_NBINS );
  vel_dists = Build_Velocity_Distributions( table, nrows, HMM_RECLASSIFY_DISTS_NBINS );

Err:
  free(T);
  Free_Distributions(vel_dists);
  Free_Distributions(shp_dists);
  Free_Measurements_Table(table);

  return 0;
}

#endif
