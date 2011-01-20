/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : May 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include "compat.h"
#include <stdio.h>
#include <string.h>
#include "tiff_io.h"
#include "tiff_image.h"
#include "error.h"

static int is_lsm(const char *filename)
{ int n = strlen(filename);
  return strncmp(filename+n-3,"ext",3)==0;
}

SHARED_EXPORT
int Get_Number_Frames( char *filename )
{ int endian, depth=0;
  Tiff_Reader *tif = 0;
  tif = Open_Tiff_Reader( filename, &endian, is_lsm(filename) );
  while( !Advance_Tiff_Reader(tif) )
    depth += 1;
  Free_Tiff_Reader( tif );
  return depth;
}

SHARED_EXPORT
int Get_Stack_Dimensions_px( char *filename, 
                             int *width, 
                             int *height, 
                             int *depth, 
                             int *kind)
{ int endian,d=0; 
  Tiff_Reader *tif = 0;
  Tiff_IFD *ifd;
  Tiff_Image *tim;

  tif = Open_Tiff_Reader( filename, &endian, is_lsm(filename) );
  while( !Advance_Tiff_Reader(tif) )
    d += 1;
  Free_Tiff_Reader( tif );

  tif = Open_Tiff_Reader( filename, &endian, is_lsm(filename) );
  ifd = Read_Tiff_IFD( tif );
  tim = Extract_Image_From_IFD( ifd );

  if(!tim) 
  { warning("Could not extract first image\n");
    warning(Tiff_Image_Error());
    return 0;
  }

  (*depth) = d;
  (*width) = tim->width;
  (*height) = tim->height;
  (*kind) = tim->channels[0]->bytes_per_pixel;

  Free_Tiff_Reader( tif );
  return 1;
}

SHARED_EXPORT
int Compute_Sizeof_Stack_px( char *filename )
{ int width, height, depth, kind;
  Get_Stack_Dimensions_px( filename, &width, &height, &depth, &kind );
  return width*height*depth;
}

SHARED_EXPORT
int Compute_Sizeof_Stack_Bytes( char *filename )
{ int width, height, depth, kind;
  Get_Stack_Dimensions_px( filename, &width, &height, &depth, &kind );
  return width*height*depth*kind;
}

void print_usage(void)
{
  printf("Tiff Images: %5d\n",Tiff_Image_Usage());
  printf("Tiff IFD   : %5d\n",Tiff_IFD_Usage());
  printf("Tiff Reader: %5d\n",Tiff_Reader_Usage());
}


SHARED_EXPORT
int Read_Tiff_Stack_Into_Buffer( char *filename, void *buffer )
{ int endian; 
  Tiff_Reader *tif = 0;
  Tiff_IFD *ifd;
  Tiff_Image *tim;
  char *b = (char*) buffer;
  int nbytes;

  tif = Open_Tiff_Reader( filename, &endian, is_lsm(filename) );
  ifd = Read_Tiff_IFD( tif );
  tim = Extract_Image_From_IFD( ifd );

  nbytes = (tim->width)*(tim->height)*(tim->channels[0]->bytes_per_pixel);

  while (1)
  { memcpy( b, tim->channels[0]->plane, nbytes );
    b += nbytes;
    Free_Tiff_Image( tim );
    Free_Tiff_IFD( ifd );
    
    if( End_Of_Tiff( tif ) ) break;
    ifd = Read_Tiff_IFD( tif );
    tim = Extract_Image_From_IFD( ifd );
  }   
  if(tif) Free_Tiff_Reader ( tif );

  return 1;
}
