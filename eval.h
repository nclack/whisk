/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef H_EVAL
#define H_EVAL

#include "compat.h"
#include "aip.h"

typedef struct _Range {double min; double max; double step;} Range;

typedef struct _Array
{ void *data; 
  int *strides_bytes;
  int *strides_px;
  int *shape;  
  int ndim;
} Array;

void scale( point* pbuf, int n, float sc);
void rotate( point *pbuf, int n, float angle);
void translate( point* pbuf, int n, point ori);

void Read_Range( FILE *fp, Range *r );
void Write_Range( FILE *fp, Range *r );
int  Is_Same_Range( Range *a, Range *b );

Array *Make_Array( int *shape , int ndim, int bytesperpixel );
Array *Read_Array( FILE *fp );
void Write_Array(FILE *fp, Array *a);
void Free_Array( Array *a);

void Sum_Pixel_Overlap( float *xy, int n, float gain, float *grid, int *strides );
void Render_Line_Detector( float offset, 
                           float length, 
                           float angle, 
                           float width,
                           point anchor,
                           float *image, int *strides  );

inline float *Get_Line_Detector( Array *bank, int iangle, int iwidth, int ioffset );
Array *Build_Line_Detectors( Range off, Range ang, Range wid, float length, int supportsize );
Array *Build_Curved_Line_Detectors( Range off, 
                                    Range wid, 
                                    Range ang, 
                                    float length, 
                                    int supportsize );


Array *Build_Harmonic_Line_Detectors( Range off, 
                                      Range wid, 
                                      Range ang, 
                                      float length, 
                                      int supportsize );

inline float *Get_Half_Space_Detector( Array *bank, int iangle, int iwidth, int ioffset );
Array *Build_Half_Space_Detectors( Range off, Range ang, Range wid, float length, int supportsize );
#endif // H_EVAL
