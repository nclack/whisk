/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef H_TRACE
#define H_TRACE

#include "common.h"
#include "image_lib.h"
#include "contour_lib.h"
#include "seed.h"
#include "eval.h"

#include "parameters/param.h"

//#define FRAME_DELTA 1

//#define HAT_RADIUS  1.5    //  Mexican-hat radius for whisker detection
//#define MIN_LEVEL   1      //  Level-set threshold for an object
//#define MIN_SIZE    20     //  Minimum # of pixels in an object

//#define TLEN  8
//#define OFFSET_STEP  .1   // pixels
//#define ANGLE_STEP   18.  // divisions of pi/4
//#define WIDTH_STEP   .2   // pixels [ngc: as of 2009-04-01 needs to be same as OFFSET_STEP]
//#define MIN_SIGNAL   5.0  //  per column , typically: sigmin = (2*TLEN+1)*MIN_SIGNAL
//#define MAX_DELTA_ANGLE  10.1 // degrees
//#define MAX_DELTA_WIDTH   6.0 // pixels
//#define MAX_DELTA_OFFSET  6.0 // pixels

//#define DUPLICATE_THRESHOLD 5.0

//#define HALF_SPACE_ASSYMETRY_THRESH 0.15     // between 0 and 1.  1 is completely insensitive to assymetry
//#define HALF_SPACE_TUNNELING_MAX_MOVES 10   // pixels.  This should be the largest size of an occluding area to cross

typedef struct  _Zone      //
  { int    cutoff;    //
    Image *mask;      //
  } Zone;             //

typedef struct  _Line_Params      //
  { float  offset;    //
    float  angle;     //
    float  width;     //
    float  score;     //
  } Line_Params;      //

typedef struct _Interval       //
  { double min;       //
    double max;       //
  } Interval;         //

typedef struct  _Whisker_Seg_Old      //
  { int    id;        //
    double width;     //
    int    beg;       //
    int    end;       //
    int    time;      // in units of planes
    float *track;     //
    float *scores;    //
  } Whisker_Seg_Old;  //

typedef struct _Whisker_Seg       //
{ int id;             //
  int time;           //
  int len;            //
  float *x;           //
  float *y;           //
  float *thick;       //
  float *scores;      //
} Whisker_Seg;        //

 SHARED_EXPORT  Whisker_Seg  *Make_Whisker_Seg                      ( int n );
 SHARED_EXPORT  void          Free_Whisker_Seg                      ( Whisker_Seg *w );
 SHARED_EXPORT  void          Free_Whisker_Seg_Data                 ( Whisker_Seg *w );
 SHARED_EXPORT  void          Free_Whisker_Seg_Vec                  ( Whisker_Seg *wv, int n );
 SHARED_EXPORT  void          Whisker_Seg_Sort_By_Id                ( Whisker_Seg *wv, int n );
 SHARED_EXPORT  void          Estimate_Image_Shape_From_Segments    ( Whisker_Seg* wv, int n, int *width, int *height );

 SHARED_EXPORT  Array *get_line_detector_bank   (Range *off, Range *wid, Range *ang);
 SHARED_EXPORT  int    read_line_detector_bank  (char *filename, Array **bank, Range *off, Range *wid, Range *ang );

 SHARED_EXPORT  Whisker_Seg  *find_segments                 (int iFrame, Image *image, Image *bg, int *nseg );
 SHARED_EXPORT  Image        *compute_background            (Stack *movie);
 SHARED_EXPORT  Zone         *compute_zone                  (Stack *movie);
 SHARED_EXPORT  float         eval_line                     (Line_Params *line, Image *image, int p);

 SHARED_EXPORT  int           adjust_line_walk              (Line_Params *line, Image *image, int *pp,
                                                             Interval *roff, Interval *rang, Interval *rwid);

 SHARED_EXPORT  int           adjust_line_start             (Line_Params *line, Image *image, int *pp,
                                                             Interval *roff, Interval *rang, Interval *rwid);

 SHARED_EXPORT  Whisker_Seg  *trace_whisker                 (Seed *s, Image *image);

#endif
