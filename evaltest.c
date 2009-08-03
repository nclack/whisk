#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include "image_lib.h"
#include "tiff_io.h"
#include "tiff_image.h"
#include "eval.h"
#define _USE_MATH_DEFINES
#include <math.h> //for testing

//typedef unsigned char uint8;
typedef struct _rgb{uint8 r; uint8 g; uint8 b;} rgb;
typedef struct _hsv{uint8 h; uint8 s; uint8 v;} hsv;
//typedef struct {float x; float y;} point;

void hsv2rgb( hsv *im, rgb *r    )
{ float h, s, v;
  h = im->h / 255.; // maps to [0,1)
  s = im->s / 255.; // maps to [0,1) 
  v = im->v;

  if( s < 0.001 )   // low saturation so just gray value
  { rgb tr = {v,v,v};
    *r = tr;
  }
  else
  { uint8 a,b,c;  
    float f;
    h *= 6.0;
    f = h - floor(h);
    a = v * (1.0 - s);
    b = v * (1.0 - s*f);
    c = v * (1.0 - s*(1.0-f));
    v = v;
    switch ((int) h) {
      case 0: 
        { rgb tr = {v, c, a}; *r = tr; }
        break;
      case 1: 
        { rgb tr = {b, v, a}; *r = tr; }
        break;
      case 2: 
        { rgb tr = {a, v, c}; *r = tr; }
        break;
      case 3: 
        { rgb tr = {a, b, v}; *r = tr; }
        break;
      case 4: 
        { rgb tr = {c, a, v}; *r = tr; }
        break;
      case 5: 
        { rgb tr = {v, a, b}; *r = tr; }
        break;
    }
  }
  return;
}

inline void blend_rgb_ip( rgb *im, rgb *b, float a )
{ im->r = (uint8)( (im->r * (1.0-a)) + b->r * a );
  im->g = (uint8)( (im->g * (1.0-a)) + b->g * a );
  im->b = (uint8)( (im->b * (1.0-a)) + b->b * a );
  return;
}

void write_tiff_stack_rgb( char *name, rgb* buf, int width, int height, int depth )
{ int iframe;
  Tiff_Writer *tiff  = Open_Tiff_Writer(name,0);
  Tiff_Image  *plane = Create_Tiff_Image( width, height );
  void *tr,*tb,*tg;

  Add_Tiff_Image_Channel( plane, CHAN_RED,    8, CHAN_UNSIGNED );
  Add_Tiff_Image_Channel( plane, CHAN_GREEN,  8, CHAN_UNSIGNED );
  Add_Tiff_Image_Channel( plane, CHAN_BLUE,   8, CHAN_UNSIGNED );

  for( iframe = 0; iframe < depth; iframe++ )
  { uint8 *dest;
    rgb  *im, *src;

    /* Select Plane and copy out to channels of Tiff_Image */
    im = buf + iframe * width * height;
    /* red */
    src  =            im                        + width*height  ;
    dest = (uint8*) ( plane->channels[0]->plane)+ width*height  ;
    while( --src >= im ) 
      *(--dest) = src->r;
    /* green */
    src  =            im                        + width*height  ;
    dest = (uint8*) ( plane->channels[1]->plane)+ width*height  ;
    while( --src >= im ) 
      *(--dest) = src->g;
    /* blue  */
    src  =            im                        + width*height  ;
    dest = (uint8*) ( plane->channels[2]->plane)+ width*height  ;
    while( --src >= im ) 
      *(--dest) = src->b;
      
    { Tiff_IFD *ifd = Make_IFD_For_Image( plane, 0 );
      Write_Tiff_IFD(tiff, ifd );
      Free_Tiff_IFD(ifd);
    }
  }

  Free_Tiff_Image( plane );

  Close_Tiff_Writer(tiff);
  Free_Tiff_Writer(tiff);
}

#ifdef EVAL_TEST_1
#define WIDTH   100
#define HEIGHT  100
#define NFRAMES 50 
#define N       20
int main(int argc, char* argv[])
{ float grid[ WIDTH*HEIGHT ];
  int strides[3] = { WIDTH*HEIGHT, WIDTH, 1 }; // for grid which is 1 channel
  uint8 *image, stack[ 3*WIDTH*HEIGHT*NFRAMES ];
  int iframe;
  float poly[2*N];

  memset(stack, 0, 3*WIDTH*HEIGHT*NFRAMES );


  iframe = NFRAMES;
  while(iframe--)
  { float sc = 80.0       *  (NFRAMES-iframe) / (float) NFRAMES;
    float tr = 40.0       *  (        iframe) / (float) NFRAMES; 
    float rr = (M_PI/2.0) *  (        iframe) / (float) NFRAMES; 
    printf("frame: %d\n",iframe);
    image = stack + iframe*3*WIDTH*HEIGHT;
    { int i = 2*N;
      while( i )
      { poly[--i] = sin(2.*M_PI*i/(2.0*N));
        poly[--i] = cos(2.*M_PI*(0.2  + (i+1)/(2.0*N)));
      }
    }   
//  poly[0] = -1.0;   poly[1] = -0.005;
//  poly[2] = -1.0;   poly[3] =  0.005;
//  poly[4] =  1.0;   poly[5] =  0.005;
//  poly[6] =  1.0;   poly[7] = -0.005;

    scale(    (point*) poly, N, sc ); 
    rotate(   (point*) poly, N, rr );
    { point shift = {  tr + WIDTH/2.0,  tr + HEIGHT/2.0 };
      translate( (point*) poly, N,  shift ); 
    }

    { float th;
      //hsv px_hsv;
      rgb px_rgb;
      for( th = 0; th < M_PI; th += M_PI/8.0)
      { const point shift = {  -tr -WIDTH/2.0, -tr  -HEIGHT/2.0 },
                    ishift = {   tr +WIDTH/2.0,  tr + HEIGHT/2.0 };
        translate( (point*) poly, N, shift ); 
        rotate(    (point*) poly, N, th );
        translate( (point*) poly, N, ishift ); 

        memset(grid, 0, sizeof(float)*WIDTH*HEIGHT );
        Compute_Pixel_Overlap( poly, N, 1.0, grid, strides );

        { int i = WIDTH*HEIGHT;
          hsv px_hsv = {  255 * th / M_PI       , //255 * fabs(grid[i]);  
                          255                   ,  
                          255                   };
          hsv2rgb( &px_hsv, &px_rgb );
          while(i--)
            blend_rgb_ip( (rgb*)(image+3*i), &px_rgb, 0.7*fabs(grid[i]));
        }
      }
    }
  }
  write_tiff_stack_rgb( "test.tif", (rgb*) stack, WIDTH, HEIGHT, NFRAMES);
  return 0;
}
#endif //EVAL_TEST_1

#ifdef EVAL_TEST_2
int main(int argc, char* argv[])
{ Image *im = Make_Image( FLOAT32, 101, 101);
  int strides[3] = {101*101,101,1};
  point p = { 50.5, 50.5 };
  memset( im->array, 0, im->kind * im->width * im->height );
  Render_Line_Detector( 0.1, 8.0, 0.0, 2.0,
      p,
      (float*) im->array, strides );
  Scale_Image_To_Range(im, 0, 0, 255);
  im = Translate_Image(im, GREY8, 1);
  Write_Image("evaltest2.tif",im);
  Free_Image(im);
  return 0;
}
#endif //EVAL_TEST_2                  

#ifdef EVAL_TEST_3
int main(int argc, char* argv[])
{ Range params[3] = { {0.0, 1.0,  0.2},
                      {0.5, 6.5,  0.4},
                      {-M_PI/4.0, M_PI/4.0, M_PI/36.0} };
  Array *bank = Build_Line_Detectors( params[0], params[1], params[2],
                                      7, 2*7+3 );
  { float *p,*d,sum=0.0; 
    d = (float*) bank->data;
    p = d + bank->strides_px[0];
    while( p-- > d )
      sum += *p;
    printf("sum: %g\n",sum);
    if( sum > 0.01 )
      printf("Mean is not zero:  The image is probably too small to support the detector\n");
  }

  { Stack stk, *t;
    stk.kind = 4;
    stk.width  = bank->shape[0];
    stk.height = bank->shape[1];
    stk.depth  = bank->strides_px[0] / bank->strides_px[3];
    stk.array  = (uint8*) bank->data;

    Scale_Stack_To_Range( &stk, 0, 0, 255 );
    t = Translate_Stack( &stk, GREY8 , 0 );
    Write_Stack( "bank.tif", t );

    Free_Stack(t);
  }
  Free_Array( bank );
  return 0;
}
#endif //EVAL_TEST_3                  

#ifdef EVAL_TEST_4
int main(int argc, char* argv[])
{ Range params[3] = { {0.0, 1.0,  0.1},
                      {0.5, 4.5,  0.5},
                      {-M_PI/4.0, M_PI/4.0, M_PI/72.0} };
  Array *bank = Build_Half_Space_Detectors( 
                    params[0], 
                    params[1], 
                    params[2], 
                    7, 
                    2*7+3);
  { float *p,*d,sum=0.0; 
    d = (float*) bank->data;
    p = d + bank->strides_px[0];
    while( p-- > d )
      sum += *p;
    printf("sum: %g\n",sum);
    if( sum > 0.01 )
      printf("Mean is not zero:  The image is probably too small to support the detector\n");
  }

  { Stack stk, *t;
    stk.kind = 4;
    stk.width  = bank->shape[0];
    stk.height = bank->shape[1];
    stk.depth  = bank->strides_px[0] / bank->strides_px[3];
    stk.array  = (uint8*) bank->data;

    Scale_Stack_To_Range( &stk, 0, 0, 255 );
    t = Translate_Stack( &stk, GREY8 , 0 );
    Write_Stack( "evaltest4.tif", t );

    Free_Stack(t);
  }
  Free_Array( bank );
  return 0;
}
#endif //EVAL_TEST_4

#ifdef EVAL_TEST_5
#define NLABELS 2
int main(int argc, char* argv[])
{ Range params[3] = { {0.0, 1.0,  0.1},
                      {0.5, 4.5,  0.5},
                      {-M_PI/4.0, M_PI/4.0, M_PI/72.0} };
  Array *bank = Build_Harmonic_Line_Detectors( 
                    params[0], 
                    params[1], 
                    params[2], 
                    7, 
                    2*7+3);
  { float *p,*d,sum=0.0; 
    d = (float*) bank->data;
    p = d + bank->strides_px[0];
    while( p-- > d )
      sum += ( *p - round(*p) )*10.0;;
    printf("sum: %g\n",sum);
    if( sum > 0.01 )
      printf("Mean is not zero:  The image is probably too small to support the detector\n");
  }

  { Stack stk, *lbl, *t;
    float *d;
    int i;
    d = (float*) bank->data;
    stk.kind = 4;
    stk.width  = bank->shape[0];
    stk.height = bank->shape[1];
    stk.depth  = bank->strides_px[0] / bank->strides_px[3];
    stk.array  = (uint8*) bank->data;

    lbl = Make_Stack(stk.kind, stk.width, stk.height, stk.depth);
    { int j,labels[NLABELS] = {2,3/*,5,7*/};
      d = (float*) bank->data;
      i =  bank->strides_px[0];
      while( i-- )
      { float v = d[i];
        int   l = lround(v);
        int cnt = 0;
        ( (float*) stk.array )[i] = (v-l)*10.0;
        ( (float*) lbl->array )[i] = 0;
        for(j=0;j<NLABELS;j++)
          if( l > 0.0 )
            if( ((int)l) % labels[j] == 0 )
            { ( (float*) lbl->array )[i] += (j+1);
                cnt++;
            }
          if(cnt)
            ( (float*) lbl->array )[i] /= cnt;
      }
    }


    Scale_Stack_To_Range( &stk, 0, 0, 255 );
    Scale_Stack_To_Range( lbl, 0, 0, 255 );
    t = Translate_Stack( &stk, GREY8 , 0 );
    Free_Stack(t);
    Write_Stack( "evaltest5_weights.tif", t );
    t = Translate_Stack( lbl, GREY8 , 0 );
    Write_Stack( "evaltest5_labels.tif", t );
    Free_Stack(t);
    Free_Stack(lbl);
  }
  Free_Array( bank );
  return 0;
}
#endif //EVAL_TEST_5
