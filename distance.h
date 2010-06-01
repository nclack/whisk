#ifndef H_DISTANCE
#define H_DISTANCE

#include "trace.h"

float whisker_distance                 ( Whisker_Seg *w0, Whisker_Seg *w1 );
float whisker_distance_time_independent( Whisker_Seg *w0, Whisker_Seg *w1 );

int    hit(   Whisker_Seg *q, Whisker_Seg *target, 
              int side, int tstart, int *intersection );
int    distance( Whisker_Seg *w0, Whisker_Seg **sv, float *results );

#endif
