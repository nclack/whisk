%% Central command to build MEX file interface
%

mex -o LoadWhiskers whisker_io.mex.c -lwhisk -L./lib -I./include
