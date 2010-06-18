/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef H_HMM_RECLASSIFY
#define H_HMM_RECLASSIFY

typedef struct _measurements_reference
{ Measurements  *frame;     // points to head of a set of whisker segmens (e.g. those in a frame)
  Measurements **whiskers;  // map: whisker id's --> whisker segments
  int nframe;               // the number of whisker segements(rows) in the frame
  int nwhiskers;            // the number of whisker id's in the domain of the map.
} Measurements_Reference;


#endif  //H_HMM_RECLASSIFY 



