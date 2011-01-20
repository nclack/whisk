/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : May 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include "image_lib.h"
SHARED_EXPORT
Image *Cast_As_Image( uint8 *array, int width, int height, int kind )
{ static Image im = {0,0,0,NULL,NULL};
  im.kind = kind;
  im.width = width;
  im.height = height;
  im.text = "\0";
  im.array = array;
  return &im;
}
