#include <stdio.h>

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
    { Bar *b = bars[i];
      fscanf( file, "%d%*[ ]%d%*[ ]%d", &b->time, &b->x, &b-y );
    }
    return bars;
  }
}
