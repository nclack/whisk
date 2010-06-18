#include "compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>


#include "utilities.h"
#include "image_lib.h"
#include "trace.h"
#include "seq.h"

#include "bar.h"
#include "bar_io.h"
#include "merge.h"

#include "adjust_scan_bias.h"
#include "whisker_io.h"
#include "error.h"

#include "ffmpeg_adapt.h"

#include "parameters/param.h"

#undef  WRITE_BACKGROUND

#define PRINT_USAGE( object ) printf("\tUsage "#object": %5d\n", object##_Usage())

void check_usage(void)
{ PRINT_USAGE(Image);
  PRINT_USAGE(Stack);
}

int max_uint8( Image *s )
{ int m = 0;
  unsigned int size = (s->width) * (s->height);
  uint8 *p;
  for( p = s->array; p < ((s->array)+size); p++ )
    m = MAX(m,*p);
  return m;
}

int min_uint8( Image *s )
{ int m = 255;
  unsigned int size = (s->width) * (s->height);
  uint8 *p;
  for( p = s->array; p < ((s->array)+size); p++ )
    m = MIN(m,*p);
  return m;
}

int  invert_uint8( Image *s )
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

Image *transpose_copy_uint8( Image *s )
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

unsigned int Seq_Get_Depth( SeqReader* s)
{ return (unsigned int)s->nframes; }

unsigned int Stack_Get_Depth( Stack* s)
{ return (unsigned int)s->depth; }

typedef void*        (*pf_opener)( const char* ); 
typedef void         (*pf_closer)( void* );       
typedef Image*       (*pf_fetch) ( void*, int );       
typedef unsigned int (*pf_get_nframes) (void* );  

Image *load(char *path, int index, int *nframes)
{ // Abstract interface
  static pf_opener opener = NULL;
  static pf_closer closer = NULL;
  static pf_fetch  fetch  = NULL; // Assumes static storage
  static pf_get_nframes get_nframes = NULL;
  static int opened = 0;

  // stats
  static double hgain, vgain, hstat, vstat;
  static int mn = 255,mx=0;

  // file handle
  static void* fp = NULL;
  static unsigned int saved_nframes;
  
  if( !opened )
  { char *ext = strrchr( path, '.' ); 
    if(ext==NULL)
      error("Could not recognize the extension from the filename.\n\tFilename: %s\n",path);
    opened = 1;

    // Setup function calls according to file extension
    if( strcmp(ext,".tif")==0 || strcmp(ext,".tiff")==0 )
    { opener      = (pf_opener)      &Read_Stack;
      closer      = (pf_closer)      &Free_Stack;
      fetch       = (pf_fetch)       &Select_Plane;
      get_nframes = (pf_get_nframes) &Stack_Get_Depth;
    } else if( strcmp(ext,".seq")==0 )
    {
      opener      = (pf_opener)      &Seq_Open;
      closer      = (pf_closer)      &Seq_Close;
      fetch       = (pf_fetch)       &Seq_Read_Image_Static_Storage;
      get_nframes = (pf_get_nframes) &Seq_Get_Depth;
    } 
#ifdef HAVE_FFMPEG
    else if
      ( strcmp(ext,".mp4")==0 ||
        strcmp(ext,".mov")==0 || 
        strcmp(ext,".avi")==0 || 
        strcmp(ext,".mpg")==0 || 
        strcmp(ext,".mp4")==0
      )
    {
      opener      = (pf_opener)      &FFMPEG_Open;
      closer      = (pf_closer)      &FFMPEG_Close;
      fetch       = (pf_fetch)       &FFMPEG_Fetch;
      get_nframes = (pf_get_nframes) &FFMPEG_Frame_Count;
    }  
#endif
    else
    { 
#ifdef HAVE_FFMPEG
      warning("Didn't recognize the video file's extension.\n"
              "Attempting to use FFMPEG.\n");
      opener      = (pf_opener)      &FFMPEG_Open;
      closer      = (pf_closer)      &FFMPEG_Close;
      fetch       = (pf_fetch)       &FFMPEG_Fetch;
      get_nframes = (pf_get_nframes) &FFMPEG_Frame_Count;
#else
      error("Didn't recognize the file extension.\n\tFilename: %s\n",path); 
#endif
    }
    if(opener==NULL)
      error("Could not recognize the file's type by it's extension.  Got: %s.\n",ext);

    // Use abstract file interface to open file and init
    fp = (*opener)(path);
    if( !fp )
      error( "Couldn't open file %s", path );
      
    saved_nframes = (*get_nframes)(fp);
    if( nframes )
      *nframes = saved_nframes;

    // estimate scan bias adjustment and image stats
    { int i,a,n = MIN( 20, saved_nframes ); //use a subset of at most 20 frames - saves some time
      int step = saved_nframes/n;
      double mean = 0.0;
      Image *image = (*fetch)(fp,0);

      //Compute mean from first frame
      a = image->width * image->height;
      { uint8 *p = image->array;
        for( i=0; i<a; i++ )
          mean += p[i];
        mean /= (double) a;
      }

      incremental_estimate_scan_bias_v(NULL,0,NULL); // Initialize              // Internally, there is a statically alocated accumulator
      incremental_estimate_scan_bias_h(NULL,0,NULL);                            // that keeps track of the statistics.
      for(i=0; i<saved_nframes; i+=step)                                        // First,  the accumulator is reset by passing NULLS
      { int t;                                                                  // Second, image statistics are computed incrementally
        image = (*fetch)(fp,i);
        hgain = incremental_estimate_scan_bias_h(image,mean,&hstat); // Collect
        vgain = incremental_estimate_scan_bias_v(image,mean,&vstat); 
        t = min_uint8(image);
        mn = MIN(mn, t);
        t = max_uint8(image);
        mx = MAX(mx, t);
      }
    }
  } else if (index==-1) // opened and index == -1 ==> close
  { (*closer)(fp);
    fp = NULL;
    opened = 0;
    return NULL;
  }

  // At this point the file's been opened and 
  // the relevant correction factors measured
  // so grab the frame of interest, correct 
  // and return!
  
  { Image *image = Copy_Image( (*fetch)(fp,index) );
    if(hstat > vstat)
      image_adjust_scan_bias_h(image,hgain);
    else
      image_adjust_scan_bias_v(image,vgain);
    //Scale_Image(image,0, 255.0/( (float)(mx-mn) ) , mn );
    return image;
  }
}

/*
 * MAIN
 */
static char *Spec[] = { "<movie:string> <prefix:string> [--no-whisk] [--no-bar]", NULL };
int main(int argc, char *argv[])
{ char  *whisker_file_name, *bar_file_name, *prefix;
  size_t prefix_len;
  FILE  *fp;
  Image *bg, *image;
  int    i,depth;

  char * movie;

  /* Process Arguments */
  Process_Arguments(argc,argv,Spec,0);
  if(Load_Params_File("./parameters/default.parameters"))
  { error("Could not load parameters.\n");
  }

  prefix = Get_String_Arg("prefix");
  prefix_len = strlen(prefix);
  { char *dot = strrchr(prefix,'.');  // Remove any file extension from the prefix
    if(dot) *dot = 0;
  }
  whisker_file_name = (char*) Guarded_Malloc( (prefix_len+32)*sizeof(char), "whisker file name");
  bar_file_name = (char*) Guarded_Malloc( (prefix_len+32)*sizeof(char), "bar file name");
  memset(whisker_file_name, 0, (prefix_len+32)*sizeof(char) );
  memset(bar_file_name ,    0, (prefix_len+32)*sizeof(char) );

  sprintf( whisker_file_name, "%s.whiskers", prefix );
  sprintf(  bar_file_name, "%s.bar", prefix );

  progress("Loading...\n"); fflush(stdout);
  movie = Get_String_Arg("movie");
  image = load(movie,0,&depth);

  progress("Done.\n");

  // No background subtraction (init to blank)
  { bg = Make_Image( image->kind, image->width, image->height );
    memset(bg->array, 0, bg->width * bg->height );
  }
  Free_Image( image );
  /*
   * Bar tracking
   */
  if( !Is_Arg_Matched("--no-bar") )
  { double x,y;
    BarFile *bfile = Bar_File_Open( bar_file_name, "w" );
    progress( "Finding bar positions\n" );
    for( i=0; i<depth; i++ )
    { progress_meter(i, 0, depth-1, 79, "Finding     post: [%5d/%5d]",i,depth);
      image = load(movie,i,NULL);
      invert_uint8( image );
      Compute_Bar_Location(   image,
                              &x,             // Output: x position
                              &y,             // Output: y position
                              15,             // Neighbor distance
                              15,             // minimum contour length
                              0,              // minimum intensity of interest
                              255,            // maximum intentity of interest
                              10.0,           // minimum radius of interest
                              30.0          );// maximum radius of interest
      Bar_File_Append_Bar( bfile, Bar_Static_Cast(i,x,y) );
      Free_Image(image);
    }
    Bar_File_Close( bfile );
  }

  /*
   * Trace whisker segments
   */
  if( !Is_Arg_Matched("--no-whisk") )
  { int           nTotalSegs = 0;
    Whisker_Seg   *wv;
    int wv_n; 
    WhiskerFile wfile = Whisker_File_Open(whisker_file_name,"whiskbin1","w");

    if( !wfile )
    { fprintf(stderr, "Warning: couldn't open %s for writing.", whisker_file_name);
    } else
    { //int step = (int) pow(10,round(log10(depth/100)));
      for( i=0; i<depth; i++ )
      //for( i=450; i<460; i++ )
      //for( i=0; i<depth; i+= step )
      { //int i=174;
        int k;
        image = load(movie,i,NULL);
        progress_meter(i, 0, depth, 79, "Finding segments: [%5d/%5d]",i,depth);
        wv = find_segments(i, image, bg, &wv_n);                                                // Thrashing heap
        k = Remove_Overlapping_Whiskers_One_Frame( wv, wv_n, 
                                                   image->width, image->height, 
                                                   2.0,    // scale down by this
                                                   2.0,    // distance threshold
                                                   0.5 );  // significant overlap fraction
        Whisker_File_Append_Segments(wfile, wv, k);
        Free_Whisker_Seg_Vec( wv, wv_n );
        Free_Image(image);
      }
      printf("\n");
      Whisker_File_Close(wfile);
    }
  }
  load(movie,-1,NULL); // Close (and free)

  Free_Image( bg );
  return 0;
}
