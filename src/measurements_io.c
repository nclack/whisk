/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : May 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include <stdlib.h>
#include <string.h>

#include "measurements_io.h"
#include "measurements_io_v0.h"
#include "measurements_io_v1.h"
#include "measurements_io_v2.h"
#include "measurements_io_v3.h"

#include "error.h"
#include "traj.h"

#define MF_CALL(a,name)  (*(((_MeasurementsFile*)a)->name))
#define MF_DEREF(a,name)   (((_MeasurementsFile*)a)->name)
 
#define MEASUREMENTS_FILE_DEFAULT_FORMAT 2

/* 
 * Typedefs defining abstract interface for whisker file io
 * These must be  defined for each supported filetype
 */

typedef int            (*pf_mf_detect) (const char* filename);                      // Should return true iff file is of the specific file type 
typedef FILE*          (*pf_mf_open  ) (const char* filename, const char* mode );   // Opens the file for reading and/or writing
typedef void           (*pf_mf_close ) (FILE* file);                                // Writes footer, closes and frees resources
typedef void           (*pf_mf_write)  (FILE* file, Measurements *table, int n);
typedef Measurements*  (*pf_mf_read)   (FILE* file, int *n);                        // Gets the table

typedef struct __MeasurementsFile
{ FILE*          fp;
  // Interface
  pf_mf_detect   detect;
  pf_mf_open     open;
  pf_mf_close    close;
  pf_mf_write    write_segments;
  pf_mf_read     read_segments;
} _MeasurementsFile;

/***********************************************************************
 * Format registration
 */
int Measurements_File_Format_Count = 3;

char *Measurements_File_Formats[] = {
  "v0",
  "v1",
  "v2",
  "v3",
  NULL};

char *Measurements_File_Format_Descriptions[] = {
  "Binary format.  Deprecated.  See measurements_io_v0.c for details.",
  "Binary format.  Deprecated.  See measurements_io_v1.c for details.",
  "Binary format.  Deprecated.  See measurements_io_v2.c for details.",
  "Binary format.  Recommended. See measurements_io_v3.c for details.",
  NULL
};

pf_mf_detect Measurements_File_Detectors_Table[] = {
  is_file_measurements_v0,
  is_file_measurements_v1,
  is_file_measurements_v2,
  is_file_measurements_v3,
};

pf_mf_open Measurements_File_Openers_Table[] = {
  open_measurements_v0,
  open_measurements_v1,
  open_measurements_v2,
  open_measurements_v3
};

pf_mf_close Measurements_File_Closers_Table[] = {
  close_measurements_v0,
  close_measurements_v1,
  close_measurements_v2
};

pf_mf_write Measurements_File_Write_Table[] = {
  write_measurements_v0,
  write_measurements_v1,
  write_measurements_v2,
  write_measurements_v3
};

pf_mf_read Measurements_File_Read_Table[] = {
  read_measurements_v0,
  read_measurements_v1,
  read_measurements_v2,
  read_measurements_v3
};


/*********************************************************************** 
 * General interface
 */
SHARED_EXPORT
int Measurements_File_Autodetect( const char * filename, char** format )
{ int i;
  for( i=0; i < Measurements_File_Format_Count; i++ )
  { if( (*Measurements_File_Detectors_Table[i])(filename) )
    { *format = Measurements_File_Formats[i];
      return i;
    }
  }
  warning("Could not detect measurements file format for %s.\n"
        "\t\tPerhaps it's not a measurements file.\n",filename);
  return -1;
}

SHARED_EXPORT
MeasurementsFile Measurements_File_Open(const char* filename, char* format, const char* mode )
// Returns NULL on failure
{ int ifmt = -1;

  /*
   * Determine format
   */
  if( format == NULL ) // No format so guess
  { if( mode[0] == 'r' )
    { ifmt = Measurements_File_Autodetect(filename,&format);
    } else {
      ifmt = MEASUREMENTS_FILE_DEFAULT_FORMAT;
    }
    if(ifmt==-1)
      goto Err;
  } else  // Check against table
  { int i; 
    for( i=0; i < Measurements_File_Format_Count; i++ )
    { if( strncmp(format, Measurements_File_Formats[i],128) == 0 ) // match input format against registered formats
      { ifmt = i;
        break;
      }
    }//end for
    if( ifmt==-1 )
    { warning("Specified file format (%s) not recognized\n",format);
      warning("\tOptions are:\n");
      for( i=0; i < Measurements_File_Format_Count; i++ )
        warning("\t\t%s\n",Measurements_File_Formats[i]);
      goto Err;
    }
  }


  /* 
   * Open
   */
  { _MeasurementsFile *mf = (_MeasurementsFile*) malloc( sizeof(_MeasurementsFile) );
    if( mf==NULL )
    { warning("Out of memory in Measurements_File_Open\n");
      goto Err;
    }
    mf->detect          = Measurements_File_Detectors_Table       [ifmt];
    mf->open            = Measurements_File_Openers_Table         [ifmt];
    mf->close           = Measurements_File_Closers_Table         [ifmt];
    mf->write_segments  = Measurements_File_Write_Table           [ifmt];
    mf->read_segments   = Measurements_File_Read_Table            [ifmt];
    mf->fp = MF_CALL( mf, open )(filename, mode);
    if( mf->fp == NULL )
    { warning("Could not open file %s with mode %s.\n",filename,mode);
      goto Err1;
    }
    return mf;
Err1:
    free(mf);
  }

Err:
  return NULL;
}

SHARED_EXPORT
void Measurements_File_Close(MeasurementsFile mf)
{ MF_CALL( mf, close )( MF_DEREF(mf,fp) );
  MF_DEREF(mf,fp) = NULL;
  free(mf);
}

SHARED_EXPORT
void Measurements_File_Write(MeasurementsFile mf, Measurements *table, int n)
{ MF_CALL( mf, write_segments )( MF_DEREF(mf,fp),table,n);
}

SHARED_EXPORT
Measurements* Measurements_File_Read(MeasurementsFile mf, int *n)
{ return MF_CALL(mf, read_segments)( MF_DEREF(mf,fp),n);
}

SHARED_EXPORT
Measurements *Measurements_Table_From_Filename(const char *filename, char* format, int *n )
{ Measurements *table;
  MeasurementsFile mf = Measurements_File_Open(filename, format, "r");
  if(!mf) return NULL;
  table = Measurements_File_Read( mf, n); // returns NULL on failure.
  Measurements_File_Close(mf);
  return table;
}

SHARED_EXPORT
int  Measurements_Table_To_Filename(const char *filename, char* format, Measurements *table, int n )
{ MeasurementsFile mf;
  mf = Measurements_File_Open(filename, format, "w"); 
  if(!mf)
  { warning("Could not open %s\n",filename);
    return 0;
  }
  Measurements_File_Write(mf,table,n);
  Measurements_File_Close(mf);
  return 1;
}

#ifdef MEASUREMENTS_IO_CONVERTER
#include "utilities.h"
static char *Spec[] = {"<source:string> <destination:string> <format:string> | -{h|help}", NULL};
int main(int argc, char *argv[]) {
  int i;
  Process_Arguments(argc,argv,Spec,0);
  if( Is_Arg_Matched("-help") )
  { printf("\n"  
           "This is a utility for converting between the different formats available for\n"
           "measurements files.  The `source` is the input file for reading.  It's format is \n"
           "determined automatically.  The `destination` is the output file for writing.\n"
           "It's format should be specified as the `format` string.\n"
           "\n\tAvailable formats are:\n");

    for( i=0; i < Measurements_File_Format_Count; i++ )
    {   printf("\t%2d. %s\n",i+1,Measurements_File_Formats[i]);
        printf("\t\t%s\n",Measurements_File_Format_Descriptions[i]); 
    }
    printf("\n");
    return 0;
  }

  { Measurements *table;
    int n;
    table = Measurements_Table_From_Filename( Get_String_Arg("source"), NULL, &n);
    if(!table) error("Could not read %s\n",Get_String_Arg("source"));
    Measurements_Table_To_Filename( Get_String_Arg("destination"), Get_String_Arg("format"), table, n);
    Free_Measurements_Table(table);
  }

  return 0;
};
#endif

#ifdef MEASUREMENTS_IO_TEST_REPEATED_READS
#include "utilities.h"
static char *Spec[] = {"[-help] <source:string>", NULL};
int main(int argc, char *argv[]) {
  int i;
  Process_Arguments(argc,argv,Spec,0);
  if( Is_Arg_Matched("-help") )
  { printf("\n"  
           "This test will open and close the <source> file in an infinite loop\n"
           "\n");
    return 0;
  }
        
  i=0;
  while(++i)
  { Measurements *table;
    int n;
    if(i%10 == 0)
      printf("Attempt: %d.\n",i);
    table = Measurements_Table_From_Filename( Get_String_Arg("source"), NULL, &n);
    if(!table) error("Could not read %s\n",Get_String_Arg("source"));
    Free_Measurements_Table(table);
  }

  return 0;
};
#endif

#ifdef MEASUREMENTS_IO_TEST_REPEATED_READ_WRITES
#include "utilities.h"
static char *Spec[] = {"[-help] <source:string>", NULL};
int main(int argc, char *argv[]) {
  int i;
  Process_Arguments(argc,argv,Spec,0);
  if( Is_Arg_Matched("-help") )
  { printf("\n"  
           "This test will open and close the <source> file in an infinite loop\n"
           "\n");
    return 0;
  }
        
  i=0;
  while(++i)
  { Measurements *table;
    int n;
    if(i%10 == 0)
      printf("Attempt: %d.\n",i);
    table = Measurements_Table_From_Filename( Get_String_Arg("source"), NULL, &n);
    if(!table) error("Could not read %s\n",Get_String_Arg("source"));
    Measurements_Table_To_Filename( "test.measurements",NULL, table, n);
    Free_Measurements_Table(table);
  }

  return 0;
};
#endif
