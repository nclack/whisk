/*
 * Code here is largely based on Michael Meeuwisse's example code (copyright
 * and license below).  The only changes I've made have been to rename some
 * things.
 *
 * Author: Nathan Clack and Mark Bolstadt
 *   Date: May 31, 2010
 */
/* 
 * (C) Copyright 2010 Michael Meeuwisse
 *
 * Adapted from avcodec_sample.0.5.0.c, license unknown
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

#include "ffmpeg_adapt.h"

#include <common.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

//---
#ifdef HAVE_FFMPEG
//#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
//#include <libswscale/swscale.h>
#include <avcodec.h>
#include <avformat.h>
#include <swscale.h>
//---
//  FORWARD DECLARATIONS
//

typedef struct _ffmpeg_video 
{
   AVFormatContext *pFormatCtx;
   AVCodecContext *pCtx;
   AVCodec *pCodec;
   AVFrame *pRaw;
   AVFrame *pDat;
   uint8_t *buffer,
           *rawimage;
   struct SwsContext *Sctx;
   int videoStream, width, height, format;
} ffmpeg_video;

ffmpeg_video *ffmpeg_video_init(char          *file, int format);

int           ffmpeg_video_bytes_per_frame(   void );
long          ffmpeg_video_bytes_all_frames(  void );

int           ffmpeg_video_next(ffmpeg_video  *cur);
ffmpeg_video *ffmpeg_video_quit(ffmpeg_video  *cur);

void ffmpeg_video_video_debug_ppm(ffmpeg_video *cur, char *file);

//---
//  IMPLEMENTATION
//
static int numBytes = 0;
static int numFrames = 0;

/* Init ffmpeg_video source 
 * file: path to open
 * format: PIX_FMT_GRAY8 or PIX_FMT_RGB24
 * Returns ffmpeg_video context on succes, NULL otherwise
 */
ffmpeg_video *ffmpeg_video_init( char *file, int format ) 
{
  int i = 0;

  ffmpeg_video *ret = malloc( sizeof( ffmpeg_video ) );
  memset( ret, 0, sizeof( ffmpeg_video ) );
  ret->format = format;

  /* Init ffmpeg */
  av_register_all();

  /* Open file, check usability */
  if( av_open_input_file( &ret->pFormatCtx, file, NULL, 0, NULL ) ||
      av_find_stream_info( ret->pFormatCtx ) < 0 )
    return ffmpeg_video_quit( ret );

  /* Find the first ffmpeg_video stream */
  ret->videoStream = -1;
  for( i = 0; i < ret->pFormatCtx->nb_streams; i++ )
    if( ret->pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO ) 
    {
      ret->videoStream = i;
      break;
    }

  if( ret->videoStream == -1 )
    return ffmpeg_video_quit( ret );

  /* Get context for codec, pin down target width/height, find codec */
  ret->pCtx = ret->pFormatCtx->streams[ret->videoStream]->codec;
  ret->width = ret->pCtx->width;
  ret->height = ret->pCtx->height;
  ret->pCodec = avcodec_find_decoder( ret->pCtx->codec_id );

  if( !ret->pCodec ||
      avcodec_open( ret->pCtx, ret->pCodec ) < 0 )
    return ffmpeg_video_quit( ret );

  /* Frame rate fix for some codecs */
  if( ret->pCtx->time_base.num > 1000 && ret->pCtx->time_base.den == 1 )
    ret->pCtx->time_base.den = 1000;

  /* Compute the total number of frames in the file */
  /* duration is in microsecs */
  numFrames = (int)(( ret->pFormatCtx->duration / (double)AV_TIME_BASE ) * ret->pCtx->time_base.den );

  /* Get framebuffers */
  ret->pRaw = avcodec_alloc_frame();
  ret->pDat = avcodec_alloc_frame();

  if( !ret->pRaw || !ret->pDat )
    return ffmpeg_video_quit( ret );

  /* Create data buffer */
  numBytes = avpicture_get_size( ret->format, 
      ret->pCtx->width, ret->pCtx->height );
  ret->buffer   = malloc( numBytes );
  ret->rawimage = malloc( ret->width * ret->height * 1/*Bpp*/ );
  assert(ret->buffer);
  assert(ret->rawimage);

  /* Init buffers */
  avpicture_fill( (AVPicture * ) ret->pDat, ret->buffer, ret->format, 
      ret->pCtx->width, ret->pCtx->height );

  /* Init scale & convert */
  ret->Sctx = sws_getContext( ret->pCtx->width, ret->pCtx->height, ret->pCtx->pix_fmt,
      ret->width, ret->height, ret->format, SWS_BICUBIC, NULL, NULL, NULL );

  if( !ret->Sctx )
    return ffmpeg_video_quit( ret );

  /* Give some info on stderr about the file & stream */
  //dump_format(ret->pFormatCtx, 0, file, 0);

  /* copy out raw data */
  { int i;
    for( i = 0; i < ret->height; i++ )
      memcpy(ret->rawimage+i*ret->width,
            ret->pDat->data[0] + i * ret->pDat->linesize[0],
            ret->width);
  }
  return ret;
}

int ffmpeg_video_bytes_per_frame( void )
{
  return numBytes;
}

long ffmpeg_video_bytes_all_frames( void )
{
  return numFrames * numBytes;
}

/* Parse next packet from cur video
 * Returns 0 on succes, -1 on read error,
 * -2 when not a video packet (ignore these) and -3 for invalid frames (skip these)
 */
int ffmpeg_video_next( ffmpeg_video *cur ) 
{
  AVPacket packet;
  int finished = 0;

  /* Can we read a frame? */
  if( av_read_frame( cur->pFormatCtx, &packet ) < 0 )
    return -1;

  /* Is it what we're trying to parse? */
  if( packet.stream_index != cur->videoStream ) 
  {
    av_free_packet( &packet );
    return -2;
  }

  /* Decode it! */
  avcodec_decode_video( cur->pCtx, cur->pRaw, &finished, packet.data, packet.size );

  /* Success? If not, drop packet. */
  if( !finished ) 
  {
    av_free_packet( &packet );
    return -3;
  }

  sws_scale( cur->Sctx, cur->pRaw->data, cur->pRaw->linesize, 
      0, cur->pCtx->height, cur->pDat->data, cur->pDat->linesize );
  av_free_packet( &packet );

  /* copy out raw data */
  { int i;
    for( i = 0; i < cur->height; i++ )
      memcpy(cur->rawimage+i*cur->width,
            cur->pDat->data[0] + i * cur->pDat->linesize[0],
            cur->width);
  }
  return 0;
}

// Returns the current frame on success.
// Panics on failure.
int ffmpeg_video_seek( ffmpeg_video *cur, int64_t iframe )
{ int sts;
  assert(iframe>=0 && iframe<numFrames);
  sts=av_seek_frame(cur->pFormatCtx,
                    cur->videoStream,
                    iframe*cur->pCtx->time_base.num/cur->pCtx->time_base.den,  /* convert to microsec */
                    //iframe*AV_TIME_BASE*cur->pCtx->time_base.num/cur->pCtx->time_base.den,  /* convert to microsec */
                    AVSEEK_FLAG_ANY);
  assert(sts>=0);
  assert(ffmpeg_video_next(cur)==0);
  return iframe;
}

/* Close & Free cur video context 
 * This function is also called on failed init, so check existence before de-init
 */
ffmpeg_video *ffmpeg_video_quit( ffmpeg_video *cur ) 
{
  if( cur->Sctx )
    sws_freeContext( cur->Sctx );

  if( cur->pRaw )
    av_free( cur->pRaw );
  if( cur->pDat )
    av_free( cur->pDat );

  if( cur->pCtx )
    avcodec_close( cur->pCtx );
  if( cur->pFormatCtx )
    av_close_input_file( cur->pFormatCtx );

  free( cur->buffer );
  free( cur );

  return NULL;
}

/* Output frame to file in PPM format */
void ffmpeg_video_debug_ppm( ffmpeg_video *cur, char *file ) 
{
  int i = 0;
  FILE *out = fopen( file, "wb" );

  if( !out )
    return;

  /* PPM header */
  fprintf( out, "P%d\n%d %d\n255\n", cur->format == PIX_FMT_GRAY8? 5: 6, 
      cur->width, cur->height );

  /* Spit out raw data */
  for( i = 0; i < cur->height; i++ )
    fwrite( cur->pDat->data[0] + i * cur->pDat->linesize[0], 1,
        cur->width * ( cur->format == PIX_FMT_GRAY8? 1: 3 ), out );

  fclose( out );
}


//--- Wrappers

#include "image_lib.h"

int _handle_open_status(char *filename, void *c)
{
  if(c==NULL)
  { warning("Could not open file: %s\n",filename);
    if(c) ffmpeg_video_quit(c); 
    return 0;
  }
  return 1;
}

SHARED_EXPORT void *FFMPEG_Open(const char* filename)
{ void *ctx = NULL;
  ctx = ffmpeg_video_init(filename,PIX_FMT_GRAY8);
  if(!_handle_open_status(filename,ctx))
    return NULL;
  return ctx; // NULL on error
}

SHARED_EXPORT void FFMPEG_Close(void *context)
{ if(context) ffmpeg_video_quit(context);
}
/*
typedef struct _Image
  { int      kind;
    int      width; 
    int      height;
    char    *text;
    uint8   *array;   // Array of pixel values lexicographically ordered on (y,x,c).
  } Image;            //    Pixel (0,0) is the lower left-hand corner of an image.
  */
void _handle_ffmpeg_video_next_error(int sts)
{ char prefix[] = "Decoding next frame:";
  switch(sts)
  { case 0:
      warning("%s Success\n",prefix); break;
    case -1:
      error("%s Read Error\n",prefix); break;
    case -2:
      warning("%s Not a video packet\n",prefix); break;
    case -3:
      warning("%s Invalid frame\n",prefix); break;
    default:
      error("Unrecognized status code during attempt to decode the next frame\n");
  };
}

SHARED_EXPORT Image *FFMPEG_Fetch(void *context, int iframe)
{ static int   last=0;
  static Image *cur=NULL;
  ffmpeg_video *v = context;

  assert(iframe>=0 && iframe<numFrames);                     // ensure iframe is in bounds

  if(cur==0)
  { cur = Guarded_Malloc(sizeof(Image), __FILE__);
    last = iframe;
    cur->kind = 1;
    cur->width  = v->width;
    cur->height = v->height;
    cur->text   = "\0";
    ffmpeg_video_seek(v,iframe);
  } else
  { if(iframe == last)                                       // nothing to do
    { return cur;
    } else if(iframe == last+1)                              // read in next frame
    { int sts;
      while((sts=ffmpeg_video_next(v))!=0)
          _handle_ffmpeg_video_next_error(sts);
      ++last;
    } else                                                   // must seek
    { last = ffmpeg_video_seek(v,iframe);
    }
  }
  cur->array  = v->rawimage;
  return cur;
}

SHARED_EXPORT unsigned int  FFMPEG_Frame_Count(void* ctx)
{ return numFrames; }

//--- UI2.PY interface

int FFMPEG_Get_Stack_Dimensions(char *filename, int *width, int *height, int *depth, int *kind)
{ ffmpeg_video *ctx = ffmpeg_video_init(filename,PIX_FMT_GRAY8); 
  if(!_handle_open_status(filename,ctx))
    return 0;
  *width = ctx->width;
  *height = ctx->height;
  *depth = numFrames;
  *kind = 1;
  if(ctx) ffmpeg_video_quit(ctx);
  return 1;
}

// This, unfortunantly, involves an unneccesary copy.
int FFMPEG_Read_Stack_Into_Buffer(char *filename, unsigned char *buf)
{ ffmpeg_video *ctx = ffmpeg_video_init(filename,PIX_FMT_GRAY8); 
  if(!_handle_open_status(filename,ctx))
    return 0;  
  { int planestride = ffmpeg_video_bytes_per_frame();
    int i;
    for(i=0;i<numFrames;++i)
    { int sts;
      while((sts=ffmpeg_video_next(ctx))!=0)
        _handle_ffmpeg_video_next_error(sts);
      memcpy(buf+i*planestride,ctx->buffer,planestride);
    }
  }
  if(ctx) ffmpeg_video_quit(ctx);
  return 1;
}


#else // HAVE_FFMPEG not defined

void _handle_ffmpeg_not_installed(void)
{ error("FFMPEG was not built into this package.\n");
}

SHARED_EXPORT void         *FFMPEG_Open       (const char* filename)     {_handle_ffmpeg_not_installed(); return 0;}
SHARED_EXPORT void          FFMPEG_Close      (void *context)            {_handle_ffmpeg_not_installed();}             
SHARED_EXPORT Image        *FFMPEG_Fetch      (void *context, int iframe){_handle_ffmpeg_not_installed(); return 0;}   
SHARED_EXPORT unsigned int  FFMPEG_Frame_Count(void *context)            {_handle_ffmpeg_not_installed(); return 0;}   

//--- UI2.PY interface
SHARED_EXPORT int FFMPEG_Get_Stack_Dimensions(char *filename, int *width, int *height, int *depth, int *kind)
{_handle_ffmpeg_not_installed(); return 0;}

SHARED_EXPORT int FFMPEG_Read_Stack_Into_Buffer(char *filename, unsigned char *buf)
{_handle_ffmpeg_not_installed(); return 0;}

#endif // if/else defined HAVE_FFMPEG
