/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef H_LRMODEL

#include "traj.h"

int   LRModel_State_Count                                       ( int nwhisk );
real *LRModel_Alloc_Transitions                                 ( int nwhisk );
real *LRModel_Init_Uniform_Transitions                          ( real *T, int nwhisk );
void  LRModel_Estimate_Transitions                              ( real *T, int nwhisk, Measurements *table, int nrows );
void  LRModel_Log2_Transitions                                  ( real *T, int nwhisk, real baseline_log2 );
real *LRModel_Alloc_Starts                                      ( int nwhisk );
void  LRModel_Compute_Starts_For_Two_Classes_Log2               ( real *S, real *T, int nwhisk, Measurements *first, Distributions *shp_dists );
void  LRModel_Compute_Starts_For_Distinct_Whiskers_Log2         ( real *S, real *T, int nwhisk, Measurements *first, Distributions *shp_dists );
real *LRModel_Alloc_Emissions                                   ( int nwhisk, int nobs );
real *LRModel_Request_Static_Resizable_Emissions                ( int nwhisk, int nobs );
void  LRModel_Compute_Emissions_For_Two_Classes_Log2            ( real *E, int nwhisk, Measurements *obs, int nobs, Distributions *shp_dists );
void  LRModel_Compute_Emissions_For_Distinct_Whiskers_Log2      ( real *E, int nwhisk, Measurements *obs, int nobs, Distributions *shp_dists );
void  LRModel_Compute_Emissions_For_Two_Classes_W_History_Log2  ( real *E, int nwhisk, Measurements *obs, int nobs, Measurements_Reference *prev, Distributions *shp_dists, Distributions *vel_dists );
void  LRModel_Compute_Emissions_For_Two_Classes_W_Prev_And_Next_Log2
                                                                ( real *E, int nwhisk, Measurements *obs, int nobs, Measurements_Reference *prev, Measurements_Reference *next, Distributions *shp_dists, Distributions *vel_dists );
int   LRModel_State_Decode                                      ( int state );

#endif
