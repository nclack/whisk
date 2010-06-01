/*****************************************************************************************\
*                                                                                         *
*  Watershed Partitioning                                                                 *
*                                                                                         *
*  Author:  Gene Myers                                                                    *
*  Date  :  June 2007                                                                     *
*                                                                                         *
\*****************************************************************************************/

#ifndef _WATERSHED

#define _WATERSHED

#include "image_lib.h"

typedef struct _Watershed_2D
  { int    nlabels;   // The number of distinct catchment basin (CB) labels (1..nlabels)
    Image *labels;    // A GREY image whose values are the CB labels
    int    nbasins;   // The number of CBs in the partition
    int   *seeds;     // A pixel in each CB from which one can flood fill or contour
    int    iscon4;    // 4-connected (vs. 8-connected) regions?
  } Watershed_2D;

//  Watershed partition (we use a modification of Vincent & Soille, IEEE. Trans. on Pat.
//    Analysis & Mach. Learning, 13, 6 (1991)) an image using either 4- (con4 on) or 8-
//    (con4 off) connectivity.  The image must be either GREY or GREY16.  In the "labels"
//    image that is returned the set of pixels in a given catchment basin (CB) have the
//    same GREY value and values are assigned to CBs in such a way that two CBs with the
//    same label are never adjacent.  Typically no more than handful of labels are used
//    so encoding them in the GREY image "labels" above is safe (no attempt is made to
//    optimize, we just greedily color the 10's of thousands of CBs that can result, but
//    typically never need more than 8 or 9 colors/labels to color the watershed partition)

Watershed_2D *Build_2D_Watershed(Image *image, int con4);

Watershed_2D *Copy_Watershed_2D(Watershed_2D *part);
void          Pack_Watershed_2D(Watershed_2D *part);
void          Free_Watershed_2D(Watershed_2D *part);
void          Kill_Watershed_2D(Watershed_2D *part);
void          Reset_Watershed_2D();
int           Watershed_2D_Usage();


// 3D Watershed (untested !!)

typedef struct _Watershed_3D
  { int    nlabels;   // The number of distinct catchment basin (CB) labels (1..nlabels)
    Stack *labels;    // A GREY stack whose values are the CB labels
    int    nbasins;   // The number of CBs in the partition
    int   *seeds;     // A pixel in each CB from which one can flood fill or contour
    int    iscon6;    // 6-connected (vs. 26-connected) regions?
  } Watershed_3D;

Watershed_3D *Build_3D_Watershed(Stack *stack, int con6);

Watershed_3D *Copy_Watershed_3D(Watershed_3D *part);
void          Pack_Watershed_3D(Watershed_3D *part);
void          Free_Watershed_3D(Watershed_3D *part);
void          Kill_Watershed_3D(Watershed_3D *part);
void          Reset_Watershed_3D();
int           Watershed_3D_Usage();

#endif
