/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */

//
// Bar type
//
typedef struct _Bar
{ int time;
  float x;
  float y;
} Bar;

 Bar *Bar_Static_Cast( int time, float x, float y);

void        Bar_Sort_By_Time( Bar *bars, int nbars);

//
// Bar File IO 
//
typedef void BarFile;

BarFile *Bar_File_Open      ( const char* filename, const char *mode);
void     Bar_File_Close     ( BarFile *file );
void     Bar_File_Append_Bar( BarFile *file,  Bar* b );
Bar     *Read_Bars          ( BarFile *file, int *n );

Bar     *Load_Bars_From_Filename( const char *filename, int *nbars );
void     Save_Bars_To_Filename  ( const char *filename, Bar *bars, int nbars );
