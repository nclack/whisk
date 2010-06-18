function M = read_whisker_measurements(filename)
%% Reads measurements files output from whisker tracking
%
% CAVEAT   
% This function only reads the "V1" measurements format.
%
% See "measurements_convert" help for more information
% about available file formats and converting to/from
% the different formats.
%
% USAGE
% M = read_whisker_measurements(filename)
%
% <filename> is the full path to the file.
% <M> is the output measurements matrix.  It has the
%     following column format:
%
%     1.  whisker identity (-1:other, 0,1,2...:Whiskers)
%     2.  time (frame #)
%     3.  segment id
%     4.  length (px)
%     5.  tracing score
%     6.  angle at follicle (degrees)
%     7.  mean curvature (1/px)
%     8.  follicle position: x (px)
%     9.  follicle position: y (px)
%     10. tip position: x (px)
%     11. tip position: y (px)
%
%     and optionally (with a provided .bar file)
%     12. distance to center of bar
%
% EXAMPLE
%
% %% plot time course of follicle angle for whisker "0."
% >> M = read_whisker_measurements("test.measurements");
% >> mask = M(:,1)==0;   % select whisker "0"
% >> plot( M(mask,2), M(mask,6) );
%
%
%--------------------------------------------------------------------------------%
% Author: Nathan Clack <clackn@janelia.hhmi.org> 
%
% Copyright 2010 Howard Hughes Medical Institute.
% All rights reserved.
% Use is subject to Janelia Farm Research Campus Software Copyright 1.1
% license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
%

%% open
fid = fopen( filename, 'rb' );

try
  %% check format signature
  esig = 'measV1';
  sig = fread(fid,8,'uint8=>char')';
  assert( strncmp(esig, sig, length(esig) ), ['Wrong format for measurements file: ',filename]);

  %% read dimensions and alloc
  nrows = fread(fid,1,'int32');
  nmeas = fread(fid,1,'int32');

  M = zeros( nrows, nmeas+3 );

  %% read in row by row
  for i=1:nrows,
    [row,count] = fread(fid,10,'int32');
    assert( count==10, ['Could not read the measurements file: ',filename] );
    M(i,1:3) = row( [4 2 3] );
    [data,count] = fread(fid, nmeas, 'float64');
    assert( count==nmeas, ['Could not read the measurements file: ',filename] );
    M(i,4:end) = data;
    sts = fseek( fid, nmeas*8, 'cof' );
    assert(sts==0, ['Could not read the measurements file: ',filename] );
  end
  fclose(fid);
catch ME
  fclose(fid);
  rethrow(ME);
end


%% Some relevant c-code.
%  See traj.h and measurements_io_v1.c
%
% typedef struct _Measurements
% { int row;           // offset from head of data buffer ... Note: the type limits size of table
%   int fid;
%   int wid;
%   int state;
%
%   int face_x;        // used in ordering whiskers on the face...roughly, the center of the face
%   int face_y;        //                                      ...does not need to be in image
%   int col_follicle_x; // index of the column corresponding to the folicle x position
%   int col_follicle_y; // index of the column corresponding to the folicle y position
%                                                                           
%   int valid_velocity;
%   int n;
%   double *data;     // array of n elements
%   double *velocity; // array of n elements - change in data/time
% } Measurements;
%
% Measurements *read_measurements_v1( FILE *fp, int *n_rows)
% { Measurements *table, *row;
%   static const int rowsize = sizeof( Measurements ) - 2*sizeof(double*); //exclude the pointers
%   double *head;
%   int n_measures;
%
%   fread( n_rows, sizeof(int), 1, fp);
%   fread( &n_measures, sizeof(int), 1, fp );
%
%   table = Alloc_Measurements_Table( *n_rows, n_measures );
%   head = table[0].data;
%   row = table + (*n_rows);
%
%   while( row-- > table )
%   { fread( row, rowsize, 1, fp );
%     row->row = (row->data - head)/sizeof(double);
%     fread( row->data, sizeof(double), n_measures, fp);
%     fread( row->velocity, sizeof(double), n_measures, fp);
%   }
%   return table;
% }
%
