/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : May 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include "whisker_io.h"

#include "whisker_io_whisker1.h"
#include "whisker_io_whiskbin1.h"
#include "whisker_io_whiskold.h"
#include "whisker_io_whiskpoly1.h"

#include "error.h"
#include "trace.h"

#include <string.h>

#define WF_CALL(a,name)  (*(((_WhiskerFile*)a)->name))
#define WF_DEREF(a,name) (((_WhiskerFile*)a)->name)
 
#define WHISKER_FILE_DEFAULT_FORMAT 2

/* 
 * Typedefs defining abstract interface for whisker file io
 * These must be  defined for each supported filetype
 *
 * <pf_wf_detect>
 *   Used to detect whisker file format.
 *   Returns true(1) if file has format matching implementation.
 *   On error, should warn, then return false (0).
 *
 *   The idea is that each file format implementation defines one of these
 *   function.  Each of these functions is called in succession till one returns
 *   true, then that format is used to decode a file.
 *
 * <pf_wf_open>
 *   Opens a file.
 *   On success, returns a FILE*.
 *   On failure, should warn, then return NULL.
 *
 */

typedef int            (*pf_wf_detect)           (const char* filename);                      // Should return true iff file is of the specific file type 
typedef FILE*          (*pf_wf_open  )           (const char* filename, const char* mode );   // Opens the file for reading and/or writing
typedef void           (*pf_wf_close )           (FILE* file);                                // Writes footer, closes and frees resources
typedef void           (*pf_wf_append_segments)  (FILE* file, Whisker_Seg *w, int n);
typedef void           (*pf_wf_write_segments)   (FILE* file, Whisker_Seg *w, int n);
typedef Whisker_Seg*   (*pf_wf_read_segments)    (FILE* file, int *n);                        // Gets all the segements

typedef struct __WhiskerFile
{ FILE                   *fp;
  // Interface
  pf_wf_detect            detect;
  pf_wf_open              open;
  pf_wf_close             close;
  pf_wf_append_segments   append_segments;
  pf_wf_write_segments    write_segments;
  pf_wf_read_segments     read_segments;
} _WhiskerFile;

/***********************************************************************
 * Format registration
 */
int Whisker_File_Format_Count = 3;

char *Whisker_File_Formats[] = {
  "whisk1",
  "whiskpoly1",
  "whiskbin1",
  "whiskold",
  NULL};

char *Whisker_File_Format_Descriptions[] = {
  "Text based format.  See first whisker_io_whisker1.c for details.",
  "Binary format.  Stores whiskers in a parametric polynomial\n"
    "\t\trepresentation. See whisker_io_whiskpoly1.c for details.",
  "Binary format.  See whisker_io_whiskbin1.c for details.",
  "Old text based format.  Deprecated.",
  NULL
};

pf_wf_detect Whisker_File_Detectors_Table[] = {
  is_file_whisk1,
  is_file_whiskpoly1,
  is_file_whiskbin1,
  is_file_whisk_old
};

pf_wf_open Whisker_File_Openers_Table[] = {
  open_whisk1,
  open_whiskpoly1,
  open_whiskbin1,
  open_whisk_old
};

pf_wf_close Whisker_File_Closers_Table[] = {
  close_whisk1,
  close_whiskpoly1,
  close_whiskbin1,
  close_whisk_old
};

pf_wf_append_segments Whisker_File_Append_Segments_Table[] = {
  append_segments_whisk1,
  append_segments_whiskpoly1,
  append_segments_whiskbin1,
  append_segments_whisk_old
};

pf_wf_write_segments Whisker_File_Write_Segments_Table[] = {
  append_segments_whisk1,
  append_segments_whiskpoly1,
  append_segments_whiskbin1,
  append_segments_whisk_old,
};

pf_wf_read_segments Whisker_File_Read_Segments_Table[] = {
  read_segments_whisker1,
  read_segments_whiskpoly1,
  read_segments_whiskbin1,
  read_segments_whisker_old
};


/*********************************************************************** 
 * General interface
 */
SHARED_EXPORT
int Whisker_File_Autodetect( const char * filename, char** format )
{ int i;
  for( i=0; i < Whisker_File_Format_Count; i++ )
  { if( (*Whisker_File_Detectors_Table[i])(filename) )
    { *format = Whisker_File_Formats[i];
      return i;
    }
  }
  warning("Could not detect whisker file format for %s.\n"
        "\t\tPerhaps it's not a whiskers file.\n",filename);
  return -1; // indicate failure
}

SHARED_EXPORT
WhiskerFile Whisker_File_Open(const char* filename, char* format, const char* mode )
// Returns NULL on failure
{ int ifmt = -1;

  /*
   * Determine format
   */
  if( format == NULL ) // No format so guess
  { if( mode[0] == 'r' )
    { ifmt = Whisker_File_Autodetect(filename,&format);
    } else {
      ifmt = WHISKER_FILE_DEFAULT_FORMAT;
    }
    if(ifmt==-1)   /*format failed to autodetect*/
      goto Err;
  } else  // Check against table
  { int i; 
    for( i=0; i < Whisker_File_Format_Count; i++ )
    { if( strncmp(format, Whisker_File_Formats[i],128) == 0 ) // match input format against registered formats
      { ifmt = i;
        break;
      }
    }//end for
    if( ifmt==-1 )
    { warning("Specified file format (%s) not recognized\n"
              "\tOptions are:\n",format);
      for( i=0; i < Whisker_File_Format_Count; i++ )
        warning("\t\t%s\n",Whisker_File_Formats[i]);
      goto Err;
    }
  }


  /* 
   * Open
   */
  { _WhiskerFile *wf = (_WhiskerFile*) malloc( sizeof(_WhiskerFile) );
    if( wf==NULL )
    { warning("Out of memory in Whisker_File_Open\n");
      goto Err;
    }
    wf->detect          = Whisker_File_Detectors_Table       [ifmt];
    wf->open            = Whisker_File_Openers_Table         [ifmt];
    wf->close           = Whisker_File_Closers_Table         [ifmt];
    wf->append_segments = Whisker_File_Append_Segments_Table [ifmt];
    wf->write_segments  = Whisker_File_Write_Segments_Table  [ifmt];
    wf->read_segments   = Whisker_File_Read_Segments_Table   [ifmt];
    wf->fp = WF_CALL( wf, open )(filename, mode);
    if( wf->fp == NULL )
    { warning("Could not open file %s with mode %s.\n",filename,mode);
      goto Err1;
    }
    return wf;
Err1:
    free(wf);
  }

Err:
  return NULL;
}

SHARED_EXPORT
void Whisker_File_Close(WhiskerFile wf)
{ WF_CALL( wf, close )( WF_DEREF(wf,fp) );
  WF_DEREF(wf,fp) = NULL;
  //wf->fp = NULL;
  free(wf);
}

SHARED_EXPORT
void Whisker_File_Append_Segments(WhiskerFile wf, Whisker_Seg *w, int n)
{ WF_CALL( wf, append_segments )( WF_DEREF(wf,fp) ,w,n);
}

SHARED_EXPORT
void Whisker_File_Write_Segments(WhiskerFile wf, Whisker_Seg *w, int n)
{ WF_CALL( wf, write_segments )( WF_DEREF(wf,fp),w,n);
}

SHARED_EXPORT
Whisker_Seg* Whisker_File_Read_Segments(WhiskerFile wf, int *n)
{ return WF_CALL(wf, read_segments)( WF_DEREF(wf,fp),n);
}

SHARED_EXPORT
Whisker_Seg *Load_Whiskers(const char *filename, char* format, int *n )
{ Whisker_Seg *wv;
  WhiskerFile wf = Whisker_File_Open(filename, format, "r");
  if(!wf)
    return NULL;
  wv = Whisker_File_Read_Segments( wf, n);
  Whisker_File_Close(wf);
  return wv;
}

SHARED_EXPORT
int  Save_Whiskers(const char *filename, char* format, Whisker_Seg *w, int n )
{ WhiskerFile wf;
  if( format == NULL )
  { wf = Whisker_File_Open(filename, Whisker_File_Formats[ WHISKER_FILE_DEFAULT_FORMAT ], "w");
  } else 
  { wf = Whisker_File_Open(filename, format, "w"); 
  }
  if(!wf)
    return 0;
  Whisker_File_Write_Segments(wf,w,n);
  Whisker_File_Close(wf);
  return 1;
}

#ifdef WHISKER_IO_CONVERTER
#include "utilities.h"
static char *Spec[] = {"<source:string> <destination:string> <format:string> | -{h|help}", NULL};
int main(int argc, char *argv[]) {
  int i;
  Process_Arguments(argc,argv,Spec,0);
  if( Is_Arg_Matched("-help") )
  { printf("\n"  
           "This is a utility for converting between the different formats available for\n"
           "whisker files.  The `source` is the input file for reading.  It's format is \n"
           "determined automatically.  The `destination` is the output file for writing.\n"
           "It's format should be specified as the `format` string.\n"
           "\n\tAvailable formats are:\n");

    for( i=0; i < Whisker_File_Format_Count; i++ )
    {   printf("\t%2d. %s\n",i+1,Whisker_File_Formats[i]);
        printf("\t\t%s\n",Whisker_File_Format_Descriptions[i]); 
    }
    printf("\n");
    return 0;
  }

  { Whisker_Seg *wv;
    int n;
    wv = Load_Whiskers( Get_String_Arg("source"), NULL, &n);
    Save_Whiskers( Get_String_Arg("destination"), Get_String_Arg("format"), wv, n);
    Free_Whisker_Seg_Vec(wv,n);
  }

  return 0;
};
#endif

