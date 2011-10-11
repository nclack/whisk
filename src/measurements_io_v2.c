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
#include "traj.h"
#include "error.h"

/*
 * This format is very similar to V1 but will remember the face_axis.
 * In v1, the face_axis is thrown away.
 */

// Struct is here as a reference to compute the size of the data payload 
//   written to the file.  
typedef struct _Measurements_v2
{ int row;           // offset from head of data buffer ... Note: the type limits size of table
  int fid;
  int wid;
  int state;

  int face_x;         // used in ordering whiskers on the face...roughly, the center of the face
  int face_y;         //                                      ...does not need to be in image
  int col_follicle_x; // index of the column corresponding to the folicle x position
  int col_follicle_y; // index of the column corresponding to the folicle y position
                                                                           
  int valid_velocity;
  int n;
  char face_axis;   // (v2) used to determine orientation of face for follicle ordering
  double *data;     // array of n elements
  double *velocity; // array of n elements - change in data/time
} Measurements_v2;

void measurements_v2_write_header( FILE *file )
{ char type[] = "measv2\0";
  fwrite( type, sizeof(type), 1, file );
}

void measurements_v2_read_header( FILE *file ) 
{ char type[] = "measv2\0"; 
  fseek( file, sizeof(type), SEEK_SET ); // just seek to end of header
}

int is_file_measurements_v2( const char* filename)
{ char type[] = "measv2\0";
  char buf[33];
  FILE *file = fopen(filename,"rb");
  long pos;

  if(!file)
  { warning("Could not open file (%s) for reading.\n",filename);
    return 0;
  }
  pos = ftell(file); 
  
  fread(buf, sizeof(type), 1, file);
  fclose(file);
  if( strncmp( buf, type, sizeof(type) )==0 ) // if this correct, read position is not reset
    return 1;
  return 0;
}

FILE* open_measurements_v2( const char* filename, const char* mode )
{ FILE *fp;
  if( *mode == 'w' )
  { fp = fopen(filename,"wb");
    if( fp == NULL )
    { warning("Could not open file (%s) for writing.\n");
      goto Err;
    }
    measurements_v2_write_header(fp);
  } else if( *mode == 'r' )
  { fp = fopen(filename,"rb");
    measurements_v2_read_header(fp);
  } else {
    warning("Could not recognize mode (%s) for file (%s).\n",mode,filename);
    goto Err;
  }
  return fp;
Err:
  return NULL;
}

void close_measurements_v2( FILE *fp )
{ fclose(fp);
}

void write_measurements_v2( FILE *fp, Measurements *table, int n_rows )
{ int n_measures = table[0].n;
  Measurements *row = table + n_rows;
  static const int rowsize = sizeof( Measurements_v2 ) - 2*sizeof(double*); //exclude the pointers

  fwrite( &n_rows, sizeof(int), 1, fp );
  fwrite( &n_measures, sizeof(int), 1, fp );

  while( row-- > table )
  { fwrite( row, rowsize, 1, fp );
    fwrite( row->data,     sizeof(double), n_measures, fp );
    fwrite( row->velocity, sizeof(double), n_measures, fp );
  }
}

Measurements *read_measurements_v2( FILE *fp, int *n_rows)
{ Measurements *table, *row;
  static const int rowsize = sizeof( Measurements_v2 ) - 2*sizeof(double*); //exclude the pointers
  double *head;
  int n_measures;

  fread( n_rows, sizeof(int), 1, fp);
  fread( &n_measures, sizeof(int), 1, fp );

  table = Alloc_Measurements_Table( *n_rows, n_measures );
  if(!table) return NULL;
  head = table[0].data;
  row = table + (*n_rows);

  while( row-- > table )
  { fread( row, rowsize, 1, fp );
    row->row = (row->data - head)/sizeof(double);
    fread( row->data, sizeof(double), n_measures, fp);
    fread( row->velocity, sizeof(double), n_measures, fp);
  }
  return table;
}
