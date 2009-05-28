#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "utilities.h"
#include "image_lib.h"
#include "water_shed.h"

static int black[3]   = {   0,   0,   0 };
static int blue[3]    = {   0,   0, 255 };
static int yellow[3]  = { 255, 255,   0 };
static int cyan[3]    = {   0, 255, 255 };
static int magenta[3] = { 255,   0, 255 };
static int orange[3]  = { 255, 127,   0 };
static int brown[3]   = { 150, 102,  50 };
static int red[3]     = { 255,   0,   0 };
static int green[3]   = {   0, 255,   0 };
static int *palette[8] = { yellow, red, green, blue, cyan, magenta, orange, brown };

/****************************************************************************************
 *                                                                                      *
 *  WATERSHED SPACE MANAGEMENT ROUTINES                                                 *
 *                                                                                      *
 ****************************************************************************************/

//  Workspace routines for building a component tree

static int *get_chord(int area, char *routine)
{ static int  Max_Area = 0;
  static int *Array = NULL;

  if (area < 0)
    { free(Array);
      Max_Area = 0;
      Array    = NULL;
    }
  else if (area > Max_Area)
    { Max_Area = area;
      Array    = (int *) Guarded_Realloc(Array,sizeof(int)*Max_Area,routine);
    }
  return (Array);
}

static inline int watershed_2d_ssize(Watershed_2D *part)
{ return (sizeof(int)*(part->nbasins+1)); }

MANAGER -r Watershed_2D seeds:ssize labels^Image

void Reset_Watershed_2D()
{ reset_watershed_2d();
  get_chord(-1,NULL);
}

/****************************************************************************************
 *                                                                                      *
 *  BUILD A 2D WATERSHED DECOMPOSITION                                                  *
 *                                                                                      *
 ****************************************************************************************/

static int cwidth;
static int cheight;
static int carea;

static int chk_width;
static int chk_height;
static int chk_iscon4;

static inline int *boundary_pixels_2d(int p)
{ static int bound[8];
  int x, xn, xp;
  int y, yn, yp;

  y = p/cwidth;
  x = p%cwidth;    
  
  yn = (y > 0);
  xn = (x > 0);
  xp = (x < chk_width);
  yp = (y < chk_height);
  if (chk_iscon4)
    { bound[0] = yn;
      bound[1] = xn;
      bound[2] = xp;
      bound[3] = yp;
    }
  else
    { bound[0] = yn && xn;
      bound[1] = yn;
      bound[2] = yn && xp;
      bound[3] = xn;
      bound[4] = xp;
      bound[5] = yp && xn;
      bound[6] = yp;
      bound[7] = yp && xp;
    }

  return (bound);                                                                       
}

#define WSHED    0
#define ONQUEUE -1
#define MASK    -2
#define INIT    -3
#define MINIM   -4

Watershed_2D *Build_2D_Watershed(Image *frame, int iscon4)
{ Watershed_2D *result;

  int    nlabels;   // The number of labels (1..nlabels)
  int    nbasins;   // Number of watersheds
  int   *seeds;     // A pixel in each watershed from which one can flood fill or contour

  int index[0x10001];
  int maxval;

  int neighbor[8];
  int n_nbrs;

  uint8  *value8;
  uint16 *value16;

  int   *chord;
  int   *out;

  if (frame->kind == COLOR)
    { fprintf(stderr,"Build_2D_Watershed: Cannot apply to COLOR images\n");
      exit (1);
    }
  if (frame->kind == FLOAT32)
    { fprintf(stderr,"Build_2D_Watershed: Cannot apply to FLOAT32 images\n");
      exit (1);
    }

  cwidth   = frame->width;
  cheight  = frame->height;
  carea    = cwidth*cheight;

  if (iscon4)
    { neighbor[0] = -cwidth;
      neighbor[1] = -1;
      neighbor[2] =  1;
      neighbor[3] =  cwidth;
      n_nbrs = 4;
    }
  else
    { int x, y;

      n_nbrs = 0;
      for (y = -cwidth; y <= cwidth; y += cwidth)
        for (x = -1; x <= 1; x += 1)
          if (! (x==0 && y == 0))
            neighbor[n_nbrs++] = y+x;
    }

  chk_width  = cwidth-1;
  chk_height = cheight-1;
  chk_iscon4 = iscon4;

  chord = get_chord(2*carea,"Build_2D_Watershed");
  out   = chord + carea;

  if (frame->kind == GREY16)
    { maxval  = 0x10000;
      value8  = NULL;
      value16 = (uint16 *) (frame->array);
    }
  else
    { maxval  = 0x100;
      value8  = frame->array;
      value16 = NULL;
    }

  // Establish lists of pixels with each value

  { int p, v;

    for (v = 0; v <= maxval; v++)
      index[v] = -1;

    if (value8 != NULL)
      for (p = 0; p < carea; p++)
        { v        = value8[p];
          chord[p] = index[v];
          index[v] = p;
        }
    else  // value16 != NULL
      for (p = 0; p < carea; p++)
        { v        = value16[p];
          chord[p] = index[v];
          index[v] = p;
        }
  }

#define PUSH(p)			\
{ if (qbot < 0)			\
    qtop = qbot = (p);		\
  else				\
    qtop = chord[qtop] = (p);	\
  chord[qtop] = -1;		\
}

#define POP(p)  { qbot = chord[p = qbot]; }

  //  Label all pixels that are not part of a catchment basin minimum with INIT
  //    and those that are with MINIM.

  { int v, p;

    for (p = 0; p < carea; p++)
      out[p] = MINIM;

    if (value8 != NULL)

      for (v = 0; v < maxval; v++)
        { int qtop, qbot, t;
  
          //  Push all c-pixels that are adjacent to a pixel of lesser value and mark INIT

          qbot = -1;
          for (p = index[v]; p >= 0; p = t)
            { int *b, j;
  
              t = chord[p];
              b = boundary_pixels_2d(p);
              for (j = 0; j < n_nbrs; j++)
                if (b[j] && value8[p+neighbor[j]] < v)
                  { out[p] = INIT;
                    PUSH(p)
                    break;
                  }
            }

          //  Find all c-pixels connected to another labeled INIT and mark as INIT
  
          while (qbot >= 0)
            { int p, *b, j, q;
              int cdist;
  
              POP(p);
              b = boundary_pixels_2d(p);
              for (j = 0; j < n_nbrs; j++)
                if (b[j])
                  { q = p + neighbor[j];
                    if (out[q] != INIT && value8[q] == v)
                      { out[q] = INIT;
                        PUSH(q);
                      }
                  }
            }
        }
  }

  // Build pixel value lists again but now with only INIT pixels in each list

  { int p, v;

    for (v = 0; v <= maxval; v++)
      index[v] = -1;

    if (value8 != NULL)
      { for (p = 0; p < carea; p++)
          if (out[p] == INIT)
            { v        = value8[p];
              chord[p] = index[v];
              index[v] = p;
            }
       }

     else // value16 != NULL
      { for (p = 0; p < carea; p++)
          if (out[p] == INIT)
            { v        = value16[p];
              chord[p] = index[v];
              index[v] = p;
            }
      }
  }

  // Find connected components of MINIM pixels and label with a unique catchment basin #.

  { int v, p;
    int qtop, qbot;

    nbasins = 0;

    qbot = -1;
    for (p = 0; p < carea; p++)
      if (out[p] == MINIM)
        { PUSH(p);
          out[p] = ++nbasins;
          while (qbot >= 0)
            { int q, *b, j, r;

              POP(q);
              b = boundary_pixels_2d(q);
              for (j = 0; j < n_nbrs; j++)
                if (b[j])
                  { r = q+neighbor[j];
                    if (out[r] == MINIM)
                      { PUSH(r);
                        out[r] = nbasins;
                      }
                  }
            }
        }
  }

  { int v, p;

    for (v = 0; v < maxval; v++)
      { int qbot, qtop, t;

        // Push all INIT c-pixels that are adjacent to a previously labeled pixel and
        //   label ONQUEUE, label all others MASK

        qbot = -1;
        for (p = index[v]; p >= 0; p = t)
          { int *b, j;

            t      = chord[p];
            out[p] = MASK;
            b      = boundary_pixels_2d(p);
            for (j = 0; j < n_nbrs; j++)
              if (b[j] && out[p+neighbor[j]] >= 0)
                { out[p] = ONQUEUE;
                  PUSH(p);
                  break;
                }
          }

        //  In BFS, reach all c-pixels (have MASK set) in order of distance, and label
        //    with catchment basin or WSHED

        while (qbot >= 0)
          { int p, *b, j, q, o;

            POP(p);
            b = boundary_pixels_2d(p);

            for (j = 0; j < n_nbrs; j++)
              if (b[j])
                { q = p + neighbor[j];
                  o = out[q];
                  if (o == MASK)
                    { out[q] = ONQUEUE;
                      PUSH(q);
                    }
                  else if (o != ONQUEUE && o != INIT)
                    { if (out[p] == ONQUEUE)
                        out[p] = o;
                      else if (o != out[p] && o != WSHED)
                        if (out[p] == WSHED)
                          out[p] = o;
                        else
                          out[p] = WSHED;
                    }
                }
          }
      }
  }

  //  Find all WSHED pixels in order of distance from a non-WSHED pixel and randomly
  //    assign to one of the nearest catchment basins

  { int p, qbot, qtop;

    qbot = -1;
    for (p = 0; p < carea; p++)
      if (out[p] == WSHED)
        { int *b, j;

          b = boundary_pixels_2d(p);
          for (j = 0; j < n_nbrs; j++)
            if (b[j] && out[p+neighbor[j]] > 0)
              { PUSH(p);
                out[p] = ONQUEUE;
                break;
              }
        }

    while (qbot >= 0)
      { int *b, j, q, o;

        POP(p);
        b = boundary_pixels_2d(p);

        for (j = 0; j < n_nbrs; j++)
          if (b[j])
            { q = p + neighbor[j];
              o = out[q];
              if (o > 0)
                out[p] = o;
              else if (out[q] != ONQUEUE)
                { PUSH(q);
                  out[q] = ONQUEUE;
                }
            }
      }
  }

  //  Make the data structure now

  result = new_watershed_2d(sizeof(int)*(nbasins+1),"Build_2D_Watershed");
  result->nbasins = nbasins;
  result->iscon4  = iscon4;

  //  Greedily assign labels to catchment basins (CBs) so that no basin is adjacent to
  //    another with the same number.  Typicall the number of labels required is very,
  //    very small, 6 or 7 in my experience.  At the end of this segment, out[p] is
  //    the negative of its label reassignment, seeds[b] is the new label for CB b,
  //    and nlabels is the number of unique labels used.

  { int   qbot, qtop;
    int   olabel;
    int   adjacent[256];
    int   alargest;
    int   p;

    seeds   = result->seeds;
    nlabels = 0;

    for (p = 1; p < 256; p++)
      adjacent[p] = 0;

    qbot = -1;
    for (p = 0; p < carea; p++)
      if (out[p] > 0)

        { PUSH(p);
          olabel   = out[p];
          out[p]   = 0;
          alargest = 0;

          while (qbot >= 0)
            { int *b, j, q, r, o;

              POP(q);
              out[q] = -olabel;
      
              b = boundary_pixels_2d(q);
              for (j = 0; j < n_nbrs; j++)
                if (b[j])
                  { r = q + neighbor[j];
                    o = out[r];
                    if (o == olabel)
                      { PUSH(r);
                        out[r] = 0;
                      }
                    else
                      { o = -o;
                        if (o > 0 && o != olabel)
                          { o = seeds[o];
                            adjacent[o] = 1;
                            if (o > alargest)
                              alargest = o;
                          }
                      }
                  }
            }

          { int j;

            for (j = 1; j <= alargest; j++)
              if (! adjacent[j])
                break;

            seeds[olabel] = j;
            if (j > nlabels)
              { nlabels = j;
                if (j > 255)
                  { fprintf(stderr,"Warning: More than 255 colors needed to label watershed\n");
                    fprintf(stderr,"         Returning NULL result\n");
                    fflush(stderr);
                    return (NULL);
                  }
              }

            for (j = 1; j <= alargest; j++)
              adjacent[j] = 0;
          }
        }
  }

  //  Make an image with the assigned labels, and make seeds contain the lexicographically
  //    smalles pixel in each region.

  { int    p, b;
    uint8 *iarray;

    result->nlabels = nlabels;
    result->labels  = Make_Image(GREY,cwidth,cheight);
    iarray = result->labels->array;

    for (p = 0; p < carea; p++)
      { out[p] = b = -out[p];
        iarray[p] = seeds[b];
      }
    for (p = 0; p < carea; p++)
      if (seeds[b = out[p]]  > 0) 
        seeds[b] = -(p+1); 
    for (b = 1; b <= nbasins; b++)
      seeds[b] = -seeds[b];
  }

  { int    p;
    int   *color;
    Image *view;

    view = Make_Image(COLOR,frame->width,frame->height);

    for (p = 0; p < carea; p++)
      { if (out[p] <= 0)
          { printf("Unassigned pix\n");
            continue;
          }
        color = palette[(out[p]-1)%8];
        view->array[3*p]   = color[0];
        view->array[3*p+1] = color[1];
        view->array[3*p+2] = color[2];
      }

    Write_Image("watershed.tif",view);

    Kill_Image(view);
  }

  return (result);
}


/****************************************************************************************
 *                                                                                      *
 *  BUILD A 3D WATERSHED DECOMPOSITION  (UNTESTED !!!)                                  *
 *                                                                                      *
 ****************************************************************************************/


static inline int watershed_3d_ssize(Watershed_3D *part)
{ return (sizeof(int)*(part->nbasins+1)); }

MANAGER -r Watershed_3D seeds:ssize labels^Stack

void Reset_Watershed_3D()
{ reset_watershed_3d();
  get_chord(-1,NULL);
}

static int cdepth;     //  Other globals, e.g. cwidth, etc. already introduced for 2D code
static int cvolume;
static int chk_depth;
static int chk_iscon6;

static inline int *boundary_pixels_3d(int p)
{ static int bound[26];
  int x, xn, xp;
  int y, yn, yp;
  int z, zn, zp;

  z = p/carea;
  y = p%carea;
  x = y%cwidth;
  y = y/cwidth;

  yn = (y > 0);
  xn = (x > 0);
  zn = (z > 0);
  xp = (x < chk_width);
  yp = (y < chk_height);
  zp = (z < chk_depth);
  if (chk_iscon6)
    { bound[0] = zn;
      bound[1] = yn;
      bound[2] = xn;
      bound[3] = xp;
      bound[4] = yp;
      bound[5] = zp;
    }
  else
    { int t;

      bound[ 1] = t = zn && yn;
      bound[ 0] = t && xn;
      bound[ 2] = t && xp;
      bound[ 3] = zn && xn;
      bound[ 4] = zn;
      bound[ 5] = zn && xp;
      bound[ 7] = t = zn && yp;
      bound[ 6] = t && xn;
      bound[ 8] = t && xp;

      bound[ 9] = yn && xn;
      bound[10] = yn;
      bound[11] = yn && xp;
      bound[12] = xn;
      bound[13] = xp;
      bound[14] = yp && xn;
      bound[15] = yp;
      bound[16] = yp && xp;

      bound[18] = t = zp && yn;
      bound[17] = t && xn;
      bound[19] = t && xp;
      bound[20] = zp && xn;
      bound[21] = zp;
      bound[22] = zp && xp;
      bound[24] = t = zp && yp;
      bound[23] = t && xn;
      bound[25] = t && xp;
    }

  return (bound);
}

Watershed_3D *Build_3D_Watershed(Stack *frame, int iscon6)
{ Watershed_3D *result;

  int    nlabels;   // The number of labels (1..nlabels)
  int    nbasins;   // Number of watersheds
  int   *seeds;     // A pixel in each watershed from which one can flood fill or contour

  int index[0x10001];
  int maxval;

  int neighbor[26];
  int n_nbrs;

  uint8  *value8;
  uint16 *value16;

  int   *chord;
  int   *out;

  if (frame->kind == COLOR)
    { fprintf(stderr,"Build_3D_Watershed: Cannot apply to COLOR images\n");
      exit (1);
    }
  if (frame->kind == FLOAT32)
    { fprintf(stderr,"Build_3D_Watershed: Cannot apply to FLOAT32 images\n");
      exit (1);
    }

  cwidth   = frame->width;
  cheight  = frame->height;
  cdepth   = frame->depth;
  carea    = cwidth*cheight;
  cvolume  = carea*cdepth;

  if (iscon6)
    { neighbor[0] = -carea;
      neighbor[1] = -cwidth;
      neighbor[2] = -1;
      neighbor[3] =  1;
      neighbor[4] =  cwidth;
      neighbor[5] =  carea;
      n_nbrs = 6;
    }
  else
    { int x, y, z;

      n_nbrs = 0;
      for (z = -carea; z <= carea; z += carea)
        for (y = -cwidth; y <= cwidth; y += cwidth)
          for (x = -1; x <= 1; x += 1)
            if (! (x==0 && y == 0 && z == 0))
              neighbor[n_nbrs++] = z+y+x;
    }

  chk_width  = cwidth-1;
  chk_height = cheight-1;
  chk_depth  = cdepth-1;
  chk_iscon6 = iscon6;

  chord = get_chord(2*cvolume,"Build_3D_Watershed");
  out   = chord + cvolume;

  if (frame->kind == GREY16)
    { maxval  = 0x10000;
      value8  = NULL;
      value16 = (uint16 *) (frame->array);
    }
  else
    { maxval  = 0x100;
      value8  = frame->array;
      value16 = NULL;
    }

  // Establish lists of pixels with each value

  { int p, v;

    for (v = 0; v <= maxval; v++)
      index[v] = -1;

    if (value8 != NULL)
      for (p = 0; p < cvolume; p++)
        { v        = value8[p];
          chord[p] = index[v];
          index[v] = p;
        }
    else  // value16 != NULL
      for (p = 0; p < cvolume; p++)
        { v        = value16[p];
          chord[p] = index[v];
          index[v] = p;
        }
  }

#define PUSH(p)			\
{ if (qbot < 0)			\
    qtop = qbot = (p);		\
  else				\
    qtop = chord[qtop] = (p);	\
  chord[qtop] = -1;		\
}

#define POP(p)  { qbot = chord[p = qbot]; }

  //  Label all pixels that are not part of a catchment basin minimum with INIT
  //    and those that are with MINIM.

  { int v, p;

    for (p = 0; p < cvolume; p++)
      out[p] = MINIM;

    if (value8 != NULL)

      for (v = 0; v < maxval; v++)
        { int qtop, qbot, t;
  
          //  Push all c-pixels that are adjacent to a pixel of lesser value and mark INIT

          qbot = -1;
          for (p = index[v]; p >= 0; p = t)
            { int *b, j;
  
              t = chord[p];
              b = boundary_pixels_3d(p);
              for (j = 0; j < n_nbrs; j++)
                if (b[j] && value8[p+neighbor[j]] < v)
                  { out[p] = INIT;
                    PUSH(p)
                    break;
                  }
            }

          //  Find all c-pixels connected to another labeled INIT and mark as INIT
  
          while (qbot >= 0)
            { int p, *b, j, q;
              int cdist;
  
              POP(p);
              b = boundary_pixels_3d(p);
              for (j = 0; j < n_nbrs; j++)
                if (b[j])
                  { q = p + neighbor[j];
                    if (out[q] != INIT && value8[q] == v)
                      { out[q] = INIT;
                        PUSH(q);
                      }
                  }
            }
        }
  }

  // Build pixel value lists again but now with only INIT pixels in each list

  { int p, v;

    for (v = 0; v <= maxval; v++)
      index[v] = -1;

    if (value8 != NULL)
      { for (p = 0; p < cvolume; p++)
          if (out[p] == INIT)
            { v        = value8[p];
              chord[p] = index[v];
              index[v] = p;
            }
       }

     else // value16 != NULL
      { for (p = 0; p < cvolume; p++)
          if (out[p] == INIT)
            { v        = value16[p];
              chord[p] = index[v];
              index[v] = p;
            }
      }
  }

  // Find connected components of MINIM pixels and label with a unique catchment basin #.

  { int v, p;
    int qtop, qbot;

    nbasins = 0;

    qbot = -1;
    for (p = 0; p < cvolume; p++)
      if (out[p] == MINIM)
        { PUSH(p);
          out[p] = ++nbasins;
          while (qbot >= 0)
            { int q, *b, j, r;

              POP(q);
              b = boundary_pixels_3d(q);
              for (j = 0; j < n_nbrs; j++)
                if (b[j])
                  { r = q+neighbor[j];
                    if (out[r] == MINIM)
                      { PUSH(r);
                        out[r] = nbasins;
                      }
                  }
            }
        }
  }

  { int v, p;

    for (v = 0; v < maxval; v++)
      { int qbot, qtop, t;

        // Push all INIT c-pixels that are adjacent to a previously labeled pixel and
        //   label ONQUEUE, label all others MASK

        qbot = -1;
        for (p = index[v]; p >= 0; p = t)
          { int *b, j;

            t      = chord[p];
            out[p] = MASK;
            b      = boundary_pixels_3d(p);
            for (j = 0; j < n_nbrs; j++)
              if (b[j] && out[p+neighbor[j]] >= 0)
                { out[p] = ONQUEUE;
                  PUSH(p);
                  break;
                }
          }

        //  In BFS, reach all c-pixels (have MASK set) in order of distance, and label
        //    with catchment basin or WSHED

        while (qbot >= 0)
          { int p, *b, j, q, o;

            POP(p);
            b = boundary_pixels_3d(p);

            for (j = 0; j < n_nbrs; j++)
              if (b[j])
                { q = p + neighbor[j];
                  o = out[q];
                  if (o == MASK)
                    { out[q] = ONQUEUE;
                      PUSH(q);
                    }
                  else if (o != ONQUEUE && o != INIT)
                    { if (out[p] == ONQUEUE)
                        out[p] = o;
                      else if (o != out[p] && o != WSHED)
                        if (out[p] == WSHED)
                          out[p] = o;
                        else
                          out[p] = WSHED;
                    }
                }
          }
      }
  }

  //  Find all WSHED pixels in order of distance from a non-WSHED pixel and randomly
  //    assign to one of the nearest catchment basins

  { int p, qbot, qtop;

    qbot = -1;
    for (p = 0; p < cvolume; p++)
      if (out[p] == WSHED)
        { int *b, j;

          b = boundary_pixels_3d(p);
          for (j = 0; j < n_nbrs; j++)
            if (b[j] && out[p+neighbor[j]] > 0)
              { PUSH(p);
                out[p] = ONQUEUE;
                break;
              }
        }

    while (qbot >= 0)
      { int *b, j, q, o;

        POP(p);
        b = boundary_pixels_3d(p);

        for (j = 0; j < n_nbrs; j++)
          if (b[j])
            { q = p + neighbor[j];
              o = out[q];
              if (o > 0)
                out[p] = o;
              else if (out[q] != ONQUEUE)
                { PUSH(q);
                  out[q] = ONQUEUE;
                }
            }
      }
  }

  //  Make the data structure now

  result = new_watershed_3d(sizeof(int)*(nbasins+1),"Build_2D_Watershed");
  result->nbasins = nbasins;
  result->iscon6  = iscon6;

  //  Greedily assign labels to catchment basins (CBs) so that no basin is adjacent to
  //    another with the same number.  Typicall the number of labels required is very,
  //    very small, 6 or 7 in my experience.  At the end of this segment, out[p] is
  //    the negative of its label reassignment, seeds[b] is the new label for CB b,
  //    and nlabels is the number of unique labels used.

  { int   qbot, qtop;
    int   olabel;
    int   adjacent[256];
    int   alargest;
    int   p;

    seeds   = result->seeds;
    nlabels = 0;

    for (p = 1; p < 256; p++)
      adjacent[p] = 0;

    qbot = -1;
    for (p = 0; p < cvolume; p++)
      if (out[p] > 0)

        { PUSH(p);
          olabel   = out[p];
          out[p]   = 0;
          alargest = 0;

          while (qbot >= 0)
            { int *b, j, q, r, o;

              POP(q);
              out[q] = -olabel;
      
              b = boundary_pixels_3d(q);
              for (j = 0; j < n_nbrs; j++)
                if (b[j])
                  { r = q + neighbor[j];
                    o = out[r];
                    if (o == olabel)
                      { PUSH(r);
                        out[r] = 0;
                      }
                    else
                      { o = -o;
                        if (o > 0 && o != olabel)
                          { o = seeds[o];
                            adjacent[o] = 1;
                            if (o > alargest)
                              alargest = o;
                          }
                      }
                  }
            }

          { int j;

            for (j = 1; j <= alargest; j++)
              if (! adjacent[j])
                break;

            seeds[olabel] = j;
            if (j > nlabels)
              { nlabels = j;
                if (j > 255)
                  { fprintf(stderr,"Warning: More than 255 colors needed to label watershed\n");
                    fprintf(stderr,"         Returning NULL result\n");
                    fflush(stderr);
                    return (NULL);
                  }
              }

            for (j = 1; j <= alargest; j++)
              adjacent[j] = 0;
          }
        }
  }

  //  Make an image with the assigned labels, and make seeds contain the lexicographically
  //    smalles pixel in each region.

  { int    p, b;
    uint8 *iarray;

    result->nlabels = nlabels;
    result->labels  = Make_Stack(GREY,cwidth,cheight,cdepth);
    iarray = result->labels->array;

    for (p = 0; p < cvolume; p++)
      { out[p] = b = -out[p];
        iarray[p] = seeds[b];
      }
    for (p = 0; p < cvolume; p++)
      if (seeds[b = out[p]]  > 0) 
        seeds[b] = -(p+1); 
    for (b = 1; b <= nbasins; b++)
      seeds[b] = -seeds[b];
  }

  { int    p;
    int   *color;
    Stack *view;

    view = Make_Stack(COLOR,frame->width,frame->height,frame->depth);

    for (p = 0; p < cvolume; p++)
      { if (out[p] <= 0)
          { printf("Unassigned pix\n");
            continue;
          }
        color = palette[(out[p]-1)%8];
        view->array[3*p]   = color[0];
        view->array[3*p+1] = color[1];
        view->array[3*p+2] = color[2];
      }

    Write_Stack("watershed.tif",view);

    Kill_Stack(view);
  }

  return (result);
}
