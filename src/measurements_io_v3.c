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
   This format is very similar to V2.

   V2 had a bug where the size of the reference struct was different on 32-bit vs
   64-bit systems (at least when compiled with visual studio).

   Here we set the bytes size using the ROWSIZE macro to make sure it's the same
   across systems.  I could've tried to hack v2 to consistently read/write files
   from/to 64-bit systems, but files written by 32-bit systems (in this example) 
   would have been unreadible.  Adding a new format maximizes compatibility 
   with old files.
 */

// Struct is here as a reference to compute the size of the data payload 
//   written to the file.  
typedef struct _Measurements_v3
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
  char face_axis;   // (v3) used to determine orientation of face for follicle ordering
  double *data;     // array of n elements
  double *velocity; // array of n elements - change in data/time
} Measurements_v3;

#define ROWSIZE (41) //Bytes

void measurements_v3_write_header( FILE *file )
{ char type[] = "measv3\0";
  fwrite( type, sizeof(type), 1, file );
}

void measurements_v3_read_header( FILE *file ) 
{ char type[] = "measv3\0"; 
  fseek( file, sizeof(type), SEEK_SET ); // just seek to end of header
}

int is_file_measurements_v3( const char* filename)
{ char type[] = "measv3\0";
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

FILE* open_measurements_v3( const char* filename, const char* mode )
{ FILE *fp;
  if( *mode == 'w' )
  { fp = fopen(filename,"wb");
    if( fp == NULL )
    { warning("Could not open file (%s) for writing.\n");
      goto Err;
    }
    measurements_v3_write_header(fp);
  } else if( *mode == 'r' )
  { fp = fopen(filename,"rb");
    measurements_v3_read_header(fp);
  } else {
    warning("Could not recognize mode (%s) for file (%s).\n",mode,filename);
    goto Err;
  }
  return fp;
Err:
  return NULL;
}

void close_measurements_v3( FILE *fp )
{ fclose(fp);
}

#define HERE printf("%s(%d)\n",__FILE__,__LINE__)

void write_measurements_v3( FILE *fp, Measurements *table, int n_rows )
{ int n_measures = table[0].n;
  Measurements *row = table + n_rows;
  //static const int rowsize = sizeof( Measurements_v3 ) - 2*sizeof(double*); //exclude the pointers
  
  fwrite( &n_rows, sizeof(int), 1, fp );
  fwrite( &n_measures, sizeof(int), 1, fp );

  while( row-- > table )
  { fwrite( row, ROWSIZE, 1, fp );

    
    //if(row->data==-1)
    //{
    //  HERE;
    //  printf("table: %p\trow: %p\tdata: %p\tvel: %p\n",table,row,row->data,row->velocity);
    //  printf("row:%6d fid:%5d wid:%3d state:%2d face:(%4d,%4d) col:(%1d,%1d) valid vel:%1d n:%d face_axis:%c\n",
    //    row->row,
    //    row->fid,
    //    row->wid,
    //    row->state,
    //    row->face_x,
    //    row->face_y,
    //    row->col_follicle_x,
    //    row->col_follicle_y,
    //    row->valid_velocity,
    //    row->n,
    //    row->face_axis);   
    //}

    fwrite( row->data,     sizeof(double), n_measures, fp );
    fwrite( row->velocity, sizeof(double), n_measures, fp );
  }
}

Measurements *read_measurements_v3( FILE *fp, int *n_rows)
{ Measurements *table, *row;
  static const int rowsize = sizeof( Measurements_v3 ) - 2*sizeof(double*); //exclude the pointers
  double *head;
  int n_measures;

  fread( n_rows, sizeof(int), 1, fp);
  fread( &n_measures, sizeof(int), 1, fp );

  table = Alloc_Measurements_Table( *n_rows, n_measures );
  if(!table) return NULL;
  head = table[0].data;
  row = table + (*n_rows);

  while( row-- > table )
  { fread( row, ROWSIZE, 1, fp );
    row->row = (row->data - head)/sizeof(double);
    fread( row->data, sizeof(double), n_measures, fp);
    fread( row->velocity, sizeof(double), n_measures, fp);
  }
  return table;
}
