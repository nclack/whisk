#include "whisker_io_whiskold.h"
#include "trace.h"
#include "error.h"

void write_segments_old( FILE* file, Whisker_Seg_Old **wv, int *sz, int nplanes )
{ Whisker_Seg_Old *w;
  int i,j,iy;

  for( i=0; i<nplanes; i++ )
  { for( j=0; j<sz[i]; j++ )
    { w = wv[i] + j;
      fprintf( file, "%d,%d,%d,%d", i, w->id, w->beg, w->end );
      for( iy=0; iy < (w->end - w->beg + 1); iy++ )
        fprintf( file, ",%g", w->track[iy]);
      fprintf(file, "\n");
    }
  }
}

int is_file_whisk_old( const char* filename)
{ FILE *fp = fopen(filename,"r");
  char format[33],
       not[] = "whisk";
  if(fp==NULL)
    { error("In is_file_whisk1, could not open file (%s) for reading.\n",filename);
      exit(1);
    }
  fscanf(fp,"%32s", format);

  if( strncmp(format,not,sizeof(not)) != 0 )
  { int n, time, id, beg, end;
    rewind(fp);
    n = fscanf( fp, "%d%*[, ]%d%*[, ]%d%*[, ]%d", &time, &id, &beg, &end );
    fclose(fp);
    return n==4;
  }
  fclose(fp);
  return 0;
}

FILE* open_whisk_old(const char* filename, const char* mode)
{ FILE *fp;
  if( strncmp(mode,"w",1)==0 )
  { error("This format is depricated and writing is not supported.\n");
    return NULL;
  } else if( strncmp(mode,"r",1)==0 )
  { fp = fopen(filename,"r");
  } else {
    error("Could not recognize mode (%s) for file (%s).\n",mode,filename);
    return NULL;
  }
  return fp;
}

void close_whisk_old(FILE* fp)
{ fclose(fp);
}

void append_segments_whisk_old ( FILE *fp, Whisker_Seg *wv, int n )
{ error("This format is depricated and writing is not supported.\n");
}

Whisker_Seg *read_segments_whisker_old( FILE *file, int *n)
{ Whisker_Seg *wv;
  int nwhiskers = 0;
  size_t nch;

  rewind(file);
  while( fskipline( file, &nch ) )
    nwhiskers++;
  *n = nwhiskers;
  wv = (Whisker_Seg*) Guarded_Malloc( sizeof(Whisker_Seg)*nwhiskers, "read whisker segments (old format)");

  /* Read things in */
  { int irow,iy,beg,end;
    Whisker_Seg *w;
    rewind( file );
    for( irow=0; irow<nwhiskers; irow++ )
    { w = wv + irow;
      fscanf( file, "%d%*[, ]%d%*[, ]%d%*[, ]%d", &w->time, &w->id, &beg, &end );
      w->len    = end-beg+1;
      w->x      = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (old format)" );
      w->y      = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (old format)" );
      w->thick  = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (old format)" );
      w->scores = (float*) Guarded_Malloc( sizeof(float)*(w->len), "read whisker segments (old format)" );
      for( iy=0; iy < (end - beg + 1); iy++ )
      { fscanf( file, "%*[, ]%g", w->y + iy);
        w->x[iy] = beg + iy;
        w->thick[iy] = 1.0;
        w->scores[iy] = 0.0;
      }
    }
  }
  return wv;
}
