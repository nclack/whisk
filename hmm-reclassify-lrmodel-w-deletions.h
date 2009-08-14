#ifndef H_LRDELMODEL

#include "traj.h"
#include "viterbi.h"

real *LRDelModel_Alloc_Transitions                            ( int nwhisk );
real *LRDelModel_Init_Uniform_Transitions                     ( real *T, int nwhisk );
void  LRDelModel_Estimate_Transitions                         ( real *T, int nwhisk, Measurements *table, int nrows );
void  LRDelModel_Log2_Transitions                             ( real *T, int nwhisk, real baseline_log2 );
real *LRDelModel_Alloc_Starts                                 ( int nwhisk );            
void  LRDelModel_Compute_Starts_For_Two_Classes_Log2          ( real *S, int nwhisk, Measurements *first, Distributions *shp_dists );
void  LRDelModel_Compute_Starts_For_Distinct_Whiskers_Log2    ( real *S, int nwhisk, Measurements *first, Distributions *shp_dists );
real *LRDelModel_Alloc_Emissions                              ( int nwhisk, int nobs );
real *LRDelModel_Request_Static_Resizable_Emissions           ( int nwhisk, int nobs );
void  LRDelModel_Compute_Emissions_For_Two_Classes_Log2       ( real *E, int nwhisk, Measurements *obs, int nobs, Distributions *shp_dists );
void  LRDelModel_Compute_Emissions_For_Distinct_Whiskers_Log2 ( real *E, int nwhisk, Measurements *obs, int nobs, Distributions *shp_dists );

#endif
