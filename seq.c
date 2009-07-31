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
#include "error.h"
#include "utilities.h"
#include "image_lib.h"
#include "seq.h"

#undef    SEQ_TESTING
#define   SEQ_PARANOID

#ifdef SEQ_PARANOID
#define   SEQ_ASSERT(stmt)    if(stmt) goto error
#else
#define   SEQ_ASSERT(stmt)    stmt
#endif

SHARED_EXPORT
SeqReader *Seq_Open( const char* path )
{ FILE *fp;
  SeqReader *h;
  
  h = (SeqReader*) Guarded_Malloc( sizeof(SeqReader), "seq_open" );
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
	
  error( "Problem reading header of seq file\n" );
  if(h)  free(h);
  if(fp) fclose(fp);
  return NULL;
}

SHARED_EXPORT
void Seq_Close( SeqReader *h )
{ if(h)
  { if(h->fp) 
      fclose(h->fp);
    free(h);
  }
}

SHARED_EXPORT
Image *Seq_Read_Image( SeqReader *h, int index )
{ size_t offset = 1024 + index * h->truesize;
  Image *im = Make_Image( h->bitdepthreal/8, h->width, h->height );
  
  SEQ_ASSERT( fseek( h->fp, offset, SEEK_SET)                            );
  SEQ_ASSERT( fread( im->array, 1, h->sizebytes, h->fp ) != h->sizebytes ); 

  return im;
error:
  warning( "Seq reader: Couldn't read image at index %d\n",index);
  Free_Image( im );
  return NULL;
}

SHARED_EXPORT
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
    SEQ_ASSERT( fread( (uint8_t*)buffer + (count++)*dz_buf,1, dz_buf, r->fp ) != dz_buf   );
  } 

  return 0;
error:
  error( "Couldn't read stack.\n"
         "         Failed on %d.\n",i);     
  return 1;   
}

SHARED_EXPORT
int  Seq_Read_Image_To_Buffer ( SeqReader *h, int index, void *buffer )
{ size_t offset = 1024 + index * h->truesize;
  
  SEQ_ASSERT( fseek( h->fp, offset, SEEK_SET)                            );
  SEQ_ASSERT( fread( buffer   , 1, h->sizebytes, h->fp ) != h->sizebytes ); 
  return 0;
error:
  warning("Seq reader: Couldn't read image at index %d\n",index);
  return 1;   
}

SHARED_EXPORT
Image *Seq_Read_Image_Static_Storage( SeqReader *h, int index )
{ size_t offset = 1024 + index * h->truesize;
  static Image *im = NULL;

  if(!im)
  { im = Make_Image( h->bitdepthreal/8, h->width, h->height );
    if(!im) goto error;
  } else if( (h->width  != im->width  ) ||
             (h->height != im->height ) || 
             (h->bitdepthreal/8 != im->kind ) )
  { Free_Image(im); 
    im = NULL;
    im = Make_Image( h->bitdepthreal/8, h->width, h->height ); 
    if(!im) goto error;
  }
  
  SEQ_ASSERT( fseek( h->fp, offset, SEEK_SET)                            );
  SEQ_ASSERT( fread( im->array, 1, h->sizebytes, h->fp ) != h->sizebytes ); 

  return im;
error:
  warning("Seq reader: Couldn't read image at index %d\n",index);
  Free_Image( im );
  im = NULL;
  return NULL;
}

SHARED_EXPORT
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
  error( "Couldn't read stack.\n"
         "         Failed on %d.\n",i);
  Free_Stack( s );
  return NULL;
}

SHARED_EXPORT
int  Seq_Read_Stack_To_Buffer ( SeqReader *r, void *buffer )
{ size_t offset = 1024;
  size_t dz_buf, dz_seq;
  unsigned int i = 0;
  unsigned int w,h;
  dz_seq = r->truesize;
  dz_buf = r->sizebytes;  

  for( i=0; i < r->nframes; i++ )
  { SEQ_ASSERT( fseek( r->fp, 1024+i*dz_seq       , SEEK_SET ) );
    SEQ_ASSERT( fread( (uint8_t*)buffer +i*dz_buf,1, dz_buf, r->fp ) != dz_buf   );
  } 

  return 0;
error:
  error( "Couldn't read stack.\n"
         "         Failed on %d.\n",i);  
  return 1;   
}

SHARED_EXPORT
double Seq_Time_Stamp( SeqReader *h, int index )
{ size_t offset = 1024 + index * h->truesize + h->sizebytes;
  double t1,t2;

  SEQ_ASSERT( fseek( h->fp, offset, SEEK_SET)                     );
  SEQ_ASSERT( fread( &t1, 8, 1 , h->fp )                     != 1 );
  SEQ_ASSERT( fread( &t2, 8, 1 , h->fp )                     != 1 );
  return t1 + t2/1000.0;

error:
  error("Seq reader: Error reading time stamp at index %d\n", index );
  exit(1);
}

SHARED_EXPORT
double Seq_Time_From_Start( SeqReader *h, int index )
{ return Seq_Time_Stamp( h, index ) - h->starttime;
}

/*
 * TESTS
 */
#ifdef SEQ_TESTING
int test1( char *path )
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

int main( int argc, char* argv[] )
{ 
  printf("Testing single image read: %d\n", test1(argv[1]));
  printf("Testing full stack read  : %d\n", test2(argv[1]));
  return 0;
}
#endif
