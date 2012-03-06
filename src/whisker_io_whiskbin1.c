/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : May 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "trace.h"

#define WHISKBIN_MODE_READ  0
#define WHISKBIN_MODE_WRITE 1

void write_whiskbin1_header( FILE *file )
{ char type[] = "bwhiskbin1\0";
  fwrite( type, sizeof(type), 1, file );
}

void read_whiskbin1_header( FILE *file ) 
{ char type[] = "bwhiskbin1\0"; 
  fseek( file, sizeof(type), SEEK_SET ); // just seek to end of header
}

void write_whiskbin1_footer( FILE *file, int nwhiskers )
{ fwrite( &nwhiskers, sizeof(int), 1, file ); // append number of whiskers
  fseek( file, -sizeof(int), SEEK_CUR );
}

int is_file_whiskbin1( const char *filename )
{ char type[] = "bwhiskbin1\0";
  char buf[33];
  FILE *file = fopen(filename,"rb");
  long pos;

  if(file==NULL)
    { warning("Could not open file (%s) for reading.\n",filename);
      return 0;
    }
  pos = ftell(file); 
  fread(buf, sizeof(type), 1, file);
  fclose(file);
  if( strncmp( buf, type, sizeof(type) )==0 ) // if this is a whiskbin1 file, read position is not reset
    return 1;
  return 0;
}

FILE* open_whiskbin1( const char* filename, const char* mode )
{ FILE *fp;
  if( strncmp(mode,"w",1)==0 )
  { fp = fopen(filename,"w+b");
    if( fp == NULL )
    { warning("Could not open file (%s) for writing.\n");
      goto Err;
    }
    write_whiskbin1_header(fp);
    write_whiskbin1_footer(fp,0);
  } else if( strncmp(mode,"r",1)==0 )
  { fp = fopen(filename,"rb");
    read_whiskbin1_header(fp);
  } else {
    warning("Could not recognize mode (%s) for file (%s).\n",mode,filename);
    goto Err;
  }
  return fp;
Err:
  return NULL;
}

void close_whiskbin1( FILE *fp )
{ fclose(fp);
}

int peek_whiskbin1_footer( FILE *file )
{ int nwhiskers;
  long pos = ftell(file);
  fseek( file, -sizeof(int), SEEK_END);
  fread( &nwhiskers, sizeof(int), 1, file );
  fseek( file, pos, SEEK_SET);
  return nwhiskers;
}

void write_whiskbin1_segment( FILE *file, Whisker_Seg *w )
{ typedef struct {int id; int time; int len;} trunc_WSeg;
  if( w->len )
  { fwrite( (trunc_WSeg*) w, sizeof( trunc_WSeg), 1, file );
    fwrite( w->x     , sizeof(float) , (w->len), file );
    fwrite( w->y     , sizeof(float) , (w->len), file );
    fwrite( w->thick , sizeof(float) , (w->len), file );
    fwrite( w->scores, sizeof(float) , (w->len), file );
  }
}

// TODO: possible optimization - make a pass through the file to determine 
//       how much memory needs to be allocated, then allocate big blocks.
//       I think this might conflict with  how data gets freed later on though...
Whisker_Seg *read_segments_whiskbin1( FILE *file, int *n)
{ typedef struct {int id; int time; int len;} trunc_WSeg;
  Whisker_Seg *wv;
  int i;

  *n = peek_whiskbin1_footer(file); //read in number of whiskers
  wv = (Whisker_Seg*) Guarded_Malloc( sizeof(Whisker_Seg)*(*n), "read whisker segments - format: whiskbin1");

  for( i=0; i<(*n); i++ )
  { Whisker_Seg *w = wv + i;
    fread( w, sizeof( trunc_WSeg ), 1, file ); //populates id,time (a.k.a frame id),len
    w->x      = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (whiskbin1 format)" );
    w->y      = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (whiskbin1 format)" );
    w->thick  = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (whiskbin1 format)" );
    w->scores = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (whiskbin1 format)" );
    fread( w->x       , sizeof(float), w->len , file );
    fread( w->y       , sizeof(float), w->len , file );
    fread( w->thick   , sizeof(float), w->len , file );
    fread( w->scores  , sizeof(float), w->len , file );
  }
  return wv;
}

void append_segments_whiskbin1( FILE *file, Whisker_Seg *wv, int n )
{ int count,i;

  count = peek_whiskbin1_footer(file);
  for(i=0; i<n; i++)
    write_whiskbin1_segment( file, wv + i );
  write_whiskbin1_footer( file, count + n );
}
