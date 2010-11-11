/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include "trace.h"

int Remove_Overlapping_Whiskers_One_Frame( Whisker_Seg *wv, 
                                           int wv_n, 
                                           int w, 
                                           int h, 
                                           float scale, 
                                           float dist_thresh, 
                                           float overlap_thresh );
