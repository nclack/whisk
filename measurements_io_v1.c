#include <stdio.h>
#include "traj.h"

void measurements_v1_write_header( FILE *file )
{ char type[] = "measV1\0";
  fwrite( type, sizeof(type), 1, file );
}

void measurements_v1_read_header( FILE *file ) 
{ char type[] = "measV1\0"; 
  fseek( file, sizeof(type), SEEK_SET ); // just seek to end of header
}

int is_file_measurements_v1( const char* filename)
{ char type[] = "measV1\0";
  char buf[33];
  FILE *file = fopen(filename,"rb");
  long pos = ftell(file);

  if(!file)
    error("Could not open file (%s) for reading.\n",filename);
  
  fread(buf, sizeof(type), 1, file);
  fclose(file);
  if( strncmp( buf, type, sizeof(type) )==0 ) // if this correct, read position is not reset
    return 1;
  return 0;
}

FILE* open_measurements_v1( const char* filename, const char* mode )
{ FILE *fp;
  if( *mode == 'w' )
  { fp = fopen(filename,"wb");
    if( fp == NULL )
    { error("Could not open file (%s) for writing.\n");
      goto Err;
    }
    measurements_v1_write_header(fp);
  } else if( *mode == 'r' )
  { fp = fopen(filename,"rb");
    measurements_v1_read_header(fp);
  } else {
    error("Could not recognize mode (%s) for file (%s).\n",mode,filename);
    goto Err;
  }
  return fp;
Err:
  return NULL;
}

void close_measurements_v1( FILE *fp )
{ fclose(fp);
}

void write_measurements_v1( FILE *fp, Measurements *table, int n_rows )
{ int n_measures = table[0].n;
  fwrite( &n_rows, sizeof(int), 1, fp );
  fwrite( &n_measures, sizeof(int), 1, fp );
  fwrite( table, sizeof(Measurements), n_rows, fp);
  fwrite( table[0].data - table[0].row*table[0].n, sizeof(double), 2*n_rows*n_measures, fp );
}

Measurements *read_measurements_v1( FILE *fp, int *n_rows)
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
