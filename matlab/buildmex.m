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

mex LINKFLAGSPOST=/NODEFAULTLIB:LIBCMT -DLOAD_MEASUREMENTS -output mexLoadMeasurements src/measurements_io.mex.c  -L. -lwhiskio -I../include 
mex LINKFLAGSPOST=/NODEFAULTLIB:LIBCMT -DSAVE_MEASUREMENTS -output mexSaveMeasurements src/measurements_io.mex.c  -L. -lwhiskio -I../include 
mex LINKFLAGSPOST=/NODEFAULTLIB:LIBCMT -output mexLoadWhiskers src/whisker_io.mex.c -L. -lwhiskio -I../include
