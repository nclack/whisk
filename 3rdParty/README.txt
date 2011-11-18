Dependencies
============

This package optionally integrates the ffmpeg libraries (ffmpeg.org).

On Unix and OS X, the build system will automatically configure the build by
detect the presence of ffmpeg in your standard libary path.

On Windows, a different approach is required.  Here we provide the required
static libaries and include files.  The build system will attempt to detect
their presence when configuring the build.  If you have an archetecture that
doesn't conform to whats provided (namely, 64-bit ffmpeg isn't supported on
windows), then the package will be build without ffmpeg support.

Currently, ffmpeg is the only external dependency.  It is optional.  The
<msinttypes> package provide C99 compatibility headers that visual studio
lacks.  These are only used by ffmpeg.

Other peoples code
==================
FFMPEG licenses are in the appropriate subdirectories.

The Matlab related cmake macros were pulled from the kitware bug tracker [1].
I've only used them to a limited extent.  When I downloaded the code, the
latest update to that bug had occured on May 23, 2011.  I've made a few
modifications.

[1] http://public.kitware.com/Bug/view.php?id=8207 
