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
#include <stdlib.h>
#include "traj.h"
#include "error.h"

  /* BEWARE
   * Untested patch.  Several problems below.
   * - this format serialized the "data" and "velocity" pointers and
   *   later used them to compute offsets.  This broke when the files
   *   were moved between machines with different pointer sizes.
   *   Hence, this format was deprecated.
   * - Later, the Measurements structure was changed.  The code was changed
   *   to read v0 data into the newer (current) Measurements structure.
   *   This is relatively untested as I don't have any v0 data around.
   */

// Struct is here as a reference to compute the size of the data payload 
//   written to the file.  
typedef struct _Measurements_v0
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
  double *data;     // array of n elements
  double *velocity; // array of n elements - change in data/time
} Measurements_v0;

int is_file_measurements_v0( const char* filename)
{ FILE *fp = fopen(filename,"rb");
  char format[33],
       not[] = "meas";
  if(fp==NULL)
  { warning("In is_file_measurements_v0, could not open file (%s) for reading.\n",filename);
    return 0;
  }
    
  fscanf(fp,"%32s", format);
  return strncmp(format,not,sizeof(not)-1) != 0 ;
}

FILE* open_measurements_v0( const char* filename, const char* mode )
{ FILE *fp;
  if( *mode == 'w' )
  { fp = fopen(filename,"wb");
    if( fp == NULL )
    { warning("Could not open file (%s) for writing.\n");
      goto Err;
    }
  } else if( *mode=='r' )
  { fp = fopen(filename,"rb");
  } else {
    warning("Could not recognize mode (%s) for file (%s).\n",mode,filename);
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
  { int i;
    for(i=0;i<n_rows;++i)
      fwrite( table+i, sizeof(Measurements_v0), 1, fp);
  }
  fwrite( table[0].data - table[0].row*table[0].n, sizeof(double), 2*n_rows*n_measures, fp );
}

Measurements *read_measurements_v0( FILE *fp, int *n_rows)
{ Measurements *table;
  Measurements_v0 *temp;
  int n_measures,i;
  double *ref, *old;

  memset(&temp,0,sizeof(temp));

  fread( n_rows, sizeof(int), 1, fp);
  fread( &n_measures, sizeof(int), 1, fp );

  table = Alloc_Measurements_Table( *n_rows, n_measures );
  temp = malloc(sizeof(Measurements_v0)*n_rows[0]);
  if(!temp) 
  {
    warning("(%s:%d\n\tCould not allocate temporary table\n",__FILE__,__LINE__);
    return NULL;
  }
  if(!table)
  { warning("Could not allocate measurements table\n");
    return NULL;
  }
  ref = table[0].data; // head of newly allocated data block
#ifdef DEBUG_MEASUREMENTS_TABLE_FROM_FILE
  debug("\tLoad -           ref: %p\n",ref);
#endif
  /* BEWARE
   * Untested patch.  Several problems below.
   * - this format serialized the "data" and "velocity" pointers and
   *   later used them to compute offsets.  This broke when the files
   *   were moved between machines with different pointer sizes.
   *   Hence, this format was deprecated.
   * - Later, the Measurements structure was changed.  The patch below
   *   reads v0 data into the newer (current) Measurements structure.
   */
  fread( temp, sizeof(Measurements_v0), *n_rows, fp); 
  
  // copy to new Measurements structure
  { int i;
    for(i=0;i<*n_rows;++i)
    { memcpy(table+i,temp+i,sizeof(Measurements_v0)-2*sizeof(double));
      table[i].face_axis = 'u'; // mark as unknown
      //table[i].row = i;
    }
  }

  old = table[0].data - table[0].row * n_measures; // head of old (nonexistent) data block
  fread( ref, sizeof(double), 2*(*n_rows)*n_measures, fp );
  
  // update pointers to new data block
  i = *n_rows;
  while(i--)
  { Measurements *row = table + i;
    row->face_axis = 'u'; // mark as unknown
    row->data     = ref + ( row->data     - old );
    row->velocity = ref + ( row->velocity - old );
  }
#ifdef DEBUG_MEASUREMENTS_TABLE_FROM_FILE
  debug("\tLoad - table[0].data: %p\n",table[0].data);
  debug("\tLoad - table[0].row : %d\n",table[0].row );
#endif

  return table;
}
