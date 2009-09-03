//
//
// -------
// Viterbi
// -------
//
// The viterbi algorithm is a dynamic programming algorithm for computing the
// most likely path through a markov model.  In particular, it handles 
// models where the states themselves have some weight.  
//
// The implimentation here is for the `forward` algorithm and assumes a
// probabilistic model and is pretty naive (unoptimized).  It takes about
// O(nstate^2) space and O(nstate) time.  There are very efficient
// implimentations that have been written for error correcting codes and
// compression, so if this becomes a bottleneck, it could be worth researching
// those.
//
#ifndef H_VITERBI
#define H_VITERBI

#include "compat.h"

#ifndef real
typedef double real;
#endif

typedef struct _ViterbiResult
{ real total;          // Probability of observation given model                                
  real prob;           // Probability of most likely state sequence given model and observations
  int  n;              // Most likely state sequence                                            
  int *sequence;
} ViterbiResult;

SHARED_EXPORT
void Free_Viterbi_Result( ViterbiResult *self);

SHARED_EXPORT
ViterbiResult *Forward_Viterbi( int  *sequence,         // size: nobs
                                int   nseq,             // number of observations (size of seqeunce)
                                real *start_prob,       // size: nstates
                                real *transition_prob,  // size: nstates*nstates, destintion state-major order
                                real *emmission_prob,   // size: nstates * nobs, state-major order (stride is nobs)
                                int nobs,               // the size of the set of observables ( need for stride )
                                int nstates );          // number of states (size of state alphabet)

// DONE: Logarithmic number system
// LNS can be ignored if P(O|Model) doesn't need to be computed
// Without LNS, can still get P(X|O,Model)
SHARED_EXPORT
ViterbiResult *Forward_Viterbi_Log2(  int  *sequence,         // size: nobs
                                      int   nseq,             // number of observations (size of seqeunce)
                                      real *start_prob,       // size: nstates (log probs)
                                      real *transition_prob,  // size: nstates*nstates, destintion state-major order (log probs)
                                      real *emmission_prob,   // size: nstates * nobs, state-major order (stride is nobs) (log probs)
                                      int nobs,               // the size of the set of observables ( need for stride )
                                      int nstates );          // number of states (size of state alphabet)
#endif // H_VITERBI
