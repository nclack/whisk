#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "utilities.h"
#include "draw_lib.h"

typedef struct
  { int red;
    int green;
    int blue;
  } Brush;

Brush *new_brush(Paint_Brush *floater, Image *canvas)
{ static Brush My_Brush;

  if (canvas->kind == GREY16)
    { My_Brush.red   = 0xFFFF * floater->red;
      My_Brush.green = 0xFFFF * floater->green;
      My_Brush.blue  = 0xFFFF * floater->blue;
    }
  else if (canvas->kind != FLOAT32)
    { My_Brush.red   = 0xFF * floater->red;
      My_Brush.green = 0xFF * floater->green;
      My_Brush.blue  = 0xFF * floater->blue;
    }
  return (&My_Brush);
}

#define PAINT_PIXEL(canvas,x)				\
switch (canvas->kind)					\
{ case GREY:						\
    bytes[x] = color->red;				\
    break;						\
  case GREY16:						\
    ((uint16 *) bytes)[x] = color->red;			\
    break;						\
  case COLOR:						\
    { int y = 3*(x);					\
      if (color->red >= 0)				\
        bytes[y]   = color->red;			\
      if (color->green >= 0)				\
        bytes[y+1] = color->green;			\
      if (color->blue >= 0)				\
        bytes[y+2] = color->blue;			\
    }							\
    break;						\
  case FLOAT32:						\
    ((float32 *) bytes)[x] = color->red;		\
}

/****************************************************************************************
 *                                                                                      *
 *  DRAWING ROUTINES FOR CONTOURS                                                       *
 *                                                                                      *
 ****************************************************************************************/

/* Color the pixels of a trace */

void Draw_Contour_Outline(Contour *cont, Paint_Brush *brush, Image *canvas)
{ int i, p, len;

  uint8   *bytes  = canvas->array;
  uint16  *shorts = (uint16 *) bytes;
  float32 *reals  = (float32 *) bytes;

  Brush   *color = new_brush(brush,canvas);

  if (canvas->kind != COLOR && color->red < 0) return;

  len = cont->length;
  switch (canvas->kind)
  { case GREY:
      for (i = 1; i < len; i++)
        bytes[cont->tour[i]] = color->red;
      break;
    case GREY16:
      for (i = 1; i < len; i++)
        shorts[cont->tour[i]] = color->red;
      break;
    case COLOR:
      for (i = 1; i < len; i++)
        { p = 3 * cont->tour[i];
          if (color->red >= 0)
            bytes[p]   = color->red;
          if (color->green >= 0)
            bytes[p+1] = color->green;
          if (color->blue >= 0)
            bytes[p+2] = color->blue;
        }
      break;
    case FLOAT32:
      for (i = 1; i < len; i++)
        reals[cont->tour[i]] = color->red;
      break;
  }
}

/* Color the region defined by trace "contour" */

void Draw_Contour_Interior(Contour *cont, Paint_Brush *brush, Image *canvas)
{ int *raster, ren;

  uint8   *bytes  = canvas->array;
  uint16  *shorts = (uint16 *) bytes;
  float32 *reals  = (float32 *) bytes;

  Brush    *color = new_brush(brush,canvas);

  if (canvas->kind != COLOR && color->red < 0) return;

  raster = Raster_Scan(cont,&ren);

  { int i, v, w, p;

    switch (canvas->kind)
    { case GREY:
        for (i = 0; i < ren; i += 2)
          { v = raster[i];
            w = raster[i+1];
            for (p = v; p < w; p++)
              bytes[p] = color->red;
          }
        break;
      case GREY16:
        for (i = 0; i < ren; i += 2)
          { v = raster[i];
            w = raster[i+1];
            for (p = v; p < w; p++)
              shorts[p] = color->red;
          }
        break;
      case COLOR:
        for (i = 0; i < ren; i += 2)
          { v = 3*raster[i];
            w = 3*raster[i+1];
            for (p = v; p < w; p += 3)
              { if (color->red >= 0)
                  bytes[p]   = color->red;
                if (color->green >= 0)
                  bytes[p+1] = color->green;
                if (color->blue >= 0)
                  bytes[p+2] = color->blue;
              }
          }
        break;
      case FLOAT32:
        for (i = 0; i < ren; i += 2)
          { v = raster[i];
            w = raster[i+1];
            for (p = v; p < w; p++)
              shorts[p] = color->red;
          }
        break;
    }
  }
}

/* Color the complement of the region defined by trace "contour" */

void Draw_Contour_Exterior(Contour *cont, Paint_Brush *brush, Image *canvas)
{ int *raster, ren;

  uint8   *bytes  = canvas->array;
  uint16  *shorts = (uint16 *) bytes;
  float32 *reals  = (float32 *) bytes;

  Brush   *color = new_brush(brush,canvas);

  if (canvas->kind != COLOR && color->red < 0) return;

  raster = Raster_Scan(cont,&ren);

  { int i, v, w, p;

    for (i = -1; i < ren; i += 2)
      { if (i < 0)
          v = 0;
        else
          v = raster[i];
        if (i+1 >= ren)
          w = canvas->width*canvas->height;
        else 
          w = raster[i+1];
        switch (canvas->kind)
        { case GREY:
            for (p = v; p < w; p++)
              bytes[p] = color->red;
            break;
          case GREY16:
            for (p = v; p < w; p++)
              shorts[p] = color->red;
            break;
          case COLOR:
            v *= 3;
            w *= 3;
            for (p = v; p < w; p += 3)
              { if (color->red >= 0)
                  bytes[p]   = color->red;
                if (color->green >= 0)
                  bytes[p+1] = color->green;
                if (color->blue >= 0)
                  bytes[p+2] = color->blue;
              }
            break;
          case FLOAT32:
            for (p = v; p < w; p++)
              reals[p] = color->red;
        }
      }
  }
}

/****************************************************************************************
 *                                                                                      *
 *  DRAWING ROUTINES FOR LEVEL SETS                                                     *
 *                                                                                      *
 ****************************************************************************************/

void Draw_Level_Set_Outline(Level_Set *r, Paint_Brush *brush, Image *canvas)
{ Contour *c;
  c = Trace_Level_Set(r);
  Draw_Contour_Outline(c,brush,canvas);
  Free_Contour(c);
}

void Draw_Level_Set_Interior(Level_Set *r, Paint_Brush *brush, Image *canvas)
{ Contour *c;
  c = Trace_Level_Set(r);
  Draw_Contour_Interior(c,brush,canvas);
  Free_Contour(c);
}

void Draw_Level_Set_Exterior(Level_Set *r, Paint_Brush *brush, Image *canvas)
{ Contour *c;
  c = Trace_Level_Set(r);
  Draw_Contour_Exterior(c,brush,canvas);
  Free_Contour(c);
}

/****************************************************************************************
 *                                                                                      *
 *  DRAWING ROUTINES FOR WATERHSEDS                                                     *
 *                                                                                      *
 ****************************************************************************************/

void Draw_Watershed_Outline(Watershed_2D *w, int cb, Paint_Brush *brush, Image *canvas)
{ Contour *c;
  c = Trace_Watershed(w,cb);
  Draw_Contour_Outline(c,brush,canvas);
  Free_Contour(c);
}

void Draw_Watershed_Interior(Watershed_2D *w, int cb, Paint_Brush *brush, Image *canvas)
{ Contour *c;
  c = Trace_Watershed(w,cb);
  Draw_Contour_Interior(c,brush,canvas);
  Free_Contour(c);
}

void Draw_Watershed_Exterior(Watershed_2D *w, int cb, Paint_Brush *brush, Image *canvas)
{ Contour *c;
  c = Trace_Watershed(w,cb);
  Draw_Contour_Exterior(c,brush,canvas);
  Free_Contour(c);
}

/****************************************************************************************
 *                                                                                      *
 *  DRAWING ROUTINES FOR BASIC SHAPES                                                   *
 *                                                                                      *
 ****************************************************************************************/

/* Draw a w x h rectangle with lower left corner (x,y)  */

void Draw_Rectangle(int x, int y, int w, int h, Paint_Brush *brush, Image *canvas)
{ int i, q, p, t;
  int wide, high, cap;

  uint8   *bytes  = canvas->array;
  uint16  *shorts = (uint16 *) bytes;
  float32 *reals  = (float32 *) bytes;

  Brush   *color = new_brush(brush,canvas);

  if (canvas->kind != COLOR && color->red < 0) return;

  wide = canvas->width;
  high = canvas->height;

  if (x < 0)
    { w += x;
      x  = 0;
    }
  if (x + w > wide)
    w = wide-x;
  if (y < 0)
    { h += y;
      y  = 0;
    }
  if (y + h > high)
    h = high - y;

  cap = (y+h)*wide;
  switch (canvas->kind)
  { case GREY:
      for (p = y * wide + x, t = p+w; p < cap; p += wide, t += wide)
        for (q = p; q < t; q++)
          bytes[q] = color->red;
      break;
    case GREY16:
      for (p = y * wide + x, t = p+w; p < cap; p += wide, t += wide)
        for (q = p; q < t; q++)
          shorts[q] = color->red;
      break;
    case COLOR:
      wide *= 3;
      cap  *= 3;
      for (p = y * wide + 3 * x, t = p+3*w; p < cap; p += wide, t += wide)
        for (q = p; q < t; q += 3)
          { if (color->red >= 0)
              bytes[q]   = color->red;
            if (color->green >= 0)
              bytes[q+1] = color->green;
            if (color->blue >= 0)
              bytes[q+2] = color->blue;
          }
      break;
    case FLOAT32:
      for (p = y * wide + x, t = p+w; p < cap; p += wide, t += wide)
        for (q = p; q < t; q++)
          reals[q] = color->red;
      break;
  }
}

/* Reset an entire image */

void Draw_Image(Paint_Brush *brush, Image *canvas)
{ Draw_Rectangle(0,0,canvas->width,canvas->height,brush,canvas); }

/* Draw a point centered a pixel (x,w) */

void Draw_Point(int x, int y, Paint_Brush *brush, Image *canvas)
{ int p;

  uint8 *bytes = canvas->array;
  Brush *color = new_brush(brush,canvas);

  if (canvas->kind != COLOR && color->red < 0) return;

  p = y * canvas->width + x;
  PAINT_PIXEL(canvas,p);
}

/* Draw a cross centered a pixel (x,y) with each arm being a pixels long */

void Draw_Cross(int x, int y, int a, Paint_Brush *brush, Image *canvas)
{ int i, q, p, cap, wide;

  uint8 *bytes = canvas->array;
  Brush *color = new_brush(brush,canvas);

  if (canvas->kind != COLOR && color->red < 0) return;

  wide  = canvas->width;
  cap   = wide*canvas->height;

  p = y * wide + x;
  q = p - a * wide;
  for (i = -a; i <= a; i++)
    { if (q < 0 || q >= cap) continue;
      PAINT_PIXEL(canvas,q);
      q += wide;
    }
  x = x - a;
  q = p - a;
  for (i = -a; i <= a; i++)
    { if (x < 0 || x >= wide) continue;
      PAINT_PIXEL(canvas,q);
      q += 1;
      x += 1;
    }
}

/* Draw a circle centered at pixel (x,y) with radius r */

void Draw_Circle(int x, int y, int r, Paint_Brush *brush, Image *canvas)
{ int i, a, q, p, s, cap, wide;

  uint8   *bytes  = canvas->array;
  Brush   *color = new_brush(brush,canvas);

  if (canvas->kind != COLOR && color->red < 0) return;

  wide  = canvas->width;
  cap   = wide*canvas->height;
  p     = y * wide + x;
  a     = r;
  for (i = 0; i <= r; i++)
    { while (a*a > r*r - i*i)
        a -= 1;

      q = p + i*wide;
      if (q < cap)
        { s = q+a;
          if (x-a < 0)
            q += a-x;
          if (x+a >= wide)
            s -= (x+a+1)-wide;
          for (q -= a; q <= s; q++)
            PAINT_PIXEL(canvas,q);
        }

      if (i == 0) continue;

      q = p - i*wide;
      if (q >= 0)
        { s = q+a;
          if (x-a < 0)
            q += a-x;
          if (x+a >= wide)
            s -= (x+a+1)-wide;
          for (q -= a; q <= s; q++)
            PAINT_PIXEL(canvas,q);
        }
    }
}

/* Midpoint scan-conversion routine drawing a line from pixel (x0,y0) to (x1,y1).  */

void Draw_Line(int x0, int y0, int x1, int y1, Paint_Brush *brush, Image *canvas)
{ int     pix;
  int     i, d, wid;
  int     dsame, dchange;
  int     dx, dy;
  int     xinc, yinc;

  uint8  *bytes = canvas->array; 
  Brush  *color = new_brush(brush,canvas);

  if (canvas->kind != COLOR && color->red < 0) return;

  /* FIX: by ngc
  ** according to tests, line's aren't drawn correctly when x0 > x1
  **  so:
  */
  if( x0 > x1 )
  { int t;
    t  = x0; 
    x0 = x1; 
    x1 = t;
    
    t  = y0;
    y0 = y1;
    y1 = t;
  }
  /* end FIX */
  

  dx = x1 - x0;
  dy = y1 - y0;
  if (dx < 0) xinc = -1; else xinc = 1;
  if (dy < 0) yinc = -1; else yinc = 1;

  wid  = canvas->width;
  pix  = wid*y0 + x0;
  wid *= yinc;

/* Case where |m| < 1 */

  if (dx*dx >= dy*dy)
    { dsame = 2*dy;
      dchange = dsame - 2*xinc*yinc*dx;
      d =  dsame*xinc - dx*yinc;
      if (dx < 0) d = -d;

#define SCAN_BODY(ifsame,increment)		\
    PAINT_PIXEL(canvas,pix);			\
    if (ifsame)					\
      d += dsame;				\
    else					\
      { d += dchange;				\
        increment;				\
      }

      if (dy >= 0)
        { for (i = x0; i != x1; i += xinc)
            { SCAN_BODY(d < 0,pix += wid)
              pix += 1;
            }
          PAINT_PIXEL(canvas,pix);
        }
      else
        { for (i = x0; i != x1; i += xinc)
            { SCAN_BODY(d >= 0,pix += wid)
              pix += 1;
            }
          PAINT_PIXEL(canvas,pix);
        }
    }

  else
    { dsame = -2*dx;
      dchange = dsame + 2*xinc*yinc*dy;
      d = dsame*yinc + dy*xinc;
      if (dy < 0) d = -d;

      if (dx >= 0)
        { for (i = y0; i != y1; i += yinc)
            { SCAN_BODY(d > 0,pix += 1)
              pix += wid;
            }
          PAINT_PIXEL(canvas,pix);
        }
      else
        { for (i = y0; i != y1; i += yinc)
            { SCAN_BODY(d <= 0,pix += 1)
              pix += wid;
            }
          PAINT_PIXEL(canvas,pix);
        }
    }
}
