#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utilities.h"
#include "image_lib.h"
#include "tiff_io.h"
#include "tiff_image.h"

static int Warnings = 1;

static void error(char *msg, char *arg)
{ fprintf(stderr,"\nError in TIFF library:\n   ");
  fprintf(stderr,msg,arg);
  fprintf(stderr,"\n");
  exit(1);
}

/*********** STACK PLANE SELECTION ****************************/

static char Empty_String[1] = { 0 };

Stack_Plane *Select_Plane(Stack *a_stack, int plane)  // Build an image for a plane of a stack
{ static Stack_Plane My_Image;

  if (plane < 0 || plane >= a_stack->depth)
    return (NULL);
  My_Image.kind   = a_stack->kind;
  My_Image.width  = a_stack->width;
  My_Image.height = a_stack->height;
  My_Image.text   = Empty_String;
  My_Image.array  = a_stack->array + plane*a_stack->width*a_stack->height*a_stack->kind;
  return (&My_Image);
}

int Set_Stack_Plane(Stack *stack, int plane, Image *image)
{ uint8 *ip, *sp;
  int    i;
  int    area;

  if (plane < 0 || plane >= stack->depth)
    return (1);
  if (image->width != stack->width || image->height != stack->height)
    return (1);
  if (image->kind != stack->kind)
    return (1);

  area = image->kind * image->width * image->height;

  ip = image->array;
  sp = stack->array + area*plane;
  for (i = 0; i < area; i++)
    *sp++ = *ip++;
  return (0);
}

/*********** SPACE MANAGEMENT ****************************/

// Raster working buffer

static uint32 *get_raster(int npixels, char *routine)
{ static uint32 *Raster = NULL;
  static int     Raster_Size = 0;                           //  Manage read work buffer

  if (npixels < 0)
    { free(Raster);
      Raster_Size = 0;
      Raster      = NULL;
    }
  else if (npixels > Raster_Size)
    { Raster_Size = npixels;
      Raster = (uint32 *) Guarded_Realloc(Raster,sizeof(uint32)*Raster_Size,routine);
    }
  return (Raster);
}

// Awk-generated (manager.awk) Image memory management

static inline int image_asize(Image *image)
{ return (image->height*image->width*image->kind); }

static inline int image_tsize(Image *image)
{ return (strlen(image->text)+1); }


typedef struct __Image
  { struct __Image *next;
    int             asize;
    int             tsize;
    Image           image;
  } _Image;

static _Image *Free_Image_List = NULL;
static int    Image_Offset, Image_Inuse;

static inline void allocate_image_array(Image *image, int asize, char *routine)
{ _Image *object  = (_Image *) (((char *) image) - Image_Offset);
  if (object->asize < asize)
    { object->image.array  = Guarded_Realloc(object->image.array,asize,routine);
      object->asize = asize;
    }
}

static inline void allocate_image_text(Image *image, int tsize, char *routine)
{ _Image *object  = (_Image *) (((char *) image) - Image_Offset);
  if (object->tsize < tsize)
    { object->image.text  = Guarded_Realloc(object->image.text,tsize,routine);
      object->tsize = tsize;
    }
}

static inline Image *new_image(int asize, int tsize, char *routine)
{ _Image *object;

  if (Free_Image_List == NULL)
    { object = (_Image *) Guarded_Malloc(sizeof(_Image),routine);
      Image_Offset = ((char *) &(object->image)) - ((char *) object);
      object->asize = 0;
      object->image.array = NULL;
      object->tsize = 0;
      object->image.text = NULL;
    }
  else
    { object = Free_Image_List;
      Free_Image_List = object->next;
    }
  Image_Inuse += 1;
  allocate_image_array(&(object->image),asize,routine);
  allocate_image_text(&(object->image),tsize,routine);
  return (&(object->image));
}

static inline Image *copy_image(Image *image)
{ _Image *object  = (_Image *) (((char *) image) - Image_Offset);
  Image *copy = new_image(image_asize(image),image_tsize(image),"Copy_Image");
  Image  temp = *copy;
  *copy = *image;
  copy->array = temp.array;
  if (image_asize(image) != 0)
    memcpy(copy->array,image->array,image_asize(image));
  copy->text = temp.text;
  if (image_tsize(image) != 0)
    memcpy(copy->text,image->text,image_tsize(image));
  return (copy);
}

Image *Copy_Image(Image *image)
{ return (copy_image(image)); }

static inline void pack_image(Image *image)
{ _Image *object  = (_Image *) (((char *) image) - Image_Offset);
  if (object->asize > image_asize(image))
    { object->asize = image_asize(image);
      if (object->asize != 0)
        object->image.array = Guarded_Realloc(object->image.array,
                                              object->asize,"Pack_Image");
      else
        object->image.array = NULL;
    }
  if (object->tsize > image_tsize(image))
    { object->tsize = image_tsize(image);
      if (object->tsize != 0)
        object->image.text = Guarded_Realloc(object->image.text,
                                             object->tsize,"Pack_Image");
      else
        object->image.text = NULL;
    }
}

void Pack_Image(Image *image)
{ pack_image(image); }

static inline void free_image(Image *image)
{ _Image *object  = (_Image *) (((char *) image) - Image_Offset);
  object->next = Free_Image_List;
  Free_Image_List = object;
  Image_Inuse -= 1;
}

void Free_Image(Image *image)
{ free_image(image); }

static inline void kill_image(Image *image)
{ _Image *object  = (_Image *) (((char *) image) - Image_Offset);
  if (image->text != NULL)
    free(image->text);
  if (image->array != NULL)
    free(image->array);
  free(((char *) image) - Image_Offset);
  Image_Inuse -= 1;
}

void Kill_Image(Image *image)
{ kill_image(image); }

static inline void reset_image()
{ _Image *object;
  while (Free_Image_List != NULL)
    { object = Free_Image_List;
      Free_Image_List = object->next;
      kill_image(&(object->image));
      Image_Inuse += 1;
    }
}

int Image_Usage()
{ return (Image_Inuse); }

void Reset_Image()
{ reset_image();
  get_raster(-1,NULL);
}

// Awk-generated (manager.awk) Stack memory management

static inline int stack_vsize(Stack *stack)
{ return (stack->depth*stack->height*stack->width*stack->kind); }

static inline int stack_tsize(Stack *stack)
{ return (strlen(stack->text)+1); }


typedef struct __Stack
  { struct __Stack *next;
    int             vsize;
    int             tsize;
    Stack           stack;
  } _Stack;

static _Stack *Free_Stack_List = NULL;
static int    Stack_Offset, Stack_Inuse;

static inline void allocate_stack_array(Stack *stack, int vsize, char *routine)
{ _Stack *object  = (_Stack *) (((char *) stack) - Stack_Offset);
  if (object->vsize < vsize)
    { object->stack.array  = Guarded_Realloc(object->stack.array,vsize,routine);
      object->vsize = vsize;
    }
}

static inline void allocate_stack_text(Stack *stack, int tsize, char *routine)
{ _Stack *object  = (_Stack *) (((char *) stack) - Stack_Offset);
  if (object->tsize < tsize)
    { object->stack.text  = Guarded_Realloc(object->stack.text,tsize,routine);
      object->tsize = tsize;
    }
}

static inline Stack *new_stack(int vsize, int tsize, char *routine)
{ _Stack *object;

  if (Free_Stack_List == NULL)
    { object = (_Stack *) Guarded_Malloc(sizeof(_Stack),routine);
      Stack_Offset = ((char *) &(object->stack)) - ((char *) object);
      object->vsize = 0;
      object->stack.array = NULL;
      object->tsize = 0;
      object->stack.text = NULL;
    }
  else
    { object = Free_Stack_List;
      Free_Stack_List = object->next;
    }
  Stack_Inuse += 1;
  allocate_stack_array(&(object->stack),vsize,routine);
  allocate_stack_text(&(object->stack),tsize,routine);
  return (&(object->stack));
}

static inline Stack *copy_stack(Stack *stack)
{ _Stack *object  = (_Stack *) (((char *) stack) - Stack_Offset);
  Stack *copy = new_stack(stack_vsize(stack),stack_tsize(stack),"Copy_Stack");
  Stack  temp = *copy;
  *copy = *stack;
  copy->array = temp.array;
  if (stack_vsize(stack) != 0)
    memcpy(copy->array,stack->array,stack_vsize(stack));
  copy->text = temp.text;
  if (stack_tsize(stack) != 0)
    memcpy(copy->text,stack->text,stack_tsize(stack));
  return (copy);
}

Stack *Copy_Stack(Stack *stack)
{ return (copy_stack(stack)); }

static inline void pack_stack(Stack *stack)
{ _Stack *object  = (_Stack *) (((char *) stack) - Stack_Offset);
  if (object->vsize > stack_vsize(stack))
    { object->vsize = stack_vsize(stack);
      if (object->vsize != 0)
        object->stack.array = Guarded_Realloc(object->stack.array,
                                              object->vsize,"Pack_Stack");
      else
        object->stack.array = NULL;
    }
  if (object->tsize > stack_tsize(stack))
    { object->tsize = stack_tsize(stack);
      if (object->tsize != 0)
        object->stack.text = Guarded_Realloc(object->stack.text,
                                             object->tsize,"Pack_Stack");
      else
        object->stack.text = NULL;
    }
}

void Pack_Stack(Stack *stack)
{ pack_stack(stack); }

static inline void free_stack(Stack *stack)
{ _Stack *object  = (_Stack *) (((char *) stack) - Stack_Offset);
  object->next = Free_Stack_List;
  Free_Stack_List = object;
  Stack_Inuse -= 1;
}

void Free_Stack(Stack *stack)
{ free_stack(stack); }

static inline void kill_stack(Stack *stack)
{ _Stack *object  = (_Stack *) (((char *) stack) - Stack_Offset);
  if (stack->text != NULL)
    free(stack->text);
  if (stack->array != NULL)
    free(stack->array);
  free(((char *) stack) - Stack_Offset);
  Stack_Inuse -= 1;
}

void Kill_Stack(Stack *stack)
{ kill_stack(stack); }

static inline void reset_stack()
{ _Stack *object;
  while (Free_Stack_List != NULL)
    { object = Free_Stack_List;
      Free_Stack_List = object->next;
      kill_stack(&(object->stack));
      Stack_Inuse += 1;
    }
}

int Stack_Usage()
{ return (Stack_Inuse); }

void Reset_Stack()
{ reset_stack();
  get_raster(-1,NULL);
}


/*********** TIFF INTERFACE ****************************/

TIFF *Open_Tiff(char *file_name, char *mode)
{ TIFF *tif = Guarded_Malloc( sizeof(TIFF), "Open_Tiff" );
	int read = 0;
	int write = 0;
	char *c;
  int endian;

	for( c = mode; *c != '\0'; c++ )
	{	if( *c == 'r' ) read = 1;
		if( *c == 'w' ) write = 1;
	}

  tif->reader = NULL;
  tif->writer = NULL;

  if(read)
	{ tif->reader = Open_Tiff_Reader( file_name, &endian, 0);
		if( !tif->reader )
			error("Cannot open TIFF file %s for reading", file_name);
	}
  if(write)
	{ tif->writer = Open_Tiff_Writer( file_name, 0);
		if( !tif->writer )
			error("Cannot open TIFF file %s for reading", file_name);
	}
  
  return (tif);
}    

Tiff_Image *Cast_Image_To_Tiff_Image(Image *im)
{ static Tiff_Image   *tim = NULL;
  static Tiff_Channel *tch = NULL;
  
  if( !tim )
  { tim = Guarded_Malloc( sizeof(Tiff_Image),              "Cast_Image_To_Tiff_Image" );
    tch = Guarded_Malloc( sizeof(Tiff_Channel),            "Cast_Image_To_Tiff_Image" ); 
    tim->channels = Guarded_Malloc( sizeof(Tiff_Channel*), "Cast_Image_To_Tiff_Image" );
    tim->channels[0] = tch;
    tim->number_channels = 1;
  }

  tim->width  = tch->width  = im->width;
  tim->height = tch->height = im->height;
  tch->interpretation       = CHAN_BLACK;
  tch->scale                = 8*im->kind;
  tch->bytes_per_pixel      = im->kind;
  tch->type                 = CHAN_UNSIGNED;
  tch->plane                = im->array;
  tch->histogram            = NULL;
  
  return (tim);
}

Image *Cast_Tiff_Image_To_Image(Tiff_Image *tim)
{ static Image *image = NULL;

  if( !image )
    image = Guarded_Malloc( sizeof(Image) , "Cast_Tiff_Image_To_Image" );

  image->array  = tim->channels[0]->plane;
  image->width  = tim->width;
  image->height = tim->height;
  image->kind   = tim->channels[0]->bytes_per_pixel;
  image->text   = NULL;

  return (image);
}

Image *Read_Tiff(TIFF *tif, int *lastone)
{ Image *image;
  Tiff_IFD *ifd = Read_Tiff_IFD( tif->reader );
	Tiff_Image *tim = Extract_Image_From_IFD(ifd);

	if( tim->number_channels < 1 )
		error("Could not read any channels in tif image.", NULL);

	/* map Tiff_Image to Image */
  image = Cast_Tiff_Image_To_Image(tim); /* This returns a reference to*/
                                         /* a static image.            */
                                         /* This will cause problems   */
  *lastone = End_Of_Tiff( tif );
  return (image);
}

void Write_Tiff(TIFF *tif, Image *a_image)
{ Tiff_Image   *tim;
	Tiff_Channel *tch;
	Tiff_IFD     *ifd;
	void *temp = NULL;

	if( !tif->writer )
		error("No writer opened for this tif.",NULL);

	if( !(tim = Create_Tiff_Image( a_image->width, a_image->height ) ) )
			error("Faled to create tiff image.",NULL);
	if( Add_Tiff_Image_Channel( tim, CHAN_BLACK, 8*a_image->kind, CHAN_UNSIGNED ) )
		error("Failed to add image channel.",NULL);
	tch = tim->channels[0];
	
	temp = tch->plane; /* Not sure this (cludge) is neccessary.            */
						         /* When the Channel gets added, a buffer for the    */
	                   /* image gets allocated.  That's not neccessary     */
										 /* since the bits are already in the input image.   */
										 /* So, we temporarily reassign the plane pointer to */
										 /* the input image data.  Later the pointer gets    */
										 /* reassigned to the (empty) allocated buffer so    */
										 /* the Channel's memory management doesn't get      */
										 /* confused. - Nathan -                             */
	
	tch->plane = (void*) (a_image->array);

  ifd = Make_IFD_For_Image( tim, 0 /*no compression*/ );
	
  if( Write_Tiff_IFD( tif->writer, ifd ) )
		error("Problem on writing image.",NULL);

	/* cleanup */
	tch->plane = temp;
	Free_Tiff_Image( tim ); /* this also frees the channel */
	
}

void Close_Tiff(TIFF *tif)
{ if( tif->writer ) 
	{ Close_Tiff_Writer (tif->writer);
		Free_Tiff_Writer  (tif->writer);
	}
  if( tif->reader )
    Free_Tiff_Reader  (tif->reader);

  free( tif );
}


/*********** READ + WRITE INTERFACE ****************************/

File_Bundle *Parse_Stack_Name(char *file_name)
{ static File_Bundle my_bundle;

  static char *Prefix = NULL;
  static int   Prefix_Max = 0;

  char *s, *t, c;

  s = file_name + strlen(file_name) - 4;
  if (strcmp(s,".tif") != 0 && strcmp(s,".TIF") != 0)
    error("1st file, %s, in stack does not have .tif extension",file_name);
  t = s;
  while (t > file_name && isdigit(t[-1]))
    t -= 1;
  if (s-t <= 0)
    error("No number sequence in stack file names %s",file_name);

  if (t-file_name > Prefix_Max)
    { Prefix_Max = (t-file_name)*1.2 + 20;
      Prefix     = (char *) Guarded_Realloc(Prefix,Prefix_Max+1,"Parse_Stack_Name");
    }

  c = *t;
  *t = '\0';
  strcpy(Prefix,file_name);
  *t = c;

  my_bundle.prefix    = Prefix;
  my_bundle.num_width = s-t;
  my_bundle.first_num = atoi(t);
  return (&my_bundle);
}

Image *Read_Image(char *file_name)
{ TIFF  *tif;
  Image *img;
  int    lastone;

  tif = Open_Tiff(file_name,"r");

  img = Read_Tiff(tif,&lastone);

  Close_Tiff(tif);

  return (img);
}

Stack *Read_Stack(char *file_name)
{ Stack       *stack;
  Tiff_IFD    *ifd;
  Tiff_Image  *tim;
  Tiff_Reader *tif;
  int          depth, width, height, kind,endian;
  char        *text;

  tif = Open_Tiff_Reader(file_name,&endian,0);
  depth = 0;
  while ( !Advance_Tiff_Reader(tif)  )
    depth += 1;
  Free_Tiff_Reader( tif );

  tif = Open_Tiff_Reader(file_name,&endian,0);
  ifd = Read_Tiff_IFD( tif );
  tim = Extract_Image_From_IFD( ifd );
  
  width  = tim->width;
  height = tim->height;
  
  text = Empty_String;

  kind  = tim->channels[0]->bytes_per_pixel;
  stack = new_stack(depth*height*width*kind,strlen(text)+1,"Read_Stack");

  stack->width  = width;
  stack->height = height;
  stack->depth  = depth;
  stack->kind   = kind;
  strcpy(stack->text,text);
                     /* copy in data */
  { int d;

    d = 0;
    while (1)
    { Set_Stack_Plane( stack, d++, Cast_Tiff_Image_To_Image( tim ) );

      Free_Tiff_Image(tim);
      Free_Tiff_IFD(ifd);

      if( End_Of_Tiff( tif ) ) break;
      
      ifd = Read_Tiff_IFD( tif );
      tim = Extract_Image_From_IFD( ifd );

      width  = tim->width;
      height = tim->height;
      
      if (width != stack->width || height != stack->height)
        error("Images of stack are not of the same dimensions!",NULL);

      kind = tim->channels[0]->bytes_per_pixel;
      if (kind != stack->kind)
        error("Images of stack are not of the same type (GREY, GREY16, or COLOR)!",NULL);
    }
  }

  Free_Tiff_Reader(tif);
  Free_Tiff_Image(tim);
  Free_Tiff_IFD(ifd);

  return (stack);
}

Stack *Read_Stack_Planes(File_Bundle *bundle)
{ Stack *stack;

  char           sname[1000];
  int            width, height, depth, kind,endian;
  Tiff_Reader   *tif;
  Tiff_IFD      *ifd;
  Tiff_Image    *tim;
  char          *text;

  depth = 0;
  while (1)
    { FILE *fd;

      sprintf(sname,"%s%0*d.tif",bundle->prefix,bundle->num_width,bundle->first_num+depth);
      if ((fd = fopen(sname,"r")) == NULL)
        break;
      fclose(fd);

      depth += 1;
    }

  sprintf(sname,"%s%0*d.tif",bundle->prefix,bundle->num_width,bundle->first_num);
  tif = Open_Tiff_Reader(sname,&endian,0); 
  ifd = Read_Tiff_IFD( tif );
  tim = Extract_Image_From_IFD( ifd );
  width = tim->width;
  height = tim->height;

  text = Empty_String;

  kind  = tim->channels[0]->bytes_per_pixel;
  stack = new_stack(depth*height*width*kind,strlen(text)+1,"Read_Stack_Planes");

  stack->width  = width;
  stack->height = height;
  stack->depth  = depth;
  stack->kind   = kind;
  strcpy(stack->text,text);

  { int d;

    d = 0;
    while (1)
      { Set_Stack_Plane( stack, d++, Cast_Tiff_Image_To_Image( tim ) );

        Free_Tiff_Image ( tim );
        Free_Tiff_IFD   ( ifd );     
        Free_Tiff_Reader( tif );
       
        if (d >= depth) break;

        sprintf(sname,"%s%0*d.tif",bundle->prefix,bundle->num_width,bundle->first_num+d);
        tif = Open_Tiff_Reader(sname,&endian,0);
        ifd = Read_Tiff_IFD( tif );
        tim = Extract_Image_From_IFD( ifd );
        width = tim->width;
        height = tim->height;
        
        if (width != stack->width || height != stack->height)
          error("Images of stack are not of the same dimensions!",NULL);

        kind = tim->channels[0]->bytes_per_pixel;
        if (kind != stack->kind)
          error("Images of stack are not of the same type (GREY, GREY16, or COLOR)!",NULL);
      }
  }

  return (stack);
}

void Write_Image(char *file_name, Image *a_image)
{ Tiff_Writer *tif;
  Tiff_Image  *tim = Cast_Image_To_Tiff_Image( a_image );
  Tiff_IFD    *ifd = Make_IFD_For_Image( tim, 0 /*no compression*/);

  tif = Open_Tiff_Writer(file_name, 0);
  if( Write_Tiff_IFD( tif, ifd ) )
    error("Warning: There was a problem with writing to %s", file_name);
  
  Close_Tiff_Writer( tif );
  Free_Tiff_Writer ( tif );
  Free_Tiff_IFD( ifd );
}

void Write_Stack(char *file_name, Stack *a_stack)
{ TIFF *tif;
  int   i;

  tif = Open_Tiff(file_name,"w");
  for (i = 0; i < a_stack->depth; i++)
    Write_Tiff(tif,Select_Plane(a_stack,i));
  Close_Tiff(tif);
}

void Write_Stack_Planes(File_Bundle *bundle, Stack *a_stack)
{ char  *name;
  Image *plane;
  int    n;

  name = (char *) Guarded_Malloc(strlen(bundle->prefix)+50,"Write_Stack_Planes");
  for (n = 0; n < a_stack->depth; n++)
    { sprintf(name,"%s.%0*d.tif",bundle->prefix,bundle->num_width,bundle->first_num+n);
      plane = Select_Plane(a_stack,n);
      if (n == 0)
        plane->text = a_stack->text;
      Write_Image(name,plane);
    }
  free(name);
}


/*********** MODIFY IMAGE/TEXT DESCRIPTIONS ****************************/

void Set_Image_Text(Image *image, char *text)
{ _Image *object = (_Image *) (((char *) image) - Image_Offset);
  int     len    = strlen(text)+1;

  image->text = Guarded_Realloc(image->text,len,"Set_Image_Text");
  strcpy(image->text,text);
  object->tsize = len;
}

void Append_To_Image_Text(Image *image, char *text)
{ _Image *object = (_Image *) (((char *) image) - Image_Offset);
  int     sen    = strlen(image->text);
  int     len    = sen + strlen(text)+1;

  image->text = Guarded_Realloc(image->text,len,"Append_To_Image_Text");
  strcpy(image->text+sen,text);
  object->tsize = len;
}

void Set_Stack_Text(Stack *stack, char *text)
{ _Stack *object = (_Stack *) (((char *) stack) - Stack_Offset);
  int     len    = strlen(text)+1;

  stack->text = Guarded_Realloc(stack->text,len,"Set_Stack_Text");
  strcpy(stack->text,text);
  object->tsize = len;
}

void Append_To_Stack_Text(Stack *stack, char *text)
{ _Stack *object = (_Stack *) (((char *) stack) - Stack_Offset);
  int     sen    = strlen(stack->text);
  int     len    = sen + strlen(text)+1;

  stack->text = Guarded_Realloc(stack->text,len,"Append_To_Stack_Text");
  strcpy(stack->text+sen,text);
  object->tsize = len;
}

/*********** MAKE (EMPTY) IMAGES AND STACKS ****************************/

Image *Make_Image(int kind, int width, int height)
{ Image *image;

  image = new_image(height*width*kind,1,"Make_Image");

  image->width   = width;
  image->height  = height;
  image->kind    = kind;
  image->text[0] = '\0';

  return (image);
}

Stack *Make_Stack(int kind, int width, int height, int depth)
{ Stack *stack;

  stack = new_stack(depth*height*width*kind,1,"Make_Stack");

  stack->width   = width;
  stack->height  = height;
  stack->depth   = depth;
  stack->kind    = kind;
  stack->text[0] = '\0';

  return (stack);
}


/*********** COMPUTE RANGES AND SCALE IMAGES AND STACKS *********************/

//  Compute min and max values in 'array' of type 'kind' with 'length' elements

static Pixel_Range *compute_minmax(uint8 *array, int kind, int length, int channel)
{ static Pixel_Range My_Range;
  int    i;

  if (kind == FLOAT32)
    { float32 *array32 = (float32 *) array;
      float    x, min, max;

      min = max = array32[0];
      for (i = 0; i < length; i++)
        { x = array32[i];
          if (x < min)
            min = x;
          else if (x > max)
            max = x;
        }
      My_Range.maxval = max;
      My_Range.minval = min;
    }
  else
    { int x, min, max;

      if (kind == GREY16)
        { uint16 *array16 = (uint16 *) array;
          min = max = array16[0];
          for (i = 0; i < length; i++)
            { x = array16[i];
              if (x < min)
                min = x;
              else if (x > max)
                max = x;
            }
        }
      else
        { if (kind == COLOR)
            { length *= 3;
              if (channel > 2)
                kind = 1;
              else
                array += channel;
            }
          min = max = array[0];
          for (i = 0; i < length; i += kind)
            { x = array[i];
              if (x < min)
                min = x;
              else if (x > max)
                max = x;
            }
        }
      My_Range.maxval = max;
      My_Range.minval = min;
    }

  return (&My_Range);
}

Pixel_Range *Image_Range(Image *image, int channel)
{ static Pixel_Range My_Range;
  My_Range = *compute_minmax(image->array,image->kind,image->width*image->height,channel);
  return (&My_Range);
}

Pixel_Range *Stack_Range(Stack *stack, int channel)
{ static Pixel_Range My_Range;
  My_Range = *compute_minmax(stack->array,stack->kind,
                             stack->width*stack->height*stack->depth,channel);
  return (&My_Range);
}

//  Scale values in 'array' of type 'kind' with 'length' elements by factor*(x)+offset

static void scale_values(uint8 *array, int kind, int length, int channel,
                         double factor, double offset)
{ int    i;

  if (kind == FLOAT32)
    { float32 *array32 = (float32 *) array;

      for (i = 0; i < length; i++)
        array32[i] = factor * (array32[i] + offset);
    }
  else if (kind == GREY16)
    { uint16 *array16 = (uint16 *) array;
      for (i = 0; i < length; i++)
        array16[i] = (uint16) (factor * (array16[i] + offset));
    }
  else
    { if (kind == COLOR)
        { length *= 3;
          if (channel > 2)
            kind = 1;
          else
            array += channel;
        }
      for (i = 0; i < length; i += kind)
        array[i] = (uint8) (factor * (array[i] + offset));
    }
}

void Scale_Image(Image *image, int channel, double factor, double offset)
{ scale_values(image->array,image->kind,image->width*image->height,channel,factor,offset); }

void Scale_Stack(Stack *stack, int channel, double factor, double offset)
{ scale_values(stack->array,stack->kind,stack->width*stack->height*stack->depth,
               channel,factor,offset);
}

void Scale_Image_To_Range(Image *image, int channel, double min, double max)
{ Pixel_Range crn; 
  crn = *compute_minmax(image->array,image->kind,image->width*image->height,channel);
  if (crn.maxval == crn.minval)
    { fprintf(stderr,"Warning: image is monotone and so cannot be scaled!\n");
      return;
    }
  Scale_Image(image,channel,(max-min)/(crn.maxval-crn.minval),min-1.*crn.minval);
}

void Scale_Stack_To_Range(Stack *stack, int channel, double min, double max)
{ Pixel_Range crn; 
  crn = *compute_minmax(stack->array,stack->kind,stack->width*stack->height*stack->depth,channel);
  if (crn.maxval == crn.minval)
    { fprintf(stderr,"Warning: stack is monotone and so cannot be scaled!\n");
      return;
    }
  Scale_Stack(stack,channel,(max-min)/(crn.maxval-crn.minval),min-1.*crn.minval);
}


/*********** CONVERT IMAGES AND STACKS  *********************/

static void translate(int skind, uint8 *in8, int tkind, uint8 *out8, int length)
{ uint16  *in16, *out16;
  float32 *in32, *out32;
  int     i, x; 
  double  c, scale, maxval;

  if (skind == GREY16 || skind == FLOAT32)
    { maxval = compute_minmax(in8,skind,length,0)->maxval;
      if (tkind == GREY16 && maxval > 65535.)
        scale  = 65535. / maxval;
      else if ((tkind == GREY || tkind == COLOR) && maxval > 255.)
        scale  = 255. / maxval;
      else
        scale  = 1.;
    }

  if (tkind > skind)
    { in8  += length*skind;
      out8 += length*tkind;
    }
  in16  = (uint16  *) in8;
  out16 = (uint16  *) out8;
  in32  = (float32 *) in8;
  out32 = (float32 *) out8;
    
     
  if (tkind == COLOR)
    if (skind == GREY)
      for (i = length; i > 0; i--)   // G->C
        { x = *--in8;
          *--out8 = x;
          *--out8 = x;
          *--out8 = x;
        }
    else if (skind == GREY16)
      for (i = length; i > 0; i--)   // G16->C
        { x = (*--in16) * scale;
          *--out8 = x;
          *--out8 = x;
          *--out8 = x;
        }
    else
      for (i = length; i > 0; i--)   // F32->C
        { x = (*in32++) * scale;
          *out8++ = x;
          *out8++ = x;
          *out8++ = x;
        }

  else if (tkind == GREY16)
    if (skind == COLOR)
      for (i = length; i > 0; i--)   // C->G16
        { c  = .3 * (*in8++);
          c += .59 * (*in8++);
          c += .11 * (*in8++);
          *out16++ = (uint16) c;
        }
    else if (skind == GREY)
      for (i = length; i > 0; i--)   // G->G16
        { *--out16 = *--in8; }
    else
      for (i = length; i > 0; i--)   // F32->G16
        { *out16++ = scale * (*in32++); }

  else if (tkind == GREY)
    if (skind == COLOR)
      for (i = length; i > 0; i--)   // C->G
        { c  = .3 * (*in8++);
          c += .59 * (*in8++);
          c += .11 * (*in8++);
          *out8++ = (uint8) c;
        }
    else if (skind == GREY16)
      for (i = length; i > 0; i--)   // G16->G
        { *out8++ = (*in16++) * scale; }
    else
      for (i = length; i > 0; i--)   // F32->G
        { *out8++ = (*in32++) * scale; }

  else // tkind == FLOAT32
    if (skind == COLOR)
      for (i = length; i > 0; i--)   // C->F32
        { c  = .3 * (*--in8);
          c += .59 * (*--in8);
          c += .11 * (*--in8);
          *--out32 = c;
        }
    else if (skind == GREY16)
      for (i = length; i > 0; i--)   // G16->F32
        { *--out32 = *--in16; }
    else
      for (i = length; i > 0; i--)   // G->F32
        { *--out32 = *--in8; }
}

Image *Translate_Image(Image *image, int kind, int in_place)
{ int width, height;

  width  = image->width;
  height = image->height;

  /* BUG ?
  ** ngc: something funny happens when doing a translation in place on an image
  **      that has been selected from a stack without copying.  The
  **      image->width gets replaced by the object->asize
  */
  if (in_place)
    { if (image->kind == kind)
        return (image);

      if (kind > image->kind)
        { _Image *object  = (_Image *) (((char *) image) - Image_Offset);
          if (object->asize < width * height * kind)
            { object->asize = width * height * kind;
              image->array  = Guarded_Realloc(image->array,object->asize,"Translate_Image");
            }
        }

      translate(image->kind,image->array,kind,image->array,width*height);

      image->kind = kind;

      return (image);
    }
  else
    { Image  *xlate;

      if (image->kind == kind)
        return (Copy_Image(image));

      xlate = new_image(kind*width*height,0,"Translate_Image");
      xlate->width   = width;
      xlate->height  = height;
      xlate->kind    = kind;
      if( xlate->text)           /* FIX by ngc */ 
        xlate->text[0] = '\0';

      translate(image->kind,image->array,kind,xlate->array,width*height);

      return (xlate);
    }
}

Stack *Translate_Stack(Stack *stack, int kind, int in_place)
{ int width, height, depth;

  width  = stack->width;
  height = stack->height;
  depth  = stack->depth;

  if (in_place)
    { if (stack->kind == kind)
        return (stack);

      if (kind > stack->kind)
        { _Stack *object  = (_Stack *) (((char *) stack) - Stack_Offset);

          if (object->vsize < width * height * depth * kind)
            { object->vsize = width * height * depth * kind;
              stack->array  = Guarded_Realloc(stack->array,object->vsize,"Translate_Stack");
            }
        }

      translate(stack->kind,stack->array,kind,stack->array,width*height*depth);

      stack->kind = kind;

      return (stack);
    }
  else
    { Stack *xlate;

      if (stack->kind == kind)
        return (Copy_Stack(stack));

      xlate = new_stack(kind*width*height*depth,1,"Translate_Stack");
      xlate->depth   = depth;
      xlate->width   = width;
      xlate->height  = height;
      xlate->kind    = kind;
      xlate->text[0] = '\0';

      translate(stack->kind,stack->array,kind,xlate->array,width*height*depth);

      return (xlate);
    }
}

//  Truncate values less than ceiling to the value ceiling

static void truncate_values(uint8 *array, int kind, int length, int channel, double ceiling)
{ int    i;
  double x;

  if (kind == FLOAT32)
    { float32 *array32 = (float32 *) array;

      for (i = 0; i < length; i++)
        { x = array32[i];
          if (x < ceiling)
            array32[i] = ceiling;
        }
    }
  else if (kind == GREY16)
    { uint16 *array16 = (uint16 *) array;
      uint16  ceil16  = ceiling;
      for (i = 0; i < length; i++)
        { x = array16[i];
          if (x < ceiling)
            array16[i] = ceil16;
        }
    }
  else
    { uint8  ceil8  = ceiling;
      if (kind == COLOR)
        { length *= 3;
          if (channel > 2)
            kind = 1;
          else
            array += channel;
        }
      for (i = 0; i < length; i += kind)
        { x = array[i];
          if (x < ceiling)
            array[i] = ceil8;
        }
    }
}

void Truncate_Image(Image *image, int channel, double ceiling)
{ truncate_values(image->array,image->kind,image->width*image->height,channel,ceiling); }

void Truncate_Stack(Stack *stack, int channel, double ceiling)
{ truncate_values(stack->array,stack->kind,stack->width*stack->height*stack->depth,
                  channel,ceiling);
}

//  Threshold values less than cutoff to black, all others to white

static void threshold_values(uint8 *array, int kind, int length, int channel, double cutoff)
{ int    i;
  double x;

  if (kind == FLOAT32)
    { float32 *array32 = (float32 *) array;

      for (i = 0; i < length; i++)
        { x = array32[i];
          if (x < cutoff)
            array32[i] = 0.;
          else
            array32[i] = 1.;
        }
    }
  else if (kind == GREY16)
    { uint16 *array16 = (uint16 *) array;
      for (i = 0; i < length; i++)
        { x = array16[i];
          if (x < cutoff)
            array16[i] = 0;
          else
            array16[i] = 0xFFFF;
        }
    }
  else
    { if (kind == COLOR)
        { length *= 3;
          if (channel > 2)
            kind = 1;
          else
            array += channel;
        }
      for (i = 0; i < length; i += kind)
        { x = array[i];
          if (x < cutoff)
            array[i] = 0;
          else
            array[i] = 0xFF;
        }
    }
}

void Threshold_Image(Image *image, int channel, double cutoff)
{ threshold_values(image->array,image->kind,image->width*image->height,channel,cutoff); }

void Threshold_Stack(Stack *stack, int channel, double cutoff)
{ threshold_values(stack->array,stack->kind,stack->width*stack->height*stack->depth,
                  channel,cutoff);
}
