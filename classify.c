#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "common.h"
#include "utilities.h"
#include "traj.h"

#if 0
#define DEBUG_MEAN_SEGMENTS_PER_FRAME_BY_TYPE
#endif 

void Identify_Whisker_By_Length( Measurements *table, int n_rows, int col, double threshold )
{ Measurements *row = table + n_rows;
  while(row-- > table)
    row->state = ( row->data[col] > threshold );
}

double Mean_Count_Segments_Per_Frame_By_Type( Measurements *table, int n_rows)
{ int maxframe = 0;
  double sum = 0;
  Measurements *row = table + n_rows;
  while( row-- > table )
  { maxframe = MAX( maxframe, row->fid );
    sum += row->state;
  }
  return sum/(maxframe+1);
}

void Label_By_Order( Measurements *table, int n_rows, int target_count )
{ Sort_Measurements_Table_Time_State_Face( table, n_rows );
  assert(n_rows);
  n_rows--;
  while(n_rows>=0)
  { int fid = table[n_rows].fid;
    int count = 1;
    int j = n_rows;
    while( j-- && table[j].state==1 )  // already counted the first row
        count ++;
    j = n_rows;
    if( count == target_count )
      while( j>=0 && table[j].state==1 )  // relabel the good ones
        table[j--].state = --count;
    while( j>=0 && table[j].fid==fid )    // relabel the bad ones as -1
      table[j--].state = -1;
    n_rows = j;
  }

}

#ifdef TEST_CLASSIFY_1
char *Spec[] = {"<source:string> <dest:string> <thresh:int> <x:int> <y:int>", NULL};
int main(int argc, char* argv[])
{ int n_rows;
  Measurements *table;
  double mean;

  Process_Arguments( argc, argv, Spec, 0);
  table = Measurements_Table_From_Filename( Get_String_Arg("source"), &n_rows );
  Measurements_Table_Set_Constant_Face_Position    ( table, n_rows, Get_Int_Arg("x"), Get_Int_Arg("y") );
  Measurements_Table_Set_Follicle_Position_Indices ( table, n_rows, 5, 6 );
  Identify_Whisker_By_Length( table, n_rows, 0 /*length*/, Get_Int_Arg("thresh") );

  mean = Mean_Count_Segments_Per_Frame_By_Type( table,n_rows );
  printf("Mean_Count_Segments_Per_Frame_By_Type: %f\n",mean ); 

  Label_By_Order(table, n_rows, round(mean) );

  Measurements_Table_To_Filename( Get_String_Arg("dest"), table, n_rows );
#if 0
  { float thresh, last = 0.0;
    for(thresh = 10; thresh<400; thresh += 10 )
    { float mean;
      Identify_Whisker_By_Length( table, n_rows, 0 /*length*/, thresh );
      mean = Mean_Count_Segments_Per_Frame_By_Type( table, n_rows );
      printf("%5.5f\t%3.3f\t%5.5f\n", thresh, mean, last-mean );
      last = mean;
    }
  }
#endif
  Free_Measurements_Table(table);
}
#endif
