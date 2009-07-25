#include <string.h> 
#include "utilities.h"
#include "image_lib.h"
#include "trace.h"
#include "seq.h"

#include "adjust_scan_bias.h"

#include "error.h"

Stack *transpose_copy_uint8( Stack *s )
  /* not optimized */
{ Stack *out;
  int x,y,z;
  if( s->kind != GREY8 )                        
  { error( "Only GREY8 images currently supported.\n" );
    goto error;
  }
  out = Make_Stack( s->kind, s->height, s->width, s->depth);
  for( x=0; x< (s->width); x++)
    for( y=0; y< (s->height); y++)
      for( z=0; z< (s->depth); z++)
        *STACK_PIXEL_8( out,y,x,z,0 ) = *STACK_PIXEL_8( s,x,y,z,0 );
  return out;
error:
  return (NULL);
}

Stack *load( char *path )
{ char *ext = strrchr( path, '.' );
  Stack *s;
  if( strcmp(ext,".tif")==0 || strcmp(ext,".tiff")==0 )
  { s = Read_Stack(path);
  } else if( strcmp(ext,".seq")==0 )
  { SeqReader *f = Seq_Open(path);
    if( !f )
    { fprintf(stderr, "Couldn't open file %s", path );
      exit(1);
    }
    s = Seq_Read_Stack(f);
    Seq_Close(f);
  }
  adjust_scan_bias(s);

  return s;
}

/*
 * MAIN
 */
static char *Spec[] = { "<movie:string> <output:string> [-t]", NULL };
int main(int argc, char *argv[])
{ Stack *movie, *out;
  FILE  *fp;
  int    i,j,iPlane,depth;
  int   area;

  Process_Arguments(argc,argv,Spec,0);

  progress("Loading...\n"); fflush(stdout);
  movie = load(Get_String_Arg("movie"));
  if( Is_Arg_Matched("-t") )
  { Stack *tmovie = transpose_copy_uint8( movie );
    Free_Stack(movie);
    movie = tmovie;
  }
  progress("Done.\n");

  Write_Stack( Get_String_Arg("output"), movie );

  Free_Stack( movie );
  return 0;
}

