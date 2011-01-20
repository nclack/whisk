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
#include <float.h>
#include "trace.h"
#include "utilities.h"
#include "whisker_io.h"

int  test_read( char *filename )
{ int n;
  Whisker_Seg *wv = Load_Whiskers( filename, NULL, &n );
  if( !wv ) 
  { printf( "Could not open file\n");
    return 0;
  }
  printf("\tRead        : %d whiskers\n",n);

  { int i,nframes = 0;
    float minscores = FLT_MAX, maxscores = -1;
    float minthick = FLT_MAX, maxthick = -1;
    float minx = FLT_MAX, maxx = -1;
    float miny = FLT_MAX, maxy = -1;
    i = n;
    while(i--)
    { Whisker_Seg *t = wv + i;
      int j = t->len;
      nframes = ( nframes  > t->time   ) ? nframes   : t->time;
      while(j--)
      { 
        maxscores =( maxscores >  t->scores[j] ) ? maxscores  : t->scores[j];
        minscores =( minscores <  t->scores[j] ) ? minscores  : t->scores[j];
        maxthick =( maxthick >  t->thick[j] ) ? maxthick  : t->thick[j];
        minthick =( minthick <  t->thick[j] ) ? minthick  : t->thick[j];
        maxx =( maxx >  t->x[j] ) ? maxx  : t->x[j];
        minx =( minx <  t->x[j] ) ? minx  : t->x[j];
        maxy =( maxy >  t->y[j] ) ? maxy  : t->y[j];
        miny =( miny <  t->y[j] ) ? miny  : t->y[j];
      }
    }
    nframes += 1;
    printf("\tMovie length: %d frames\n",nframes);
    printf("\tScore range : [%7.5g,%7.5g]\n", minscores, maxscores);
    printf("\tThick range : [%7.5g,%7.5g]\n", minthick, maxthick);
    printf("\t    x range : [%7.5g,%7.5g]\n", minx, maxx);
    printf("\t    y range : [%7.5g,%7.5g]\n", miny, maxy);
  }
  Free_Whisker_Seg_Vec( wv, n );
  return 1;
}

int  test_read_write_read( char *filename )
{ int i,n,nframes=0;
  char outname[] = "test_read_write_read.whiskers";
  Whisker_Seg *wv = Load_Whiskers( filename, NULL, &n );
  if( !wv ) 
  { printf( "Could not open file\n");
    return 0;
  }
  printf("\tRead        : %d whiskers\n",n);
  
  i = n;
  while(i--)
  { Whisker_Seg *t = wv + i;
    nframes = ( nframes  > t->time   ) ? nframes   : t->time;
  }
  nframes += 1;
  printf("\tMovie length: %d frames\n",nframes);

  Save_Whiskers( outname, NULL, wv, n );

  /* read written file and compare */
  { int n2 = 0;
    Whisker_Seg *wv2 = Load_Whiskers( outname, NULL, &n2 );
    printf("\tRead        : %d whiskers\n",n2);
    return is_exact_match_whisker_seg_vec( wv, n, wv2, n2);
  }
}

int is_exact_match_whisker_seg_vec( Whisker_Seg *wv, int n, Whisker_Seg *wv2, int n2)
{   int i;
    if( n != n2 ) return 0;
    i = n2;
    while( i-- )
    { int j;
      Whisker_Seg *s = wv  + i,
                  *t = wv2 + i;
      if(!( (s->id    == t->id)      && 
            (s->time  == t->time)    && 
            (s->len   == t->len)     ))
      { printf("\t*** FAIL! Inequality in id, time or len on read/write/read\n");
        return 0;
      }
      j = s->len;
      while( j-- )
        if(!( (s->x[j]      == t->x[j]      ) && 
              (s->y[j]      == t->y[j]      ) &&
              (s->thick[j]  == t->thick[j]  ) &&
              (s->scores[j] == t->scores[j] ) ))
        { printf("\t*** FAIL! Inequality in (x,y,scores,thick) on read/write/read\n");
          return 0;
        }
    }
    return 1;
}

int  test_read_write_read2( char *filename )
{ int i,n,nframes=0;
  char outname[] = "test_read_write_read.whiskers";
  Whisker_Seg *wv = Load_Whiskers( filename, NULL, &n );
  if( !wv ) 
  { printf( "Could not open file\n");
    return 0 ;
  }
  printf("\tRead        : %d whiskers\n",n);
  
  i = n;
  while(i--)
  { Whisker_Seg *t = wv + i;
    nframes = ( nframes  > t->time   ) ? nframes   : t->time;
  }
  nframes++;
  printf("\tMovie length: %d frames\n",nframes);

  /* Build frame index - assumes whiskers were written in order*/
  { Whisker_Seg **index = (Whisker_Seg**) Guarded_Malloc( sizeof(Whisker_Seg*) * nframes, "test_read_write_read" );
    int *sz = (int*) Guarded_Malloc( sizeof(int) * nframes, "test_read_write_read" );
    memset(sz,0,sizeof(int)*nframes);

    i = n;
    while( i-- )
    { Whisker_Seg *t = wv + i;
      sz[ t->time ]++;
    }
    i = 0;
    index[0] = wv;
    while( i++ < nframes)
      index[i] = index[i-1] + sz[i-1];

    { WhiskerFile fp = Whisker_File_Open(outname, NULL, "w" );
      printf("\tWriting (%s)...",outname);
      for( i=0; i < nframes; i++ )
      { 
        Whisker_File_Append_Segments(fp, index[i], sz[i]);
      }
      printf("Done.\n");
      Whisker_File_Close(fp);
    }
    free(index);
    free(sz);
  }

  /* read written file and compare */
  { int n2 = 0;
    Whisker_Seg *wv2 = Load_Whiskers( outname, NULL, &n2 );
    printf("\tRead        : %d whiskers\n",n2);
    return is_exact_match_whisker_seg_vec( wv, n, wv2, n2);
  }
}

const int ntests = 3;
int   (*test_funcs[4])(char*) = { test_read,
                                 test_read_write_read,
                                 test_read_write_read2,
                                 NULL };

static char *Spec[] = {"<prefix:string>", NULL};
int main( int argc, char* argv[] )
{ char *prefix;
  char *whiskers_file_name;

  Process_Arguments(argc,argv,Spec,0);

  prefix = Get_String_Arg("prefix");
  whiskers_file_name = (char*) Guarded_Malloc( (strlen(prefix)+32)*sizeof(char), "whisker file name");
  sprintf( whiskers_file_name, "%s.whiskers", prefix );
  
  { int i = ntests;
    while(i--)
    { printf("--- TEST %d ----------------------------\n", i);
      if( (test_funcs[i])(whiskers_file_name) )
        printf("--- TEST %d --- PASSED 0.o -------------\n\n",i);
      else
        printf("*** TEST %d FAILED T.T *****************\n\n",i);
    }
  }
  return 0;
}
