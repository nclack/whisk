#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "trace.h"

int fskipline(FILE* fp, size_t *nch)
{ //return fgetln(fp,nch)!=NULL; // not available on windows :(
  size_t i = 0;
  int c;
  do { c = fgetc(fp); i++; } while( c!=EOF && c!='\n' );
  *nch = i;
  return (c=='\n');
}

void write_whisk1_header( FILE *file )
{ fprintf(file, "whisker1 (frame,id,time,n,x1,y1,thick1,score1...,xn,yn,thickn,scoren)\n");
}

int is_file_whisk1(const char* filename)
{ FILE *fp = fopen(filename,"r");
  char format[33],
       target_format[] = "whisker1";
  if(fp==NULL)
    { error("In is_file_whisk1, could not open file (%s) for reading.\n",filename);
      exit(1);
    }
  fscanf(fp,"%32s", format);
  fclose(fp);
  return strncmp(format,target_format,sizeof(target_format))==0;
}

FILE* open_whisk1(const char* filename, const char* mode)
{ FILE *fp;
  if( strncmp(mode,"w",1)==0 )
  { fp = fopen(filename,"w+");
    write_whisk1_header(fp);
  } else if( strncmp(mode,"r",1)==0 )
  { fp = fopen(filename,"r");
  } else {
    error("Could not recognize mode (%s) for file (%s).\n",mode,filename);
    return NULL;
  }
  return fp;
}

void close_whisk1(FILE* fp)
{ fclose(fp);
}

void write_whisk1_segment( FILE *file, Whisker_Seg *w )
{ int k;
  if( w->len )
  { fprintf( file, "%d,%d,%d,%d", w->time, w->id, w->time, w->len );
    for( k=0; k < w->len; k++ )
      fprintf( file, ",%g,%g,%g,%g", w->x[k],w->y[k],w->thick[k],w->scores[k] );
    fprintf(file,"\n");
  }
}

void append_segments_whisk1( FILE *fp, Whisker_Seg *wv, int n )
{ int i;
  for(i=0; i<n; i++)
    write_whisk1_segment(fp,wv+i);
}

#if 0
void write_segments_whisk1( FILE *file, Whisker_Seg **wv, int *sz, int nplanes )
{ int i,j;

  write_whisk1_header( file );
  for( i=0; i<nplanes; i++ )
    for( j=0; j<sz[i]; j++ )
      write_whisk1_segment( file, wv[i] + j );
}
#endif

Whisker_Seg *read_segments_whisker1( FILE *file, int *n)
{ Whisker_Seg *wv;
  int nwhiskers = 0;
  size_t nch;

  rewind(file);
  fskipline(file, &nch ); // skip the first (format specifying) line
  while( fskipline( file, &nch ) )
    nwhiskers++;
  *n = nwhiskers;
  wv = (Whisker_Seg*) Guarded_Malloc( sizeof(Whisker_Seg)*nwhiskers, "read whisker segments - format: whisker1");

  { int i,k;
    Whisker_Seg *w;
    rewind(file);
    fskipline(file, &nch ); // skip the first (format specifying) line
    for( i=0; i < nwhiskers; i++ )
    { w = wv + i;
      fscanf ( file, "%d,%d,%d,%d", &w->time, &w->id, &w->time, &w->len );
      w->x      = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (format: whisker1)" );
      w->y      = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (format: whisker1)" );
      w->thick  = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (format: whisker1)" );
      w->scores = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (format: whisker1)" );
      for( k=0; k < w->len; k++ )
        fscanf ( file, ",%g,%g,%g,%g", w->x + k, w->y + k, w->thick + k, w->scores + k );
    }
  }
  return wv;
}


