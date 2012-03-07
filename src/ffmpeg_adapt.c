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
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
//#include <avcodec.h>
//#include <avformat.h>
//#include <swscale.h>
//---
//
#define ENDL "\n"
#define HERE         printf("%s(%d): HERE"ENDL,__FILE__,__LINE__); 
#define REPORT(expr) printf("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#define TRY(expr)    do{if(!(expr)) {REPORT(expr); goto Error;}}while(0)
#define DIE          do{printf("%s(%d): Fatal error.  Aborting."ENDL,__FILE__,__LINE__); exit(-1);}while(0)
#define AVTRY(expr,msg) \
  do{                                                   \
    int v=(expr);                                       \
    if(v<0 && v!=AVERROR_EOF)                       \
    { char buf[1024];                                   \
      av_strerror(v,buf,sizeof(buf));                 \
      if(msg) \
        printf("%s(%d):"ENDL "%s"ENDL "%s"ENDL "FFMPEG: %s"ENDL,   \
            __FILE__,__LINE__,#expr,(char*)msg,buf);                 \
      else \
        printf("%s(%d):"ENDL "%s"ENDL "FFMPEG Error: %s"ENDL,   \
            __FILE__,__LINE__,#expr,buf);                 \
      goto Error;                                         \
    }                                                   \
  }while(0)

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
   int numBytes;
   int numFrames;
   Image currentImage;
   int last;
} ffmpeg_video;

void ffmpeg_video_video_debug_ppm(ffmpeg_video *cur, char *file);

//---
//  IMPLEMENTATION
//

static int is_one_time_inited = 0;

/* Init ffmpeg */
void maybeInit()
{ if(is_one_time_inited)
    return;
  avcodec_register_all();
  av_register_all();
  avformat_network_init();
  is_one_time_inited = 1;
}

/* Close & Free cur video context 
 * This function is also called on failed init, so check existence before de-init
 */
ffmpeg_video *ffmpeg_video_quit( ffmpeg_video *cur ) 
{ if(!cur) return;
  if( cur->Sctx ) sws_freeContext( cur->Sctx );

  if( cur->pRaw ) av_free( cur->pRaw );
  if( cur->pDat ) av_free( cur->pDat );

  if( cur->pCtx ) avcodec_close( cur->pCtx );
  if( cur->pFormatCtx ) av_close_input_file( cur->pFormatCtx );

  if(cur->rawimage) av_free(cur->rawimage);
  if(cur->buffer)   av_free(cur->buffer);
  free(cur);

  return NULL;
}

/* Init ffmpeg_video source 
 * file: path to open
 * format: PIX_FMT_GRAY8 or PIX_FMT_RGB24
 * Returns ffmpeg_video context on succes, NULL otherwise
 */
ffmpeg_video *ffmpeg_video_init(const char *fname, int format ) 
{
  int i = 0;
  ffmpeg_video *ret;
  maybeInit();
  
  TRY(ret=(ffmpeg_video*)malloc(sizeof(ffmpeg_video)));
  memset(ret,0,sizeof(ffmpeg_video));
  ret->format = format;

  /* Open file, check usability */
  AVTRY(avformat_open_input(&ret->pFormatCtx,fname,NULL,NULL),fname);
  AVTRY(avformat_find_stream_info(ret->pFormatCtx,NULL),"Cannot find stream information.");
  AVTRY(ret->videoStream=av_find_best_stream(ret->pFormatCtx,AVMEDIA_TYPE_VIDEO,-1,-1,&ret->pCodec,0),"Cannot find a video stream."); 
  ret->pCtx=ret->pFormatCtx->streams[ret->videoStream]->codec;
  ret->width  = ret->pCtx->width;
  ret->height = ret->pCtx->height;
  AVTRY(avcodec_open2(ret->pCtx,ret->pCodec,NULL),"Cannot open video decoder.");

  /* Frame rate fix for some codecs */
  if( ret->pCtx->time_base.num > 1000 && ret->pCtx->time_base.den == 1 )
    ret->pCtx->time_base.den = 1000;

  /* Compute the total number of frames in the file */
  /* duration is in microsecs */
  ret->numFrames = (int)(( ret->pFormatCtx->duration / (double)AV_TIME_BASE ) * ret->pCtx->time_base.den );

  /* Get framebuffers */
  TRY(ret->pRaw = avcodec_alloc_frame());
  TRY(ret->pDat = avcodec_alloc_frame());

  /* Create data buffer */
  ret->numBytes = avpicture_get_size( ret->format, ret->pCtx->width, ret->pCtx->height );
  TRY(ret->buffer   = (uint8_t*)av_malloc(ret->numBytes));
  TRY(ret->rawimage = (uint8_t*)av_malloc(ret->width*ret->height/* 1 Bpp*/));

  /* Init buffers */
  avpicture_fill( (AVPicture * ) ret->pDat, ret->buffer, ret->format, 
      ret->pCtx->width, ret->pCtx->height );

  /* Init scale & convert */
  TRY(ret->Sctx=sws_getContext(
        ret->pCtx->width,
        ret->pCtx->height,
        ret->pCtx->pix_fmt,
        ret->width,
        ret->height,
        ret->format,
        SWS_BICUBIC,NULL,NULL,NULL));

  /* Give some info on stderr about the file & stream */
  //dump_format(ret->pFormatCtx, 0, fname, 0);

  /* copy out raw data */
  { int i;
    for( i = 0; i < ret->height; i++ )
      memcpy(ret->rawimage+i*ret->width,
            ret->pDat->data[0] + i * ret->pDat->linesize[0],
            ret->width);
  }

  // setup currentImage
  ret->currentImage.kind   = 1;
  ret->currentImage.width  = ret->width;
  ret->currentImage.height = ret->height;
  ret->currentImage.text   = "\0";
  ret->currentImage.array  = ret->rawimage;

  ret->last = -1;
  return ret;
Error:
  ffmpeg_video_quit(ret);
  return NULL;
}

int ffmpeg_video_bytes_per_frame( ffmpeg_video* v )
{
  return v->numBytes;
}

/* Parse next packet from cur video
 * Returns 0 on success, -1 otherwise
 */
int ffmpeg_video_next( ffmpeg_video *cur, int target ) 
{
  AVPacket packet = {0};
  int finished = 0;
  do
  { finished=0;
    AVTRY(av_read_frame( cur->pFormatCtx, &packet ),NULL);   // !!NOTE: see docs on packet.convergence_duration for proper seeking        
    if( packet.stream_index != cur->videoStream ) /* Is it what we're trying to parse? */
    { av_free_packet( &packet );
      continue;
    }    
    AVTRY(avcodec_decode_video2( cur->pCtx, cur->pRaw, &finished, &packet ),NULL); 
    //printf("Packet - pts:%5d dts:%5d (%5d) - flag: %1d - finished: %3d - Frame pts:%5d %5d\n",packet.pts,packet.dts,target,packet.flags,finished,cur->pRaw->pts,cur->pRaw->best_effort_timestamp);
    av_free_packet( &packet );
    if(!finished)
      TRY(packet.pts!=AV_NOPTS_VALUE);    
  } while(!finished || cur->pRaw->best_effort_timestamp<target);

  sws_scale(cur->Sctx,              // sws context
            cur->pRaw->data,        // src slice
            cur->pRaw->linesize,    // src stride
            0,                      // src slice y
            cur->pCtx->height,      // src slice height
            cur->pDat->data,        // dst
            cur->pDat->linesize );  // dst stride

  /* copy out raw data */
  { int i;
    for( i = 0; i < cur->height; i++ )
      memcpy(cur->rawimage+i*cur->width,
            cur->pDat->data[0] + i * cur->pDat->linesize[0],
            cur->width);
  }
  return 0;
Error:
  av_free_packet( &packet );
  return -1;
}


// \returns current frame on success, otherwise -1
int ffmpeg_video_seek( ffmpeg_video *cur, int64_t iframe )
{ int64_t duration = cur->pFormatCtx->streams[cur->videoStream]->duration;
  int64_t ts = av_rescale(duration,iframe,cur->numFrames),
         tol = av_rescale(duration,1,2*cur->numFrames);
  TRY(iframe>=0 && iframe<cur->numFrames);

#if 0
  AVTRY(av_seek_frame(      cur->pFormatCtx, //format context
                            cur->videoStream,//stream id
                            ts,              //target timestamp
                            0),//AVSEEK_FLAG_ANY),//flags
    "Failed to seek.");
#else  
  AVTRY(avformat_seek_file( cur->pFormatCtx, //format context
                            cur->videoStream,//stream id
                            0,               //min timestamp
                            ts,              //target timestamp
                            ts,              //max timestamp
                            0),//AVSEEK_FLAG_ANY),//flags
    "Failed to seek.");
#endif
  avcodec_flush_buffers(cur->pCtx);
  
  TRY(ffmpeg_video_next(cur,iframe)==0);
  return iframe;
Error:
  return -1;
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

int _handle_open_status(const char *filename, void *c)
{
  if(c==NULL)
  { //warning("Could not open file: %s\n",filename);
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

SHARED_EXPORT Image *FFMPEG_Fetch(void *context, int iframe)
{ 
  ffmpeg_video *v = (ffmpeg_video*)context;
  TRY(iframe>=0 && iframe<v->numFrames);     // ensure iframe is in bounds
  if(iframe==v->last+1)
    TRY(ffmpeg_video_next(v,iframe)>=0);
  else
    TRY(ffmpeg_video_seek(v,iframe)>=0);
  v->last = iframe;
  v->currentImage.array  = v->rawimage;      // just in case the pointer changed...which it didn't
  return &v->currentImage;
Error:
  return NULL;
}

SHARED_EXPORT unsigned int  FFMPEG_Frame_Count(void* ctx)
{ return ((ffmpeg_video*)ctx)->numFrames; }

//--- UI2.PY interface

int FFMPEG_Get_Stack_Dimensions(char *filename, int *width, int *height, int *depth, int *kind)
{ ffmpeg_video *ctx = ffmpeg_video_init(filename,PIX_FMT_GRAY8); 
  if(!_handle_open_status(filename,ctx))
    return 0;
  *width = ctx->width;
  *height = ctx->height;
  *depth = ctx->numFrames;
  *kind = 1;
  if(ctx) ffmpeg_video_quit(ctx);
  return 1;
}

// This involves an unneccesary copy.
int FFMPEG_Read_Stack_Into_Buffer(char *filename, unsigned char *buf)
{ ffmpeg_video *ctx;
  TRY(ctx=ffmpeg_video_init(filename,PIX_FMT_GRAY8));  
  { int planestride = ffmpeg_video_bytes_per_frame(ctx);
    int i;
    for(i=0;i<ctx->numFrames;++i)
    { int sts;
      TRY(ffmpeg_video_next(ctx,i)==0);
      memcpy(buf+i*planestride,ctx->buffer,planestride);
    }
  }
  if(ctx) ffmpeg_video_quit(ctx);
  return 1;
Error:
  return 0;
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
