Whisker tracking
================

A description of this software as well as detailed instructions and a tutorial
may be found
[here](http://openwiki.janelia.org/wiki/display/MyersLab/Whisker+Tracking).

Community
=========

[Chat](https://discord.gg/Y7QJerr)

[Issue Tracker](https://github.com/nclack/whisk/issues)

Downloading
===========

Pre-built binaries are available for [download][].

[download]: http://openwiki.janelia.org/wiki/display/MyersLab/Whisker+Tracking+Downloads

See the instructions for building below.

Tip: Git on Windows
-------------------

1. Install [msysgit][].

2. You may need to run the following command so that the https protocol will work correctly

       git config --global http.sslcainfo "/c/Program Files (x86)/Git/bin/curl-ca-bundle.crt"

3. Clone the repository

       git clone https://github.com/nclack/whisk.git

[msysgit]: http://code.google.com/p/msysgit/

Building
========

This package uses a [Cmake][] based build system.  The basic build steps are similar
under Windows, OS X, and Linux.  On a unix-y system, the procedure looks like this:

1. Install CMake 2.8 or better.

2. Install bison if it's not already available on your system.

   For windows, use [GnuWin32 Bison](http://gnuwin32.sourceforge.net/packages/bison.htm).
   It's best to install it to a path that lacks spaces such as "C:\gnuwin32".

3. Install awk. 
   
   For windows, use [GnuWin32 Gawk](http://gnuwin32.sourceforge.net/packages/gawk.htm).

4. On Windows and OS X, you'll have to install developer tools.  On Windows,
   that's [Visual Studio](http://www.microsoft.com/express); one of the free
   "Express Edition" versions is fine.  On OS X, install the 
   [XCode developer tools](http://developer.apple.com/technologies/tools/).

   If you plan on building a binary redistributable on windows, you may also
   want to install the [Nullsoft Scriptable Install System](http://nsis.sourceforge.net).

5. Start in the root of the source directory (in a terminal).
   The contents of this directory look something like this:

        3rdParty/
        CMakeLists.txt
        LICENSE.txt
        README.txt
        TODO
        build/
        config.h.in
        docs/
        include/
        matlab/
        oldbuild/
        pipeline/
        python/
        src/
        ui/

6. Type these commands:

        mkdir build
        cd build
        cmake ..
        make
   
CMake works by building the Makefile, or other required project files, in the 
"build" directory you created.  You don't have to use a Makefile; XCode projects,
Eclipse projects, Visual Studio projects, etc ... can be generated.  To generate
these other project types replace the "cmake .." step with "cmake-gui ..".  The
GUI will show available options.  See the CMake documentation for more details.

[Cmake]: http://www.cmake.org


File formats
============

### Whiskers

Whiskers can be stored in either a text or binary format.  There is a command
line utility, `whisker_convert`, provided to convert between different
supported formats.  `batch.py` can be used to speed up the process of
converting many files.

The most detailed description of each format can be found in the corresponding
`whisker_io_*.c` files implimenting the readers/writers for each format.

In general, each whisker segment in a file is comprised of a unique id and a 
number of data points describing the shape as output from the tracing program.

### Measurements 

The measurements file stores the features of traced curves used for
classification, such as follicle position, angle and whisker length.
Additionally, after classification, this file records the determined identity
of each traced curve.

Graphical interface
===================

See ui/README for instructions on how to use the graphical user interface.

      ui2.py --help

Changes
=======

v1.0.0 to v1.0.2
----------------
- improved FFMpeg support

  - more video formats supported

  - newer version of FFMPEG used on windows and 64-bit support added

- change how whiskers were ordered to better support videos imaging both sides
  of a mouse's face.

- added a special heuristic classifier for tracking single whiskers

- added a special heuristic classifier adapted for tracking whiskers on both
  sides of a mouse's face.

- added Matlab mex binaries to support reading and writing of measurements
  files, as well as reading of whiskers files.

- seeding function used to initially find whiskers is now more efficient
  and provides more accurate estimates of local whisker angle.

- Improved the installer so python and Matlab functionality is better
  supported.

- added another measurements file format that remembers the face position.
  The new format is now the default.  A warning may be issued for files
  that use the old format.  Run the measurements_convert command to update
  your files, if necessary.
