function SaveMeasurements(filename,measurements)
%SaveMeasurements     Writes .measurements files.
%
%   USAGE:
%
%   SaveMeasurements(filename,measurements)
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
%
%  Author: Nathan Clack <clackn@janelia.hhmi.org>
%  Copyright 2010 Howard Hughes Medical Institute.
%  All rights reserved.
%  Use is subject to Janelia Farm Research Campus Software Copyright 1.1
%  license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).

measurements = castfield(measurements,'fid'   ,'int32');
measurements = castfield(measurements,'wid'   ,'int32');
measurements = castfield(measurements,'label' ,'int32');
measurements = castfield(measurements,'face_x','int32');
measurements = castfield(measurements,'face_y','int32');

measurements = castfield(measurements,'length'     ,'double');
measurements = castfield(measurements,'score'      ,'double');
measurements = castfield(measurements,'angle'      ,'double');
measurements = castfield(measurements,'curvature'  ,'double');
measurements = castfield(measurements,'follicle_x' ,'double');
measurements = castfield(measurements,'follicle_y' ,'double');
measurements = castfield(measurements,'tip_x'      ,'double');
measurements = castfield(measurements,'tip_y'      ,'double');
mexSaveMeasurements(filename,measurements);

% This casting garbage sure is slow, but I'm not sure of a faster way.
% ...I suppose it might be better to do the type checking and conversion
%    on the c side, but this seems easier in terms of error handling

  function out = castfield(m,lbl,tstr)
    b = cast([m(:).(lbl)],tstr);
    for i = 1:length(b)
      m(i).(lbl) = b(i);
    end
    out = m;
  end

end