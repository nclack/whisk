/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef H_DISTANCE
#define H_DISTANCE

#include "trace.h"

float whisker_distance                 ( Whisker_Seg *w0, Whisker_Seg *w1 );
float whisker_distance_time_independent( Whisker_Seg *w0, Whisker_Seg *w1 );

int    hit(   Whisker_Seg *q, Whisker_Seg *target, 
              int side, int tstart, int *intersection );
int    distance( Whisker_Seg *w0, Whisker_Seg **sv, float *results );

#endif
