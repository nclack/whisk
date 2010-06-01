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
