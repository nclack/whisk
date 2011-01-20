/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : May 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "utilities.h"
#include "traj.h"
#include "measurements_io.h"

#define DEBUG_CLASSIFY_1
#define DEBUG_CLASSIFY_3
#if 0
#define DEBUG_MEAN_SEGMENTS_PER_FRAME_BY_TYPE
#define DEBUG_ESTIMATE_BEST_LENGTH_THRESHOLD_FOR_KNOWN_COUNT
#define DEBUG_ESTIMATE_BEST_LENGTH_THRESHOLD
#define DEBUG_MEASUREMENTS_TABLE_LABEL_BY_ORDER
#endif 

void Measurements_Table_Pixel_Support( Measurements *table, int n_rows, int *maxx, int *maxy )
{ 
  int x = 0, y = 0;
  Measurements *row = table+n_rows;
  while( row-- > table)  
  { double *d = row->data;
    x = MAX(x, d[4]);
    y = MAX(y, d[5]);
    x = MAX(x, d[6]);
    y = MAX(y, d[7]);
  }
  *maxx = x;
  *maxy = y;
}

void Helper_Get_Face_Point( char* directive, int maxx, int maxy, int* x, int *y)
{ static char *options[] = { "top",
                             "left",
                             "bottom",
                             "right",
                             NULL};
  int iopt = 0;
  while( options[iopt] && strncmp( options[iopt], directive, 10 ) != 0 )
    iopt++;

  switch(iopt)
  { 
    case 0:
      *x =   maxx/2;   *y =  -maxy/2;   break;
    case 1:
      *x =  -maxx/2;   *y =   maxy/2;   break;
    case 2:
      *x =   maxx/2;   *y = 3*maxy/2;   break;
    case 3:
      *x = 3*maxx/2;   *y =   maxy/2;   break;
    default:
      error("Directive supplied to Helper_Get_Face_Point could not be recognized.\n");
  }
}

void Helper_Get_Follicle_Const_Axis( char* directive, int maxx, int maxy, int* col, int *is_gt, int *high)
{ static char *options[] = { "top",
                             "left",
                             "bottom",
                             "right",
                             NULL};
  int iopt = 0;
  static const int x = 4,
                   y = 5;
  while( options[iopt] && strncmp( options[iopt], directive, 10 ) != 0 )
    iopt++;

  switch(iopt)
  { 
    case 0:
      *col = y;  *is_gt = 1; *high = maxy;  break;
    case 1:                   
      *col = x;  *is_gt = 0; *high = maxx;  break;
    case 2:                   
      *col = y;  *is_gt = 0; *high = maxy;  break;
    case 3:                   
      *col = x;  *is_gt = 1; *high = maxx;  break;
    default:
      error("Directive supplied to Helper_Get_Follicle_Const_Axis could not be recognized.\n");
  }
}

inline void Measurements_Table_Label_By_Threshold( Measurements *table, int n_rows, int col, double threshold, int is_gt )
{ Measurements *row = table + n_rows;
  if(is_gt)
  { while(row-- > table)
      row->state = ( row->data[col] >  threshold );
  } else
  { while(row-- > table)
      row->state = ( row->data[col] <= threshold );
  }
}

inline void Measurements_Table_Label_By_Threshold_Or( Measurements *table, int n_rows, int col, double threshold, int is_gt )
{ Measurements *row = table + n_rows;
  if(is_gt)
  { while(row-- > table)
      row->state |= ( row->data[col] >  threshold );
  } else
  { while(row-- > table)
      row->state |= ( row->data[col] <= threshold );
  }
}

inline void Measurements_Table_Label_By_Threshold_And( Measurements *table, int n_rows, int col, double threshold, int is_gt )
{ Measurements *row = table + n_rows;
  if(is_gt)
  { while(row-- > table)
      row->state &= ( row->data[col] >  threshold );
  } else
  { while(row-- > table)
      row->state &= ( row->data[col] <= threshold );
  }
}

int Measurements_Table_Best_Frame_Count_By_State( Measurements *table, int n_rows, int label, int *argmax )
{ int hist[64];    // make this larger if imaging more whiskers - this has to be greater than the largest observed count
  Measurements *row = table + n_rows;
  int counter = 0;
  int last = table->fid;

  memset( hist,0, sizeof(hist) );

  while( row-- > table )         // Build histogram
  { int fid = row->fid;
    if( fid - last != 0 )
    { last = fid;
      hist[counter]++;
      counter = 0;
    }
    if( row->state ) counter++;
  }

  { int max = -1;                // max,argmax on hist
    int *h = hist + 64;
    while(h-- > hist)
    { if( (*h)>max )
      { max = *h;
        *argmax = h-hist;
      }
    }
    return max;
  }
}

//assumes measurements table sorted by time
double Measurements_Table_Estimate_Best_Threshold( Measurements *table, int n_rows, int column, double low, double high, int is_gt, int *target_count )
{ double thresh;
  int best = -1.0;
  double argmax;
  assert(low<high);
  for( thresh = low; thresh < high; thresh ++ )
  { int count, n;
    Measurements_Table_Label_By_Threshold( table, n_rows, column, thresh, is_gt );
    count = Measurements_Table_Best_Frame_Count_By_State( table, n_rows, 1, &n );
#ifdef DEBUG_ESTIMATE_BEST_LENGTH_THRESHOLD
    printf("%4f  %3d  %3d\n", thresh, count, n );
#endif
    if( count > best && n>0)
    { best = count;
      argmax = thresh;
      if( target_count)
        *target_count = n;
    }
  }
  return argmax;
}

//assumes measurements table sorted by time
double Measurements_Table_Estimate_Best_Threshold_For_Known_Count( Measurements *table, int n_rows, int column, double low, double high, int is_gt, int target_count )
{ double thresh;
  int best = -1.0;
  double argmax;
  assert(low<high);
  for( thresh = low; thresh < high; thresh ++ )
  { int n_frames_w_target=0;
    Measurements_Table_Label_By_Threshold( table, n_rows, column, thresh, is_gt );
    // count number of frames with exactly `target_count` segments above threshold
    { Measurements *row = table+n_rows;
      int n_seg_above_thresh = 0;
      int last = table->fid;
      while( row-- > table )
      { int fid = row->fid;
        if( fid - last != 0 )
        { last = fid;
          if(n_seg_above_thresh == target_count) n_frames_w_target++;
          n_seg_above_thresh=0;
        }
#ifdef DEBUG_ESTIMATE_BEST_LENGTH_THRESHOLD_FOR_KNOWN_COUNT
        assert(row->state == 0 || row->state == 1);
#endif
        n_seg_above_thresh += row->state; // row->state is either 0 or 1;
      }
    }
#ifdef DEBUG_ESTIMATE_BEST_LENGTH_THRESHOLD_FOR_KNOWN_COUNT
    printf("%4.2f   %3d\n", thresh, n_frames_w_target );
#endif
    if( n_frames_w_target > best )
    { best = n_frames_w_target;
      argmax = thresh;
    }
  }
  return argmax;
}

void Measurements_Table_Label_By_Order( Measurements *table, int n_rows, int target_count )
{ Sort_Measurements_Table_Time_State_Face( table, n_rows );
  assert(n_rows);
  n_rows--;
  while(n_rows>=0)
  { int fid = table[n_rows].fid;
    int count = 1;
    int j = n_rows;                                         // First pass: count the state==1 
    while( j-- && table[j].state==1 && table[j].fid==fid )  //  already counted the first row
        count ++;
#ifdef DEBUG_MEASUREMENTS_TABLE_LABEL_BY_ORDER
    debug("Frame %5d: Count = %5d\n",fid,count);
#endif
    j = n_rows;                                                          
    if( count == target_count )           // Second pass: label          
      while( j>=0 && table[j].state==1 && table[j].fid==fid )  //  relabel the good ones
        table[j--].state = --count;
    while( j>=0 && table[j].fid==fid )    //  relabel the bad ones as -1
      table[j--].state = -1;
    n_rows = j;
  }
}

#ifdef TEST_CLASSIFY_1
char *Spec[] = {"[-h|--help] |",
                "<source:string> <dest:string>",
                "(<face:string> | <x:int> <y:int> <axis:string>)",
                "--px2mm <double>",
                "-n <int>", 
                "[--limit<double(1.0)>:<double(50.0)>]",
                "[--follicle <int>]",
                NULL};
int main(int argc, char* argv[])
{ int n_rows, count;
  Measurements *table,*cursor;
  double thresh,
         px2mm,
         low_px,
         high_px;
  int face_x, face_y;
  int follicle_thresh = 0,
      follicle_col = 4,
      follicle_high,
      is_gt = 1;
  int n_cursor;

  Process_Arguments( argc, argv, Spec, 0);

  if( Is_Arg_Matched("-h") | Is_Arg_Matched("--help") )
  { Print_Argument_Usage(stdout,0);
    printf("--------------------------                                                   \n"
          " Classify test 1 (autotraj)                                                   \n"
          "---------------------------                                                   \n"
          "                                                                              \n"
          "  Uses a length threshold to seperate hair/microvibrissae from main whiskers. \n"
          "  Then, for frames where the expected number of whiskers are found,           \n"
          "  label the whiskers according to their order on the face.                    \n"
          "\n"
          "  <source> Filename with Measurements table.\n"
          "  <dest>   Filename to which labelled Measurements will be saved.\n"
          "           This can be the same as <source>.\n"
          "  <face>\n"
          "  <x> <y> <axis>\n"
          "           These are used for determining the order of whisker segments along \n"
          "           the face.  This requires an approximate position for the center of \n"
          "           the face and can be specified in pixel coordinates with <x> and <y>.\n"
          "           <axis> indicates the orientaiton of the face.  Values for <axis> may\n"
          "           be 'x' or 'h' for horizontal. 'y' or 'v' indicate a vertical face. \n"
          "           If the face is located along the edge of the frame then specify    \n"
          "           that edge with 'left', 'right', 'top' or 'bottom'.                 \n"
          "  --px2mm <double>\n"
          "           The length of a pixel in millimeters.  This is used to determine   \n"
          "           appropriate thresholds for discriminating hairs from whiskers.     \n"
          "  -n <int> (Optional) Optimize the threshold to find this number of whiskers. \n"
          "           If this isn't specified, or if this is set to a number less than 1 \n"
          "           then the number of whiskers is automatically determined.           \n"
          "  --follicle <int>\n"
          "           Only count follicles that lie on one side of the line specified by \n"
          "           this threshold (in pixels).  The direction of the line points      \n"
          "           along the x or y axis depending which is closer to the orientation \n"
          "           of the mouse's face.\n"
          "--                                                                            \n");
    return 0;
  }

  px2mm   = Get_Double_Arg("--px2mm");
  low_px  = Get_Double_Arg("--limit",1) / px2mm;
  high_px = Get_Double_Arg("--limit",2) / px2mm;
#ifdef DEBUG_CLASSIFY_1
  debug("px/mm %f\n"
        "  low %f\n"
        " high %f\n", px2mm, low_px, high_px );
#endif

  table  = Measurements_Table_From_Filename ( Get_String_Arg("source"), NULL, &n_rows );
  if(!table) error("Couldn't read %s\n",Get_String_Arg("source"));
  Sort_Measurements_Table_Time(table,n_rows);

  if( Is_Arg_Matched("face") )
  { int maxx,maxy;
    Measurements_Table_Pixel_Support( table, n_rows, &maxx, &maxy );
    Helper_Get_Face_Point( Get_String_Arg("face"), maxx, maxy, &face_x, &face_y);
    Helper_Get_Follicle_Const_Axis( Get_String_Arg("face"), maxx, maxy, 
                                    &follicle_col, &is_gt, &follicle_high);
    follicle_thresh = (is_gt) ? 0 : follicle_high;
#ifdef DEBUG_CLASSIFY_1
	debug("maxx: %d\n"
		  "maxy: %d\n", maxx, maxy );
#endif
  } else 
  { int maxx,maxy;
    const char *axis = Get_String_Arg("axis");
    static const int x = 4,
                     y = 5;
    Measurements_Table_Pixel_Support( table, n_rows, &maxx, &maxy );
    face_x = Get_Int_Arg("x");
    face_y = Get_Int_Arg("y");
    follicle_thresh = 0;       // set defaults
    is_gt = 1;
    if( Is_Arg_Matched("--follicle") && Get_Int_Arg("--follicle")>0 )
    { follicle_thresh = Get_Int_Arg("--follicle");
      switch( axis[0] )        // respond to <follicle> option
      { case 'x':              // follicle must be between threshold and face
        case 'h':
          is_gt = follicle_thresh < face_y;
          follicle_col = y;
          break;
        case 'y':
        case 'v':
          is_gt = follicle_thresh < face_x;
          follicle_col = x;
          break;
        default:
          error("Could not recognize <axis>.  Must be 'x','h','y', or 'v'.  Got %s\n",axis);
      }
    }
  }
  // Follicle location threshold
  if( Is_Arg_Matched("--follicle") && Get_Int_Arg("--follicle")>0 )
    follicle_thresh = Get_Int_Arg("--follicle");
  Measurements_Table_Label_By_Threshold    ( table, 
                                             n_rows, 
                                             follicle_col,
                                             follicle_thresh,
                                             is_gt);

#ifdef DEBUG_CLASSIFY_1
  debug("   Face Position: ( %3d, %3d )\n", face_x, face_y);
#endif
  // Shuffle to select subset with good follicles
  Sort_Measurements_Table_State_Time( table, n_rows );
  { cursor = table;
    while( (cursor->state == 0) && (cursor < table+n_rows ) )
      cursor++;
    n_cursor = n_rows - (cursor-table);
  }
  Sort_Measurements_Table_Time(cursor,n_cursor); //resort selected by time

#ifdef DEBUG_CLASSIFY_1
  { Measurements *row = cursor + n_cursor; //Assert all state==1
    while(row-- > cursor)
      assert(row->state == 1);
  }
#endif
  //
  // Estimate best length threshold and apply
  //
  if( Is_Arg_Matched("-n") && ( (count = Get_Int_Arg("-n"))>=1 ) )
  { thresh = Measurements_Table_Estimate_Best_Threshold_For_Known_Count( cursor, //table, 
                                                                         n_cursor, //n_rows, 
                                                                         0 /*length column*/, 
                                                                         low_px, 
                                                                         high_px, 
                                                                         1, /*use > */
                                                                         count );
  } else 
  { thresh = Measurements_Table_Estimate_Best_Threshold( cursor, //table, 
                                                         n_cursor, //n_rows, 
                                                         0 /*length column*/, 
                                                         low_px, 
                                                         high_px,
                                                         1, /* use > */
                                                         &count );
  }
  Measurements_Table_Label_By_Threshold    ( cursor,
                                             n_cursor,
                                             follicle_col,
                                             follicle_thresh,
                                             is_gt);
#ifdef DEBUG_CLASSIFY_1
  { Measurements *row = cursor + n_cursor; //Assert all state==1
    while(row-- > cursor)
      assert(row->state == 1);
  }
#endif
  Measurements_Table_Label_By_Threshold_And ( cursor,
                                              n_cursor,
                                              0 /*length column*/, 
                                              thresh,
                                              1 /*use gt*/);
  
#ifdef DEBUG_CLASSIFY_1
  debug("   Length threshold: %f\n"
        "       Target count: %d\n"
        "Follicle pos thresh: %c %d\n",
        thresh,count,(is_gt?'>':'<'),follicle_thresh ); 
#endif

  Measurements_Table_Set_Constant_Face_Position     ( table, n_rows, face_x, face_y);
  Measurements_Table_Set_Follicle_Position_Indices  ( table, n_rows, 4, 5 );

  Measurements_Table_Label_By_Order(table, n_rows, count ); //resorts

  Measurements_Table_To_Filename( Get_String_Arg("dest"), NULL, table, n_rows );
  Free_Measurements_Table(table);
  return 0;
}
#endif


#ifdef TEST_CLASSIFY_2
char *Spec[] = {"<source:string>", NULL};
int main(int argc, char* argv[])
{ int n_rows;
  Measurements *table;
  double mean;
  
  printf("-----------------                                                             \n"
         " Classify test 2                                                              \n"
         "-----------------                                                             \n"
         "                                                                              \n"
         "  Uses a length threshold to seperate hair/microvibrissae from main whiskers. \n"
         "  A histogram method is used to measure the expected number of whiskers per   \n"
         "  frame.  A range of thresholds is examined.                                  \n"
         "--                                                                            \n");

  Process_Arguments( argc, argv, Spec, 0);
  table = Measurements_Table_From_Filename( Get_String_Arg("source"), NULL, &n_rows );
  if(!table) error("Couldn't read %s\n",Get_String_Arg("source"));
  
  { int thresh;
    for( thresh = 0; thresh < 400; thresh ++ )
    { int count, argmax;
      Measurements_Table_Label_By_Threshold( table, n_rows, 0 /*length*/, thresh, 1/*use gt*/ );
      count = Measurements_Table_Best_Frame_Count_By_State( table, n_rows, 1, &argmax );
      printf("%4d  %3d  %3d\n", thresh, count, argmax );
    }
  }
  Free_Measurements_Table(table);
}
#endif

#ifdef TEST_CLASSIFY_3
char *Spec[] = {"[-h|--help] |",
                "<source:string> <dest:string>",
                "(<face:string> | <x:int> <y:int>)",
                "-n <int>", 
                NULL};
int main(int argc, char* argv[])
{ int n_rows, count;
  Measurements *table;
  int face_x, face_y;

  Process_Arguments( argc, argv, Spec, 0);

  if( Is_Arg_Matched("-h") | Is_Arg_Matched("--help") )
  { Print_Argument_Usage(stdout,0);
    printf("-----------------------------------------                                    \n"
          " Classify test 3 (autotraj - no threshold)                                    \n"
          "------------------------------------------                                    \n"
          "                                                                              \n"
          "  For frames where the expected number of whiskers are found,                 \n"
          "  label the whiskers according to their order on the face.                    \n"
          "\n"
          "  <source> Filename with Measurements table.\n"
          "  <dest>   Filename to which labelled Measurements will be saved.\n"
          "           This can be the same as <source>.\n"
          "  <face>\n"
          "  <x> <y>  These are used for determining the order of whisker segments along \n"
          "           the face.  This requires an approximate position for the center of \n"
          "           the face and can be specified in pixel coordinates with <x> and <y>.\n"
          "           If the face is located along the edge of the frame then specify    \n"
          "           that edge with 'left', 'right', 'top' or 'bottom'.                 \n"
          "  -n <int> (Optional) Expect this number of whiskers.                         \n"
          "           If this isn't specified, or if this is set to a number less than 1 \n"
          "           then the number of whiskers is automatically determined.           \n"
          "--                                                                            \n");
    return 0;
  }

  table  = Measurements_Table_From_Filename          ( Get_String_Arg("source"), NULL, &n_rows );
  if(!table) error("Couldn't read %s\n",Get_String_Arg("source"));
  Sort_Measurements_Table_Time(table,n_rows);

  if( Is_Arg_Matched("face") )
  { int maxx,maxy;
    Measurements_Table_Pixel_Support( table, n_rows, &maxx, &maxy );
    Helper_Get_Face_Point( Get_String_Arg("face"), maxx, maxy, &face_x, &face_y);
  } else 
  {
    face_x = Get_Int_Arg("x");
    face_y = Get_Int_Arg("y");
  }
#ifdef DEBUG_CLASSIFY_3
  debug("   Face Position: ( %3d, %3d )\n", face_x, face_y);
#endif
 
  // Initialize
  Measurements_Table_Label_By_Threshold    ( table,
                                             n_rows,
                                             0, // length column
                                             0, // threshold
                                             1);// require greater than threshold
  //
  // Estimate whisker count if neccessary
  // 
  if( !Is_Arg_Matched("-n") || ( (count = Get_Int_Arg("-n"))<2 ) )
  { int n_good_frames;
    n_good_frames = Measurements_Table_Best_Frame_Count_By_State( table, n_rows, 1, &count );
	#ifdef DEBUG_CLASSIFY_3
		debug(
			  "   Frames with count: %d\n"
			  ,n_good_frames ); 
	#endif
  } 
#ifdef DEBUG_CLASSIFY_3
	debug("        Target count: %d\n"
		  ,count ); 
#endif
#ifdef DEBUG_CLASSIFY_3
  { Measurements *row = table + n_rows; //Assert all state==1
    while(row-- > table)
      assert(row->state == 1);
  }
#endif

  Measurements_Table_Set_Constant_Face_Position     ( table, n_rows, face_x, face_y);
  Measurements_Table_Set_Follicle_Position_Indices  ( table, n_rows, 4, 5 );

  Measurements_Table_Label_By_Order(table, n_rows, count ); //resorts

  Measurements_Table_To_Filename( Get_String_Arg("dest"), NULL, table, n_rows );
  Free_Measurements_Table(table);
  return 0;
}
#endif

// TODO: I may very well be doing this in the very slowest way possible
// XXX:  problem with getting a good follicle threshold estimate for known #'s
//       of whiskers.  Also, how to deal with unknown #?  I have an estimation
//       procedure for one feature dimension, but no more.  Need to do 
//       something else to simultaneously optimize.
