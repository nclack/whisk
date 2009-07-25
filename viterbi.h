#ifndef H_VITERBI
#define H_VITERBI

typedef double real;

typedef struct _ViterbiResult
{ real total;          // Probability of observation given model                                
  real prob;           // Probability of most likely state sequence given model and observations
  int  n;              // Most likely state sequence                                            
  int *sequence;
} ViterbiResult;

void Free_Viterbi_Result( ViterbiResult *self);

ViterbiResult *Forward_Viterbi( int  *sequence,         // size: nobs
                                int   nseq,             // number of observations (size of seqeunce)
                                real *start_prob,       // size: nstates
                                real *transition_prob,  // size: nstates*nstates, destintion state-major order
                                real *emmission_prob,   // size: nstates * nobs, state-major order (stride is nobs)
                                int nobs,               // the size of the set of observables ( need for stride )
                                int nstates );          // number of states (size of state alphabet)

// TODO: Logarithmic number system
// LNS can be ignored if P(O|Model) doesn't need to be computed
// Without LNS, can still get P(X|O,Model)
ViterbiResult *Forward_Viterbi_Log2(  int  *sequence,         // size: nobs
                                      int   nseq,             // number of observations (size of seqeunce)
                                      real *start_prob,       // size: nstates (log probs)
                                      real *transition_prob,  // size: nstates*nstates, destintion state-major order (log probs)
                                      real *emmission_prob,   // size: nstates * nobs, state-major order (stride is nobs) (log probs)
                                      int nobs,               // the size of the set of observables ( need for stride )
                                      int nstates );          // number of states (size of state alphabet)
#endif // H_VITERBI
