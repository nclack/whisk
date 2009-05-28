/*
 * Reader for Streampix 3 (Norpix) movie files.  These files typically have a
 * .seq extension.
 *
 * Image data is read into the Image object used by Eugene Myers image
 * processing library.
 *
 * Author: Nathan Clack
 * Date  : September 2008
 * Notes : Time stamp functions are untested.
 */
#include <stdio.h>
#include <stdlib.h>
#include "utilities.h"
#include "image_lib.h"
#include "seq.h"

//#undef    SEQ_TESTING
//#undef    SEQ_PARANOID

#ifdef SEQ_PARANOID
#define   SEQ_ASSERT(stmt)    if(stmt) goto error
#else
#define   SEQ_ASSERT(stmt)    stmt
#endif

SeqReader *Seq_Open( const char* path )
{ FILE *fp;
  SeqReader *h;
  
  h = Guarded_Malloc( sizeof(SeqReader), "seq_open" );
  fp = fopen( path, "rb" );
  if( !fp )
    return NULL;
  h->fp = fp;

  SEQ_ASSERT( fseek(fp, 548, SEEK_SET)                     );
  SEQ_ASSERT( fread( &(h->width         ), 4, 1, fp ) != 1 );
  SEQ_ASSERT( fread( &(h->height        ), 4, 1, fp ) != 1 );
  SEQ_ASSERT( fread( &(h->bitdepth      ), 4, 1, fp ) != 1 );
  SEQ_ASSERT( fread( &(h->bitdepthreal  ), 4, 1, fp ) != 1 );
  SEQ_ASSERT( fread( &(h->sizebytes     ), 4, 1, fp ) != 1 );
  SEQ_ASSERT( fread( &(h->fmt           ), 4, 1, fp ) != 1 );
  SEQ_ASSERT( fread( &(h->nframes       ), 4, 1, fp ) != 1 );
  SEQ_ASSERT( fseek(fp, 580, SEEK_SET)                     );
  SEQ_ASSERT( fread( &(h->truesize      ), 4, 1, fp ) != 1 );
  SEQ_ASSERT( fread( &(h->framerate     ), 8, 1, fp ) != 1 );

  h->starttime = Seq_Time_Stamp( h, 0 );
  return h;
error:
  fprintf( stderr, "Problem reading header of seq file\n" );
  if(h)  free(h);
  if(fp) fclose(fp);
  return NULL;
}

SeqReader *Seq_Close( SeqReader *h )
{ if(h)
  { if(h->fp) 
      fclose(h->fp);
    free(h);
  }
  return NULL;
}

Image *Seq_Read_Image( SeqReader *h, int index )
{ size_t offset = 1024 + index * h->truesize;
  Image *im = Make_Image( h->bitdepthreal/8, h->width, h->height );
  
  SEQ_ASSERT( fseek( h->fp, offset, SEEK_SET)                            );
  SEQ_ASSERT( fread( im->array, 1, h->sizebytes, h->fp ) != h->sizebytes ); 

  return im;
error:
  fprintf(stderr, "WARNING: Couldn't read image at index %d\n",index);
  Free_Image( im );
  return NULL;
}

int  Seq_Read_Images_To_Buffer ( SeqReader *r, int start, int stop, int step,  void *buffer )
{ size_t offset = 1024;
  size_t dz_buf, dz_seq;
  unsigned int i = 0, count=0;
  unsigned int w,h;
  dz_seq = r->truesize;
  dz_buf = r->sizebytes;  

  //printf("From %d to %d by %d to %p.\n", start, stop, step, buffer);
  for( i=start; i < stop; i+=step )
  { //printf(" Read at %d and write to %d\n",step*i+start ,count);
    SEQ_ASSERT( fseek( r->fp, 1024+i*dz_seq       , SEEK_SET ) );
    SEQ_ASSERT( fread( buffer + (count++)*dz_buf,1, dz_buf, r->fp ) != dz_buf   );
  } 

  return 0;
error:
  fprintf(stderr, "ERROR:   Couldn't read stack.\n");
  fprintf(stderr, "         Failed on %d.\n", i);     
  return 1;   
}

int  Seq_Read_Image_To_Buffer ( SeqReader *h, int index, void *buffer )
{ size_t offset = 1024 + index * h->truesize;
  
  SEQ_ASSERT( fseek( h->fp, offset, SEEK_SET)                            );
  SEQ_ASSERT( fread( buffer   , 1, h->sizebytes, h->fp ) != h->sizebytes ); 
  return 0;
error:
  fprintf(stderr, "WARNING: Couldn't read image at index %d\n",index);
  return 1;   
}

Stack *Seq_Read_Stack ( SeqReader *r )
{ size_t offset = 1024;
  size_t dz;
  unsigned int i = 0;
  unsigned int w,h;
  Stack *s = Make_Stack( r->bitdepthreal/8, r->width, r->height, r->nframes );
  w = r->width;
  h = r->height;
  dz = w*h*(s->kind);

  for( i=0; i < r->nframes; i++ )
  { SEQ_ASSERT( fseek( r->fp, 1024+i*(r->truesize), SEEK_SET ) );
    SEQ_ASSERT( fread( s->array + i*dz, 1, dz, r->fp ) != dz   );
  } 
  return s;

error:
  fprintf(stderr, "ERROR:   Couldn't read stack.\n");
  fprintf(stderr, "         Failed on %d.\n", i);     
  return NULL;
}

int  Seq_Read_Stack_To_Buffer ( SeqReader *r, void *buffer )
{ size_t offset = 1024;
  size_t dz_buf, dz_seq;
  unsigned int i = 0;
  unsigned int w,h;
  dz_seq = r->truesize;
  dz_buf = r->sizebytes;  

  for( i=0; i < r->nframes; i++ )
  { SEQ_ASSERT( fseek( r->fp, 1024+i*dz_seq       , SEEK_SET ) );
    SEQ_ASSERT( fread( buffer +i*dz_buf,1, dz_buf, r->fp ) != dz_buf   );
  } 

  return 0;
error:
  fprintf(stderr, "ERROR:   Couldn't read stack.\n");
  fprintf(stderr, "         Failed on %d.\n", i);     
  return 1;   
}

double Seq_Time_Stamp( SeqReader *h, int index )
{ size_t offset = 1024 + index * h->truesize + h->sizebytes;
  double t1,t2;

  SEQ_ASSERT( fseek( h->fp, offset, SEEK_SET)                     );
  SEQ_ASSERT( fread( &t1, 8, 1 , h->fp )                     != 1 );
  SEQ_ASSERT( fread( &t2, 8, 1 , h->fp )                     != 1 );
  return t1 + t2/1000.0;

error:
  fprintf( stderr, "\nError reading time stamp at index %d\n", index );
  exit(1);
}

double Seq_Time_From_Start( SeqReader *h, int index )
{ return Seq_Time_Stamp( h, index ) - h->starttime;
}

/*
 * TESTS
 */
#ifdef SEQ_TESTING
#define SEQ_TEST_RESULT(x) ((x)?"PASS":"FAIL")

int test1( char *path )
/* Test: Read an image as a myers.pack Image */
{ Image *im;
  SeqReader *f = Seq_Open( path );
  if(f)
  { im = Seq_Read_Image( f, 0 );
    Write_Image( "test_seq_read_0.tif", im );
    Free_Image(im);
    return 1;
  }
  return 0;
}

int test2( char *path )
/* Test: Read a  stack as a myers.pack Stack */
{ Stack *s;
  SeqReader *f = Seq_Open( path );
  if(f)
  { s = Seq_Read_Stack( f );
    Write_Stack( "test_seq_stack_read.tif", s );
    Free_Stack( s );
    return 1;
  }
  return 0;
}

int test3( char *path )
/* Test: Single image read to buffer */
{ SeqReader *f = Seq_Open( path );
  size_t n = f->sizebytes;
  Image im;
  im.width = f->width;
  im.height = f->height;
  im.text = '\0';
  im.kind = f->bitdepthreal/8;
  im.array = (uint8*) malloc( n );
  if(f && im.array )
  { Seq_Read_Image_To_Buffer( f, 0, im.array );
    Write_Image( "seq_test3.tif", &im );
    free( im.array );
    return 1;
  }
  return 0;
}

int test4( char *path )
/* Test: Full stack read to buffer */
{ SeqReader *f = Seq_Open( path );
  size_t n = (f->sizebytes)*(f->nframes);
  Stack im;
  im.width = f->width;
  im.height = f->height;
  im.depth = f->nframes;
  im.text = '\0';
  im.kind = f->bitdepthreal/8;
  im.array = (uint8*) malloc( n );
  if(f && im.array )
  { Seq_Read_Stack_To_Buffer( f, im.array );
    Write_Stack( "seq_test4.tif", &im );
    free( im.array );
    return 1;
  }
  return 0;
}

int main( int argc, char* argv[] )
{ 
  printf("Testing single image read          : "); fflush(stdout);
  printf("%s\n", SEQ_TEST_RESULT( test1(argv[1])) );
  printf("Testing full stack read            : "); fflush(stdout); 
  printf("%s\n", SEQ_TEST_RESULT( test2(argv[1])) );
  printf("Testing single image read to buffer: "); fflush(stdout); 
  printf("%s\n", SEQ_TEST_RESULT( test3(argv[1])) );
  printf("Testing full stack read to buffer  : "); fflush(stdout); 
  printf("%s\n", SEQ_TEST_RESULT( test4(argv[1])) );

  printf("\nBe sure to check output tif files to make sure all the bits are where they should be!\n");
  return 0;
}
#endif
