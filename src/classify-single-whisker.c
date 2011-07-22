//
// Step 5.  Another simple labelling scheme:
//
//          1. Label the segment with follicle closest to the top of the image.
//          2. Label the rest -1.
//
// Command line usage:
// my-classify.exe <source-measurements> <destination-measurements>
//

#include "measurements_io.h"
#include "error.h"

int main(int argc, char* argv[])
{ int number_of_rows;
  Measurements *table;
  
  if( argc != 3 )
    error("Usage: %s <source-measurements> <dest-measurements>\n",argv[0]);
    
  // read the table
  table = Measurements_Table_From_Filename( argv[1],          // the filename
                                            NULL,             // the format - NULL indicate the format should be auto detected
                                           &number_of_rows ); // (output) the size of the table

  // Lines with changes are preceeded with /**/
  Sort_Measurements_Table_Time( table, number_of_rows);       // sorts rows in order of ascending frame #
  { int       i;                                              //    - rows with the same frame # will be next to each other
    double    best;                                            // 
    int       best_row;                                       // To find which data column is which feature
    const int idx_feature = 0;                                // see   measure.c:Whisker_Seg_Measure (end of function)
    for( i=0; i<number_of_rows; i++ )                         //     
      table[i].state = -1;                                    // Start off by labeling every row with -1.
    for( i=0; i<number_of_rows; )                             // ...the inner loop will increment i
    { // Initialize
      best = table[i].data[ idx_feature ];                     // get the y position for the first row for the current frame
      best_row   = i;                                         // This row is the best so far!
      // Search for best row for current frame #
      for(   ++i                                              // ...start this loop with the next frame
           ;    i<number_of_rows                              // ...don't fall off the end of the table
             && table[i].fid == table[i-1].fid                // ...but stop when the frame # changes
           ; i++ )
      { double y = table[i].data[ idx_feature ];              // Keep track of the longest segment
        if( y > best )
        { best = y;
          best_row = i;
        }
      }
      // Done with the frame so label the longest segment.
      table[ best_row ].state = 0;      
    }
  }
  
  // save the table
  Measurements_Table_To_Filename( argv[2],                    // the filename
                                  NULL,                       // the file format - NULL means use the default format
                                  table,                      // the data
                                  number_of_rows );           // the size of the table
  
  Free_Measurements_Table(table);    
  return 0; // 0 - indicates success
}