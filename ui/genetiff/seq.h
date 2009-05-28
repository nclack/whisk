#ifndef H_SEQ_READER
#define H_SEQ_READER
/*
 * Reader for Streampix 3 (Norpix) movie files.  These files typically have a
 * .seq extension.
 *
 * Image data is read into the Image object used by Eugene Myers image
 * processing library, or an appropriately sized buffer.
 *
 * Author: Nathan Clack
 * Date  : September 2008
 * Notes : Time stamp functions are untested.
 *         No bounds checking on indexes. Out of bounds will result in an exit.
 */

#include <stdio.h>

typedef struct 
{ unsigned int     width;        /* Image width  in pixels                   */
  unsigned int     height;       /*       height in pixels                   */ 
  unsigned int     bitdepth;     /* significant bits per pixel               */  
  unsigned int     bitdepthreal; /* actual bits per pixel                    */ 
  unsigned int     sizebytes;    /* size of the image in bytes w*h*Bpp_real  */ 
  unsigned int     fmt;          /* image format should always be 100        */ 
  unsigned int     nframes;      /* number of frames in the movie            */ 
  unsigned int     truesize;     /* the number of bytes used per frame       */ 
  double  framerate;             /* the average framerate?                   */  
  double  starttime;             /* the timestampe of the first image        */  
  FILE   *fp;                    /*                                          */  
} SeqReader;

SeqReader *Seq_Open                 ( const char* path );
SeqReader *Seq_Close                ( SeqReader *h );
Image     *Seq_Read_Image           ( SeqReader *h, int index );
int        Seq_Read_Image_To_Buffer ( SeqReader *h, int index, void *buffer );
Stack     *Seq_Read_Stack           ( SeqReader *h );
int        Seq_Read_Stack_To_Buffer ( SeqReader *h, void *buffer );
double     Seq_Time_Stamp           ( SeqReader *h, int index );
double     Seq_Time_From_Start      ( SeqReader *h, int index );

#endif // #ifndef H_SEQ_READER
