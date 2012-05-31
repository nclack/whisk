#include "config.h"
#include "common.h"
#include "image_lib.h"
#include "seq.h"
#include "ffmpeg_adapt.h"
#include "adjust_scan_bias.h"
#include "error.h"
#include <string.h>

#define ENDL "\n"
#if 1
#define REPORT(expr) debug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#else
#define REPORT(expr)
#endif
#define TRY(expr)       if(!(expr)) {REPORT(expr); goto Error;}
#define FAIL                        {REPORT("Execution should not reach here."); goto Error;}
#define SILENTTRY(expr) if(!(expr)) {              goto Error;}
#define HERE debug("%s(%d): HERE"ENDL, __FILE__,__LINE__)

//
// UTILITIES
//
static int max_uint8( Image *s )
{ int m = 0;
  unsigned int size = (s->width) * (s->height);
  uint8 *p;
  for( p = s->array; p < ((s->array)+size); p++ )
    m = MAX(m,*p);
  return m;
}

static int min_uint8( Image *s )
{ int m = 255;
  unsigned int size = (s->width) * (s->height);
  uint8 *p;
  for( p = s->array; p < ((s->array)+size); p++ )
    m = MIN(m,*p);
  return m;
}

static double mean_u8(unsigned char* p, int n)
{ double acc = 0.0;
  unsigned char *v;
  for(v=p;v<p+n;++v)
    acc += *v;
  return acc/(double)n;
}

static int  invert_uint8( Image *s )
// Ivert intensity so bar is bright
// returns 1 on failure, 0 otherwise
{
  if( s->kind != GREY8 )
  { error( "Only GREY8 images currently supported.\n" );
    goto error;
  }
  { unsigned int size = (s->width) * (s->height);
    uint8 *p;
    for( p = s->array; p < ((s->array)+size); p++ )
      (*p) = 255 - (*p);
  }
  return 0;
error:
  Free_Image(s);
  return 1;
}

static Image *transpose_copy_uint8( Image *s )
  /* not optimized */
{ Image *out;
  int x,y;
  if( s->kind != GREY8 )
  { error( "Only GREY8 images currently supported.\n" );
    goto error;
  }
  out = Make_Image( s->kind, s->height, s->width);
  for( x=0; x< (s->width); x++)
    for( y=0; y< (s->height); y++)
        *IMAGE_PIXEL_8( out,y,x,0 ) = *IMAGE_PIXEL_8( s,x,y,0 );
  return out;
error:
  return (NULL);
}

static unsigned int Seq_Get_Depth( SeqReader* s)
{ return (unsigned int)s->nframes; }

static unsigned int Stack_Get_Depth( Stack* s)
{ return (unsigned int)s->depth; }

static int Is_Tiff(const char *path)
{ TIFF *r = Open_Tiff((char*)path,"r");
  if(r)
  { Close_Tiff(r);
    return 1;
  }
  return 0;
}

static int Is_Seq(const char *path)
{ SeqReader *r = Seq_Open(path);
  if(r)
  { Seq_Close(r);
    return 1;
  }
  return 0;
}

static int Is_FFMPEG(const char *path)
{ void *r;
  r = FFMPEG_Open(path);
  if(r)
  { FFMPEG_Close(r);
    return 1;
  }
  return 0;
}


//
// INTERNAL INTERFACE
//
typedef void*        (*pf_opener)( const char* ); 
typedef void         (*pf_closer)( void* );       
typedef Image*       (*pf_fetch) ( void*, int );       
typedef unsigned int (*pf_get_nframes) (void* );
typedef int          (*pf_is_valid)(const char*);

typedef enum _kind_t
{ K_TIFF= 0,
  SEQ,
  FFMPEG,
  MAX_KIND,
  UNKNOWN
} kind_t;

static pf_is_valid is_valid_[] =
{ Is_Tiff,
  Is_Seq,
  Is_FFMPEG
};

static pf_opener open_[] =
{ Read_Stack,
  Seq_Open,
  FFMPEG_Open
};

static pf_closer close_[] = 
{ Free_Stack,
  Seq_Close,
  FFMPEG_Close
};

static pf_fetch get_[] = 
{ Select_Plane,
  Seq_Read_Image_Static_Storage,
  FFMPEG_Fetch
};

static pf_get_nframes nframes_[] =
{ Stack_Get_Depth,
  Seq_Get_Depth,
  FFMPEG_Frame_Count
};

static const char* etif[] = {".tif",".tiff",NULL};
static const char* eseq[] = {".seq",NULL};
static const char* effmpeg[] = {NULL};

static const char** extensions_[] =
{ etif,eseq,effmpeg,NULL
};

static int is_valid_kind(kind_t k)
#ifdef HAVE_FFMPEG
{return k<MAX_KIND;}
#else
{return k<MAX_KIND && k!=FFMPEG;}
#endif

kind_t guess_format(const char *path)
{ int i;
  char *ext = strrchr(path,'.');
  if(ext==NULL)
    goto Error;

  for(i=0;i<MAX_KIND;i++)
  { const char **s;
    for(s=extensions_[i];*s;++s)
      if(strcmp(ext,*s)==0)
        return i;
  }
Error:
  return UNKNOWN; // default if nothing found
}

//
//  EXTERNAL INTERFACE
//

typedef struct _video_t
{         kind_t  kind;
           int  valid_stats;
        double  hgain, 
                vgain, 
                hstat, 
                vstat; 
           int  mn,
                mx;
  unsigned int  nframes;
          void *fp;
} video_t;

video_t* video_open(const char *path)
{ video_t *self = 0;
  kind_t k = guess_format(path);
  if(!is_valid_kind(k))
    k = FFMPEG;                          // Try FFMPEG against unknown formats
  TRY(is_valid_kind(k));                 // Double-check in case FFMPEG is invalid.
  TRY(self=malloc(sizeof(*self)));
  memset(self,0,sizeof(*self));
  self->kind = k;
  TRY(self->fp=open_[k](path));
  self->nframes=nframes_[k](self->fp);
  return self;
Error:
  if(self) free(self);
  return NULL;
}

void video_close(video_t **self_)
{ video_t *self = *self_;
  if(self)
  { kind_t k = self->kind;
    TRY(is_valid_kind(k));
    if(self->fp)
      close_[k](self->fp);
  }
Error:
  *self_=NULL;
}

unsigned int video_frame_count(video_t *self)
{ return self->nframes; }

Image* video_get(video_t *self, unsigned int iframe, int apply_line_bias_correction)
{ Image *im;
  kind_t k = self->kind;
  TRY( is_valid_kind(k));
  SILENTTRY( iframe<self->nframes);
  TRY( im=get_[k](self->fp,iframe));
  im = Copy_Image(im);
  if(apply_line_bias_correction)
  { if(!self->valid_stats)
      TRY( video_compute_stats(self,20));

    if(self->hstat > self->vstat)
      image_adjust_scan_bias_h(im,self->hgain);
    else
      image_adjust_scan_bias_v(im,self->vgain);
  }
  return im;
Error:
  return NULL;
}

/// \returns 1 on success, 0 otherwise
int video_compute_stats (video_t  *self, int at_most_nframes)
{ int i,step;
  double mean;
  double hgain, vgain, hstat, vstat;
  int mn = 255,mx=0;
  kind_t k;
  Image *im;

  step = self->nframes/MIN(at_most_nframes,self->nframes);
  TRY(is_valid_kind(k=self->kind));
  TRY(im=get_[k](self->fp,0));
  mean = mean_u8(im->array, im->width * im->height);

  incremental_estimate_scan_bias_v(NULL,0,NULL); // Initialize              // Internally, there is a statically alocated accumulator
  incremental_estimate_scan_bias_h(NULL,0,NULL);                            // that keeps track of the statistics.
  for(i=0; i<self->nframes; i+=step)                                        // First,  the accumulator is reset by passing NULLS
  { int t;                                                                  // Second, image statistics are computed incrementally
    TRY(im = get_[k](self->fp,i));
    hgain = incremental_estimate_scan_bias_h(im,mean,&hstat); // Collect
    vgain = incremental_estimate_scan_bias_v(im,mean,&vstat); 
    t = min_uint8(im);
    mn = MIN(mn, t);
    t = max_uint8(im);
    mx = MAX(mx, t);
  }
  self->hgain = hgain;
  self->vgain = vgain;
  self->hstat = hstat;
  self->vstat = vstat;
  self->mn    = mn;
  self->mx    = mx;
  self->valid_stats = 1;
  return 1;
Error:
  self->valid_stats = 0;
  return 0;
}


int is_video(const char *path)
{ kind_t k = guess_format(path);
  if(k==UNKNOWN)
    k=FFMPEG;
  return is_valid_[k](path);
}

/*
 * VID2TIF
 */
#ifdef VID_TO_TIF
char *Spec[] = {"<infile:Video> <outfile:TIFF> [split (h <x:int>|v <y:int>)]",NULL};
int main(int argc, char*argv[])
{ int      ecode=0,
               i;
  video_t     *v=0;
  TIFF        *t=0;
  Process_Arguments(argc,argv,Spec,0);
  TRY(v=video_open(Get_String_Arg("infile")));
  TRY(t=Open_Tiff(Get_String_Arg("outfile"),"w"));
  for(i=0;i<video_frame_count(v);++i)
  { Image *im;
    TRY(im=video_get(v,i,0));
    Write_Tiff(t,im);
    Free_Image(im);
  }
Finalize:
  if(t) Close_Tiff(t);
  if(v) video_close(&v);
  return ecode;
Error:  
  ecode = 1;
  goto Finalize;
}
#endif

/*
 * VIDSPLIT
 */
//#define VIDSPLIT
#ifdef VIDSPLIT
char *Spec[] = {"<infile:Video> <outfile1:TIFF> <outfile2:TIFF> (-x <x:int>|-y <y:int>)",NULL};

int hsplit(Image *in, Image **out1, Image **out2, int x)
{ unsigned i,j;
  const unsigned w = in->width,
                 h = in->height;
  TRY( 0<x && x<w );
  TRY(out1[0]=Make_Image(in->kind,x,h));
  TRY(out2[0]=Make_Image(in->kind,w-x,h));
  for(j=0;j<in->height;++j)
  { for(i=0;i<w;++i)
    { if(i<x)
        out1[0]->array[j*x+i]=in->array[j*w+i];
      else
        out2[0]->array[j*(w-x)+(i-x)]=in->array[j*w+i];
    }
  }
  return 1;
Error:
  if(out1[0]) Free_Image(out1[0]);
  if(out2[0]) Free_Image(out2[0]);
  return 0;
}

int vsplit(Image *in, Image **out1, Image **out2, int y)
{ const unsigned h = in->height,
                 w = in->width;
  TRY( 0<y && y<h );
  TRY(out1[0]=Make_Image(in->kind,w,y));
  TRY(out2[0]=Make_Image(in->kind,w,h-y));
  memcpy(out1[0]->array,in->array,w*y);
  memcpy(out2[0]->array,in->array+y*w,w*(h-y));  
  return 1;
Error:
  if(out1[0]) Free_Image(out1[0]);
  if(out2[0]) Free_Image(out2[0]);
  return 0;
}
int main(int argc, char*argv[])
{ int      ecode=0,
               i;
  video_t     *v=0;
  TIFF        *t1,*t2=0;
  Process_Arguments(argc,argv,Spec,0);
  TRY(v=video_open(Get_String_Arg("infile")));
  TRY(t1=Open_Tiff(Get_String_Arg("outfile1"),"w"));
  TRY(t2=Open_Tiff(Get_String_Arg("outfile2"),"w"));
  for(i=0;i<video_frame_count(v);++i)
  { Image *im=0,*out1=0,*out2=0;
    TRY(im=video_get(v,i,0));
    if(Is_Arg_Matched("x"))
    { TRY(hsplit(im,&out1,&out2,Get_Int_Arg("x")));
    } else if(Is_Arg_Matched("y"))
    { TRY(vsplit(im,&out1,&out2,Get_Int_Arg("y")));
    } else
    { FAIL;
    }
    Write_Tiff(t1,out1);
    Write_Tiff(t2,out2);
    Free_Image(im);
    Free_Image(out1);
    Free_Image(out2);
  }
Finalize:
  if(t1) Close_Tiff(t1);
  if(t2) Close_Tiff(t2);
  if(v) video_close(&v);
  return ecode;
Error:  
  ecode = 1;
  goto Finalize;
}
#endif