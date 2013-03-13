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
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>


#include "utilities.h"
#include "image_lib.h"
#include "video.h"
#include "trace.h"

#include "bar.h"
#include "bar_io.h"
#include "merge.h"

#include "whisker_io.h"
#include "error.h"

#include "parameters/param.h"

#undef  WRITE_BACKGROUND

#define ENDL "\n"
#if 1
#define REPORT(expr) debug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#else
#define REPORT(expr)
#endif
#define TRY(expr,lbl) if(!(expr)) {REPORT(expr); goto lbl;}

#define PRINT_USAGE( object ) printf("\tUsage "#object": %5d\n", object##_Usage())

void check_usage(void)
{ PRINT_USAGE(Image);
  PRINT_USAGE(Stack);
}

int  invert_uint8( Image *s )
// Invert intensity so bar is bright
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

/*
 * load()
 */

Image *load(char *path, int index, int *nframes)
{ 
  static video_t *v=NULL;
  Image *im=NULL;
  if(index>=0)
  { if(!v)       TRY(v=video_open(path),ErrorOpen);
    if(nframes) *nframes=video_frame_count(v);
    TRY(im=video_get(v,index,1),ErrorRead);
  } else
  { if(v) video_close(&v);
  }
  return im;
ErrorRead:
  video_close(&v);
  return NULL;
ErrorOpen:
  return NULL;
}

/*
 * MAIN
 */
static char *Spec[] = { "<movie:string> <prefix:string> [--no-bar] [--no-whisk]", NULL };
int main(int argc, char *argv[])
{ char  *whisker_file_name, *bar_file_name, *prefix;
  size_t prefix_len;
  FILE  *fp;
  Image *bg=0, *image=0;
  int    i,depth;

  char * movie;

  /* Process Arguments */
  Process_Arguments(argc,argv,Spec,0);
  { char* paramfile = "default.parameters";
    if(Load_Params_File("default.parameters"))
    { warning(
        "Could not load parameters from file: %s\n"
        "Writing %s\n"
        "\tTrying again\n",paramfile,paramfile);
      Print_Params_File(paramfile);
      if(Load_Params_File("default.parameters"))
        error("\tStill could not load parameters.\n");
    }
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
  TRY(image = load(movie,0,&depth),ErrorOpen);

  progress("Done.\n");

  // No background subtraction (init to blank)
  { bg = Make_Image( image->kind, image->width, image->height );
    memset(bg->array, 0, bg->width * bg->height );
  }
  Free_Image( image );

#if 1
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
#endif

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
      { int k;
        TRY(image=load(movie,i,NULL),ErrorRead);
        progress_meter(i, 0, depth, 79, "Finding segments: [%5d/%5d]",i,depth-1);
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
  if(bg) Free_Image( bg );
  return 0;
ErrorRead:
  load(movie,-1,NULL); // Close (and free)
  if(bg) Free_Image( bg );
  error("Could not read frame %d from %s"ENDL,i,movie);
  return 1;
ErrorOpen:
  error("Could not open %s"ENDL,movie);
  return 2;
}
