function SaveMeasurements(filename,measurements)
%SaveMeasurements     Writes .measurements files.
%
%   USAGE:
%
%   measurements = WriteMeasurements(filename)
%
%   <filename>     is the path to the destination .measurements file.
%   <measurements> is a struct array with the following fields:
%
%    fid           - Video frame where the segment was found
%    wid           - id for the segment on that frame
%    label         - identity.  For tracking use -1 for `not a whisker` and 
%                    0,1,2.. for whiskers.                                  
%    face_x        - roughly, center of whisker pad, y coordinate           
%    face_y        - roughly, center of whisker pad, y coordinate           
%    length        - path length in pixels                                  
%    score         - median score from tracing                              
%    angle         - mean angle at follicle
%    curvature     - mean curvature (1/mm)
%    follicle_x    - follicle position: x (px)
%    follicle_y    - follicle position: y (px)
%    tip_x         - tip position: x (px)
%    tip_y         - tip position: y (px)
%
%   EXAMPLES:
%
%   >> table
%
%   table = 
%
%   27771x1 struct array with fields:
%       fid
%       wid
%       label
%       face_x
%       face_y
%       length
%       score
%       angle
%       curvature
%       follicle_x
%       follicle_y
%       tip_x
%       tip_y 
%
%   >> SaveMeasurements('test.measurements',table)
%


mexSaveMeasurements(filename,measurements);