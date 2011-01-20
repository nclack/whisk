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
#include "seq.h"

#include "adjust_scan_bias.h"
#include "error.h"
#include "common.h"

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

//   
// Abstract reader
//
unsigned int Seq_Get_Depth( SeqReader* s)  //required by abstract reader
{ return (unsigned int)s->nframes; }

unsigned int Stack_Get_Depth( Stack* s)    //required by abstract reader
{ return (unsigned int)s->depth; }

typedef void*        (*pf_opener)( const char* ); 
typedef void         (*pf_closer)( void* );       
typedef Image*       (*pf_fetch) ( void*, int );       
typedef unsigned int (*pf_get_nframes) (void* );  

Image *load(char *path, int index, int *nframes)
{ 
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
      error( "Couldn't open file %s", path );
      
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
char *Spec[] = {"[-h|--help]",
                "|(",
                "    <source:string> <destination:string>",
                ")",
                NULL};
int main(int argc, char *argv[])
{ char  *source_file_name, *dest_file_name;
  FILE  *fp;
  Image *image;
  int    i,depth;

  /* Process Arguments */
  Process_Arguments(argc,argv,Spec,0);

  help( Is_Arg_Matched("-h") || Is_Arg_Matched("--help"),
      "----------------\n"
      "Adjust line bias\n"
      "----------------\n"
      "\n"
      "Some high speed cameras produce images with lines that\n"
      "appear alternating light and dark.  This utility corrects\n"
      "this systematic error writing the corrected images to a\n"
      "TIFF stack.\n"
      "\n"
      "Arguments:\n"
      "----------\n"
      "<source>\n"
      "\tInput video file.\n"
      "<destination>\n"
      "\tOutput. Only TIFF supported.\n"
      "\n" );

  dest_file_name = Get_String_Arg("destination");

  progress("Loading...\n"); fflush(stdout);
  source_file_name = Get_String_Arg("source");
  image = load(source_file_name,0,&depth);
  progress("Done.\n");

  { TIFF *fp = Open_Tiff( dest_file_name, "w" );
    Write_Tiff( fp, image );
    for( i=1; i<depth; i++ )
      Write_Tiff( fp, load(source_file_name,i,NULL) );
    Close_Tiff( fp );
  }

  load(source_file_name,-1,NULL); // Close (and free)
  return 0;
}
 
