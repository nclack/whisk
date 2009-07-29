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
  while( options[iopt] && strncasecmp( options[iopt], directive, 10 ) != 0 )
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
char *Spec[] = {"<source:string> <dest:string> (<face:string> | <x:int> <y:int>)", NULL};
int main(int argc, char* argv[])
{ int n_rows, count;
  Measurements *table;
  double thresh;
  int face_x, face_y;

  printf("-----------------                                                             \n"
         " Classify test 1                                                              \n"
         "-----------------                                                             \n"
         "                                                                              \n"
         "  Uses a length threshold to seperate hair/microvibrissae from main whiskers. \n"
         "  Then, for frames where the expected number of whiskers are found,           \n"
         "  label the whiskers according to their order on the face.                    \n"
         "--                                                                            \n");

  Process_Arguments( argc, argv, Spec, 0);
  table  = Measurements_Table_From_Filename          ( Get_String_Arg("source"), &n_rows );
  thresh = Measurements_Table_Estimate_Best_Threshold( table, n_rows, 0 /*length column*/, 50.0, 200.0, &count );
  Measurements_Table_Label_By_Threshold              ( table, n_rows, 0 /*length column*/, thresh );
  
  printf("Length threshold: %f\n",thresh ); 
  printf("    Target count: %d\n",count ); 

  if( Is_Arg_Matched("face") )
  { int maxx,maxy;
    Measurements_Table_Pixel_Support( table, n_rows, &maxx, &maxy );
    Helper_Get_Face_Point( Get_String_Arg("face"), maxx, maxy, &face_x, &face_y);
  } else 
  {
    face_x = Get_Int_Arg("x");
    face_y = Get_Int_Arg("y");
  }

  printf("   Face Position: ( %3d, %3d )\n", face_x, face_y);
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
