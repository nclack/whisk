/*****************************************************************************************\
*                                                                                         *
*  Drawing Level Sets, Contours, and Basic Shapes                                         *
*                                                                                         *
*  Author:  Gene Myers                                                                    *
*  Date  :  June 2007                                                                     *
*                                                                                         *
\*****************************************************************************************/

#ifndef _DRAW_LIB

#define _DRAW_LIB

#include <stdio.h>
#include "image_lib.h"
#include "contour_lib.h"
#include "level_set.h"
#include "water_shed.h"

/* Drawing routines for level sets and contours.  */

typedef struct              //  Specification of how to paint a pixel
  { float   red;            //  If a channel is < 0. then that channel is not painted
    float   green;          //  For GREY(16) and FLOAT32 images, only the red channel is drawn
    float   blue;
  } Paint_Brush;

void Draw_Contour_Outline(Contour *trace, Paint_Brush *color, Image *canvas);
void Draw_Contour_Interior(Contour *trace, Paint_Brush *color, Image *canvas);
void Draw_Contour_Exterior(Contour *trace, Paint_Brush *color, Image *canvas);

void Draw_Level_Set_Outline(Level_Set *r, Paint_Brush *color, Image *canvas);
void Draw_Level_Set_Interior(Level_Set *r, Paint_Brush *color, Image *canvas);
void Draw_Level_Set_Exterior(Level_Set *r, Paint_Brush *color, Image *canvas);

void Draw_Watershed_Outline(Watershed_2D *w, int cb, Paint_Brush *color, Image *canvas);
void Draw_Watershed_Interior(Watershed_2D *w, int cb, Paint_Brush *color, Image *canvas);
void Draw_Watershed_Exterior(Watershed_2D *w, int cb, Paint_Brush *color, Image *canvas);

void Draw_Point(int x, int y, Paint_Brush *color, Image *canvas);
void Draw_Cross(int x, int y, int length, Paint_Brush *color, Image *canvas);
void Draw_Rectangle(int x, int y, int w, int h, Paint_Brush *color, Image *canvas);
void Draw_Circle(int x, int y, int radius, Paint_Brush *color, Image *canvas);
void Draw_Line(int x0, int y0, int x1, int y1, Paint_Brush *color, Image *canvas);

void Draw_Image(Paint_Brush *color, Image *canvas);

#endif
