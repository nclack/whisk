#include "compat.h"

#include <stdio.h>
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

// 
// Bar File IO - basic operations
//
typedef FILE BarFile;

//
// Bar File IO - main api
//
//
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
  while( fskipline( file, &nch ) )
    *n++;

  { Bar *bars = (Bar*) Guarded_Malloc( sizeof(Bar)*(*n), "Read bars" );
    int i;
    for(i=0; i<*n; ++i)
    { Bar *b = bars + i;
      fscanf( file, "%d%*[ ]%g%*[ ]%g", &b->time, &b->x, &b->y );
    }
    return bars;
  }
}

#ifdef TEST_BAR_IO_1

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
            "\tTime (frames): %d\n",
            "\t       x (px): %f\n",
            "\t       y (px): %f\n", name, b->time, b->x, b->y );
}

// Just do a read write cycle
char *filename = "data/testing/test.bar";
int main(int argc, char* argv[])
{ BarFile *file = Bar_File_Open(filename,"r");
  assert(file); //should not fail here, should fail in open

  { int n_bars;
    Bar *bars = Read_Bars(file, &n_bars),
        low,high;
    Bar_File_Close(file);
    assert(bars);

    debug( "Read - $s\n"
           "Number of bars: %d\n", filename, n_bars);
    bar_minmax(bars, n_bars, &low, &high);
    printbar( &low,  "Min", debug );
    printbar( &high, "Max", debug );

    //write - writes over test data file!!
    file = Bar_File_Open( filename, "w" );
    assert(file); //should not fail here, should fail in open
    { int i;
      for( i=0; i<n_bars; i++ )
        Bar_File_Append_Bar( file, bars+i );
    }
    Bar_File_Close(file);
    debug("Wrote out bars to %s\n",filename);
  }
  return 0;
}
#endif
