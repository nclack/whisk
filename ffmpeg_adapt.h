/* 
 * Author: Nathan Clack
 *   Date: May 31, 2010
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
/*
 * Code here is largely based on Michael Meeuwisse's example code (copyright
 * and license below).  The only changes I've made have been to rename some
 * things.
 *
 */
/* 
 * (C) Copyright 2010 Michael Meeuwisse
 *
 * ffmpeg_test is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ffmpeg_test is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ffmpeg_test. If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#ifdef HAVE_FFMPEG

#ifdef _MSC_VER
#define inline _inline
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#endif

//--- WHISK INTERFACE
//--- These function satisfy the abstract interface required by whisk.c's
//    load() function.
//
//--- NOTES
//
//    <FFMPEG_Fetch> 
//      appears to allow random access, but it doesn't in this case.  Fetch
//      will cache the last retrieved frame.  If iframe refers to the cached
//      frame,  that frame is returned.  Otherwise, the next frame is
//      decompressed and  returned.                                                               
//                                                                              
//      Calling FFMPEG_Fetch may invalidate any previous returned Images.       
//                                                                              
//      Not thread safe.                                                        

#include "image_lib.h"

SHARED_EXPORT         void *FFMPEG_Open       (const char* filename);      
SHARED_EXPORT         void  FFMPEG_Close      (void *context);             
SHARED_EXPORT        Image *FFMPEG_Fetch      (void *context, int iframe); 
SHARED_EXPORT unsigned int  FFMPEG_Frame_Count(void*);

//--- UI2.PY interface

SHARED_EXPORT int FFMPEG_Get_Stack_Dimensions(char *filename, int *width, int *height, int *depth, int *kind);
SHARED_EXPORT int FFMPEG_Read_Stack_Into_Buffer(char *filename, unsigned char *buf); 
