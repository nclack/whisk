#include "compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include "utilities.h"
#include "common.h"
#include "error.h"

//
// Bar type
//
typedef struct _Bar
{ int time;
  float x;
  float y;
} Bar;

inline Bar *Bar_Static_Cast( int time, float x, float y)
{ static Bar b;
  b.time = time;
  b.x = x;
  b.y = y;
  return &b;
}

int _cmp_sort_bars_by_time(const void *a, const void *b)
{ return ((Bar*)a)->time - ((Bar*)b)->time;
}

void Bar_Sort_By_Time( Bar *bars, int nbars)
{ qsort( bars, nbars, sizeof(Bar), _cmp_sort_bars_by_time );
}

// 
// Bar File IO - basic operations
//
typedef FILE BarFile;

BarFile* Bar_File_Open(const char* filename, const char *mode)
{ BarFile* f = fopen(filename, mode);
  if(!f)
    error("Could not open bar file\n"
          "\tat %s\n"
          "\twith mode %s\n" , filename, mode);
  return f;
}

void Bar_File_Close( BarFile *file )
{ if(file) fclose(file);
}

void Bar_File_Append_Bar( BarFile *file,  Bar* b )
{ fprintf(file,"%d %g %g\n",b->time,b->x,b->y);
}

Bar *Read_Bars( BarFile *file, int *n )
{ size_t nch;
  rewind(file);
  *n=0;
  while( fskipline( file, &nch ) )
    n[0]++;

  rewind(file);
  { Bar *bars = (Bar*) Guarded_Malloc( sizeof(Bar)*(*n), "Read bars" );
    int i;
    for(i=0; i<*n; ++i)
    { Bar *b = bars + i;
      int nitems = fscanf( file, "%d%*[ ]%g%*[ ]%g", &b->time, &b->x, &b->y );
      assert(nitems == 3 );
    }
    return bars;
  }
}

Bar *Load_Bars_From_Filename( const char *filename, int *nbars )
{ BarFile *fp = Bar_File_Open( filename, "r" );
  Bar *bars = Read_Bars(fp,nbars);
  Bar_File_Close(fp);
  return bars;
}

void Save_Bars_To_Filename( const char *filename, Bar *bars, int nbars )
{ BarFile *fp = Bar_File_Open( filename, "w" );
  int i;
  for(i=0;i<nbars;i++)
    Bar_File_Append_Bar( fp, bars+i );
  Bar_File_Close(fp);
}


#if defined(TEST_BAR_IO_1) || defined(TEST_BAR_IO_2)

//evals expression px once, y up to 2 times
#define bd(TYPE,px,y) { TYPE *X = (px); *X = *X<(y) ? *X:(y);}
#define bu(TYPE,px,y) { TYPE *X = (px); *X = *X>(y) ? *X:(y);}

void bar_minmax(Bar *bars, int n_bars, Bar *lowbar, Bar* highbar)
{ Bar *row = bars + n_bars;
  //init
  lowbar->time = INT_MAX;    highbar->time = -INT_MAX;
  lowbar->x    = FLT_MAX;    highbar->x    = -FLT_MAX;
  lowbar->y    = FLT_MAX;    highbar->y    = -FLT_MAX;
  //minmax
  while( row-- > bars )
  { bd(int,    &lowbar->time, row->time );
    bd(float,  &lowbar->x   , row->x    ); 
    bd(float,  &lowbar->y   , row->y    ); 
    bu(int,    &highbar->time, row->time );
    bu(float,  &highbar->x   , row->x    ); 
    bu(float,  &highbar->y   , row->y    ); 
  }
}

void printbar(Bar *b, char *name, void (*print_fun)(char *,...))
{ print_fun("%s\n"
            "\tTime (frames): %d\n" 
            "\t       x (px): %f\n" 
            "\t       y (px): %f\n", name, b->time, b->x, b->y );
}
#endif

#ifdef TEST_BAR_IO_1
// Just do a read write cycle
char *filename = "data/testing/test.bar",
     *outfile  = "data/testing/trash.bar";
int main(int argc, char* argv[])
{ BarFile *file = Bar_File_Open(filename,"r");
  assert(file); //should not fail here, should fail in open

  { int n_bars=0;
    Bar *bars = Read_Bars(file, &n_bars),
        low,high;
    Bar_File_Close(file);
    assert(bars);

    debug( "Read - %s\n"
           "Number of bars: %d\n", filename, n_bars);
    bar_minmax(bars, n_bars, &low, &high);
    printbar( &low,  "Min", debug );
    printbar( &high, "Max", debug );

    //write 
    file = Bar_File_Open( outfile, "w" );
    assert(file); //should not fail here, should fail in open
    { int i;
      for( i=0; i<n_bars; i++ )
        Bar_File_Append_Bar( file, bars+i );
    }
    Bar_File_Close(file);
    debug("Wrote out bars to %s\n",outfile);
  }
  return 0;
}
#endif

#ifdef TEST_BAR_IO_2
// Just do a read write cycle
char *filename = "data/testing/test.bar",
     *outfile  = "data/testing/trash.bar";
int main(int argc, char* argv[])
{ int n_bars=0;
  Bar *bars = Load_Bars_From_Filename( filename, &n_bars ),
      low,high;
  assert(bars);

  debug( "Read - %s\n"
          "Number of bars: %d\n", filename, n_bars);
  bar_minmax(bars, n_bars, &low, &high);
  printbar( &low,  "Min", debug );
  printbar( &high, "Max", debug );

  //write 
  Save_Bars_To_Filename( outfile, bars, n_bars);
  debug("Wrote out bars to %s\n",outfile);
  return 0;
}
#endif
