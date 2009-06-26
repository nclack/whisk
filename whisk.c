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
#include "trajectory.h"

#include "adjust_scan_bias.h"
#include "whisker_io.h"
#include "error.h"

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

    // Use abstract file interface to open file and init
    fp = (*opener)(path);
    if( !fp )
    { fprintf(stderr, "Couldn't open file %s", path );
      exit(1);
    }
    saved_nframes = (*get_nframes)(fp);
    if( nframes )
      *nframes = saved_nframes;

    // estimate scan bias adjustment and image stats
    { int i,a,n = MIN( 20, saved_nframes ); //use a subset of at most 20 frames
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

      incremental_estimate_scan_bias_v(NULL,0,NULL); // Initialize
      incremental_estimate_scan_bias_h(NULL,0,NULL);
      for(i=0; i<saved_nframes; i+=step)
      { int t;
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
  
  { Image *image = (*fetch)(fp,index);
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
static char *Spec[] = { "<movie:string> <prefix:string> [-t] [--no-whisk] [--no-bar] [--no-traj]", NULL };
int main(int argc, char *argv[])
{ //Stack *movie;
  //Zone  *mask;
  char  *whisker_file_name, *bar_file_name, *prefix;
  size_t prefix_len;
  FILE  *fp;
  Image *bg, *image;
  int    i,depth;
  //int    transpose = 0;

  char * movie;

  /* Process Arguments */
  Process_Arguments(argc,argv,Spec,0);

  prefix = Get_String_Arg("prefix");
  prefix_len = strlen(prefix);
  whisker_file_name = (char*) Guarded_Malloc( (prefix_len+32)*sizeof(char), "whisker file name");
  bar_file_name = (char*) Guarded_Malloc( (prefix_len+32)*sizeof(char), "bar file name");
  memset(whisker_file_name, 0, (prefix_len+32)*sizeof(char) );
  memset(bar_file_name ,    0, (prefix_len+32)*sizeof(char) );

  sprintf( whisker_file_name, "%s.whiskers", prefix );
  sprintf(  bar_file_name, "%s.bar", prefix );


  progress("Loading...\n"); fflush(stdout);
  movie = Get_String_Arg("movie");
  image = load(movie,0,&depth);

// FIXME  
/* // NOT SUPPORTED 
  if( Is_Arg_Matched("-t") )
    transpose = 1;
  if(transpose)
  { Image *timage = transpose_copy_uint8( image );
    Free_Stack(image);
    movie = tmovie;
  }
  */
  progress("Done.\n");

  // No background subtraction (init to blank)
  { bg = Make_Image( image->kind, image->width, image->height );
    memset(bg->array, 0, bg->width * bg->height );
  }

  /*
   * Bar tracking
   */
  if( !Is_Arg_Matched("--no-bar") )
  { double x,y;
    progress( "Finding bar positions\n" );
    fp = fopen( bar_file_name, "w" );
    for( i=0; i<depth; i++ )
    { progress( "Locating bar for frame %5d of %d.\n", i, depth);
      image = Copy_Image( load(movie,i,NULL) );
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
      fprintf(fp,"%d %g %g\n",i,x,y);
      Free_Image(image);
    }
    fclose( fp );
  }

  /*
   * Trace whisker segments
   */
  if( !Is_Arg_Matched("--no-whisk") )
  { int           nTotalSegs = 0;
    Whisker_Seg   *wv;// = (Whisker_Seg**) Guarded_Malloc( depth * sizeof(Whisker_Seg*), Program_Name());
    int wv_n; // = (int*) malloc ( depth * sizeof(int) );
    WhiskerFile wfile = Whisker_File_Open(whisker_file_name,"whiskbin1","w");

    //fp = fopen(whisker_file_name,"w");
    if( !wfile )
    { fprintf(stderr, "Warning: couldn't open %s for writing.", whisker_file_name);
    } else
    { //int step = (int) pow(10,round(log10(depth/100)));

      for( i=0; i<depth; i++ )
      //for( i=450; i<460; i++ )
      //for( i=0; i<depth; i+= step )
      { //int i=76;
        int k;
        image = Copy_Image( load(movie,i,NULL) );
        progress( "Finding segments for frame %5d of %d.\n", i, depth);
        wv = find_segments(i, image, bg, &wv_n);
        nTotalSegs += wv_n;
        Whisker_File_Append_Segments(wfile, wv, wv_n);
        Free_Whisker_Seg_Vec( wv, wv_n );
        Free_Image(image);
      }
      Whisker_File_Close(wfile);
    }
  }
  load(movie,-1,NULL); // Close (and free)

  Free_Image( bg );
  return 0;
}

