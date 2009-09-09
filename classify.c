#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "utilities.h"
#include "traj.h"

#if 0
#define DEBUG_MEAN_SEGMENTS_PER_FRAME_BY_TYPE
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

inline void Measurements_Table_Label_By_Threshold( Measurements *table, int n_rows, int col, double threshold )
{ Measurements *row = table + n_rows;
  while(row-- > table)
    row->state = ( row->data[col] > threshold );
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

double Measurements_Table_Estimate_Best_Threshold( Measurements *table, int n_rows, int column, double low, double high, int *target_count )
{ double thresh;
  int best = -1.0;
  double argmax;
  assert(low<high);
  for( thresh = low; thresh < high; thresh ++ )
  { int count, n;
    Measurements_Table_Label_By_Threshold( table, n_rows, column, thresh );
    count = Measurements_Table_Best_Frame_Count_By_State( table, n_rows, 1, &n );
#ifdef DEBUG_ESTIMATE_BEST_LENGTH_THRESHOLD
    printf("%4d  %3d  %3d\n", thresh, count, n );
#endif
    if( count > best && n>0)
    { best = count;
      argmax = thresh;
      *target_count = n;
    }
  }
  return argmax;
}

//assumes measurements table sorted by time
double Measurements_Table_Estimate_Best_Threshold_For_Known_Count( Measurements *table, int n_rows, int column, double low, double high, int target_count )
{ double thresh;
  int best = -1.0;
  double argmax;
  assert(low<high);
  for( thresh = low; thresh < high; thresh ++ )
  { int n_frames_w_target=0;
    Measurements_Table_Label_By_Threshold( table, n_rows, column, thresh );
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
    printf("%4d  %3d\n", thresh, n_frames_w_target );
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
    int j = n_rows;                    // First pass: count the state==1 
    while( j-- && table[j].state==1 )  //  already counted the first row
        count ++;
    j = n_rows;                                                          
    if( count == target_count )           // Second pass: label          
      while( j>=0 && table[j].state==1 )  //  relabel the good ones
        table[j--].state = --count;
    while( j>=0 && table[j].fid==fid )    //  relabel the bad ones as -1
      table[j--].state = -1;
    n_rows = j;
  }
}

#ifdef TEST_CLASSIFY_1
char *Spec[] = {"[-h|--help] | <source:string> <dest:string> (<face:string> | <x:int> <y:int>) -n <int>", NULL};
int main(int argc, char* argv[])
{ int n_rows, count;
  Measurements *table;
  double thresh;
  int face_x, face_y;

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
          "  <x> <y>  These are used for determining the order of whisker segments along \n"
          "           the face.  This requires an approximate position for the center of \n"
          "           the face and can be specified in pixel coordinates with <x> and <y>.\n"
          "           If the face is located along the edge of the frame then specify    \n"
          "           that edge with 'left', 'right', 'top' or 'bottom'.                 \n"
          "  -n <int> (Optional) Optimize the threshold to find this number of whiskers. \n"
          "           If this isn't specified, or if this is set to a number less than 1 \n"
          "           then the number of whiskers is automatically determined.           \n"
          "--                                                                            \n");
    return 0;
  }

  table  = Measurements_Table_From_Filename          ( Get_String_Arg("source"), &n_rows );
  if( Is_Arg_Matched("-n") && ( (count = Get_Int_Arg("-n"))>=1 ) )
  { thresh = Measurements_Table_Estimate_Best_Threshold_For_Known_Count( table, 
                                                                         n_rows, 
                                                                         0 /*length column*/, 
                                                                         50.0, 
                                                                         200.0, 
                                                                         count );
  } else 
  { thresh = Measurements_Table_Estimate_Best_Threshold( table, 
                                                         n_rows, 
                                                         0 /*length column*/, 
                                                         50.0, 
                                                         200.0, 
                                                         &count );
  }
  Measurements_Table_Label_By_Threshold ( table, 
                                          n_rows, 
                                          0 /*length column*/, 
                                          thresh );
  
  progress("Length threshold: %f\n"
           "    Target count: %d\n",thresh,count ); 

  if( Is_Arg_Matched("face") )
  { int maxx,maxy;
    Measurements_Table_Pixel_Support( table, n_rows, &maxx, &maxy );
    Helper_Get_Face_Point( Get_String_Arg("face"), maxx, maxy, &face_x, &face_y);
  } else 
  {
    face_x = Get_Int_Arg("x");
    face_y = Get_Int_Arg("y");
  }

  progress("   Face Position: ( %3d, %3d )\n", face_x, face_y);
  Measurements_Table_Set_Constant_Face_Position     ( table, n_rows, face_x, face_y);
  Measurements_Table_Set_Follicle_Position_Indices  ( table, n_rows, 4, 5 );

  Measurements_Table_Label_By_Order(table, n_rows, count );

  Measurements_Table_To_Filename( Get_String_Arg("dest"), table, n_rows );
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
  table = Measurements_Table_From_Filename( Get_String_Arg("source"), &n_rows );
  
  { int thresh;
    for( thresh = 0; thresh < 400; thresh ++ )
    { int count, argmax;
      Measurements_Table_Label_By_Threshold( table, n_rows, 0 /*length*/, thresh );
      count = Measurements_Table_Best_Frame_Count_By_State( table, n_rows, 1, &argmax );
      printf("%4d  %3d  %3d\n", thresh, count, argmax );
    }
  }
  Free_Measurements_Table(table);
}
#endif
