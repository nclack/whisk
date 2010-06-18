%% Central command to build MEX file interface
%

%%
% Author: Nathan Clack <clackn@janelia.hhmi.org> 
%   Date: June 2010
%
% Copyright 2010 Howard Hughes Medical Institute.
% All rights reserved.
% Use is subject to Janelia Farm Research Campus Software Copyright 1.1
% license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
%

mex -DLOAD_MEASUREMENTS -o LoadMeasurements measurements_io.mex.c -lwhisk -L. -I./include 
mex -DSAVE_MEASUREMENTS -o SaveMeasurements measurements_io.mex.c -lwhisk -L. -I./include 
mex -o LoadWhiskers whisker_io.mex.c -lwhisk -L. -I./include
