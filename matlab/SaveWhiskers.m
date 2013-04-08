function SaveWhiskers(filename,whiskers)
%SaveWhiskers     Saves .whisker files
%
%   USAGE:
%
%   SaveWhiskers(filename,whiskers)
%
%   <whiskers> is a struct array with a number of fields describing the shape of traced whisker-like segments.
%              See below for details.
%
%   EXAMPLES:
%
%     >> SaveWhiskers('/Users/clackn/Desktop/whisker-mp4/whisker_data_0250.whiskers',w);
%     >> w
%
%     w =
%
%     27771x1 struct array with fields:
%         id
%         time
%         x
%         y
%         thick
%         scores
%
%  If the something goes wrong, you'll get a bunch of warnings and an exception will be thrown.
%
%  Author: Nathan Clack <clackn@janelia.hhmi.org>
%  Copyright 2013 Howard Hughes Medical Institute.
%  All rights reserved.
%  Use is subject to Janelia Farm Research Campus Software Copyright 1.1
%  license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).

errorCode=mexSaveWhiskers(filename,sanitize(whiskers));
if errorCode
  error('WhiskerTracker:LoadWhiskers','Got error code %d when saving %s.',ecode,filename)
end

  function w=sanitize(w)
    %% Make sure the struct fields have the right type
    for i=1:numel(w)
      w(i).id    =int32(w(i).id);
      w(i).time  =int32(w(i).time);
      w(i).x     =single(w(i).x);
      w(i).y     =single(w(i).y);
      w(i).thick =single(w(i).thick);
      w(i).scores=single(w(i).scores);
    end
  end
end
