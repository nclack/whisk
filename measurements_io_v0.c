#include <stdio.h>
#include <string.h>
#include "traj.h"
#include "error.h"

int is_file_measurements_v0( const char* filename)
{ FILE *fp = fopen(filename,"rb");
  char format[33],
       not[] = "meas";
  if(fp==NULL)
    error("In is_file_measurements_v0, could not open file (%s) for reading.\n",filename);
    
  fscanf(fp,"%32s", format);
  return strncmp(format,not,sizeof(not)-1) != 0 ;
}

FILE* open_measurements_v0( const char* filename, const char* mode )
{ FILE *fp;
  if( *mode == 'w' )
  { fp = fopen(filename,"wb");
    if( fp == NULL )
    { error("Could not open file (%s) for writing.\n");
      goto Err;
    }
  } else if( *mode=='r' )
  { fp = fopen(filename,"rb");
  } else {
    error("Could not recognize mode (%s) for file (%s).\n",mode,filename);
    goto Err;
  }
  return fp;
Err:
  return NULL;
}

void close_measurements_v0( FILE *fp )
{ fclose(fp);
}

void write_measurements_v0( FILE *fp, Measurements *table, int n_rows )
{ int n_measures = table[0].n;
  fwrite( &n_rows, sizeof(int), 1, fp );
  fwrite( &n_measures, sizeof(int), 1, fp );
  fwrite( table, sizeof(Measurements), n_rows, fp);
  fwrite( table[0].data - table[0].row*table[0].n, sizeof(double), 2*n_rows*n_measures, fp );
}

Measurements *read_measurements_v0( FILE *fp, int *n_rows)
{ Measurements *table;
  int n_measures,i;
  double *ref, *old;

  fread( n_rows, sizeof(int), 1, fp);
  fread( &n_measures, sizeof(int), 1, fp );

  table = Alloc_Measurements_Table( *n_rows, n_measures );
  ref = table[0].data; // head of newly allocated data block
#ifdef DEBUG_MEASUREMENTS_TABLE_FROM_FILE
  debug("\tLoad -           ref: %p\n",ref);
#endif
  fread( table, sizeof(Measurements), *n_rows, fp); 
  old = table[0].data - table[0].row * n_measures; // head of old (nonexistent) data block
  fread( ref, sizeof(double), 2*(*n_rows)*n_measures, fp );
  
  // update pointers to new data block
  i = *n_rows;
  while(i--)
  { Measurements *row = table + i;
    row->data     = ref + ( row->data     - old );
    row->velocity = ref + ( row->velocity - old );
  }
#ifdef DEBUG_MEASUREMENTS_TABLE_FROM_FILE
  debug("\tLoad - table[0].data: %p\n",table[0].data);
  debug("\tLoad - table[0].row : %d\n",table[0].row );
#endif

  return table;
}
