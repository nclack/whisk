/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef  _H_ADJUST_SCAN_BIAS
#define _H_ADJUST_SCAN_BIAS

#include "compat.h"
#include "image_lib.h"

SHARED_EXPORT double  incremental_estimate_scan_bias_h ( Image *image, double thresh, double *stat);
SHARED_EXPORT void    image_adjust_scan_bias_h         ( Image *image, double gain );
SHARED_EXPORT double  incremental_estimate_scan_bias_v ( Image *image, double thresh, double *stat);
SHARED_EXPORT void    image_adjust_scan_bias_v         ( Image *image, double gain );

SHARED_EXPORT double  estimate_scan_bias_h(Stack *movie, double mean_intensity, double *stat);
SHARED_EXPORT void    adjust_scan_bias_h( Stack *movie, double gain );
SHARED_EXPORT double  estimate_scan_bias_v(Stack *movie, double mean_intensity, double *stat);
SHARED_EXPORT void    adjust_scan_bias_v( Stack *movie, double gain );
SHARED_EXPORT void    adjust_scan_bias( Stack *movie );

#endif//_H_ADJUST_SCAN_BIAS
