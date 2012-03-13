#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include "traj.h"
void Whisker_Seg_Measure( Whisker_Seg *w, double *dest, int facex, int facey, char face_axis );

/** Fills an existing table with measurements */
Measurements *Whisker_Segments_Update_Measurements(
    Measurements* table, 
    Whisker_Seg *wv, int wvn, 
    int facex, int facey, char face_axis );

/** Allocates a new table and fills  with measurements */
Measurements *Whisker_Segments_Measure( 
    Whisker_Seg *wv, int wvn, 
    int facex, int facey, char face_axis );


#ifdef __cplusplus
}
#endif
