Notes on the source
===============

This project has moved from a development stage, where we were figuring out
exactly how to solve the problem, to more of a production stage.  If you try to
read the source code, you'll find that a lot of the development code has been
left in. Caveat emptor.

Building
======

This package uses a [Cmake][] based build system.  The basic build steps are similar
under Windows, OS X, and Linux.  On a unix-y system, the procedure looks like this:

# Install CMake 2.8 or better.

# Install bison if it's not already available on your system.

   For windows, use [GnuWin32 Bison](http://gnuwin32.sourceforge.net/packages/bison.htm).

# Install awk. 
   
   For windows, use [GnuWin32 Gawk](http://gnuwin32.sourceforge.net/packages/gawk.htm).

# On Windows and OS X, you'll have to install developer tools.  On Windows,
   that's Visual Studio; one of the free "Express Edition" versions is fine.
   On OS X, the "XCode developer tools" should be downloadable for free.

# Start in the root of the source directory (in a terminal).
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

# Type these commands:

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

Whisker tracking
============

File formats
-----------------

### Whiskers

Whiskers can be stored in either a text or binary format.  There is a command
line utility, `whisker_convert`, provided to convert between different
supported formats.  `batch.py` can be used to speed up the process of
converting many files.

The most detailed description of each format can be found in the corresponding
`whisker_io_*.c` files implimenting the readers/writers for each format.

In general, each whisker segment in a file is comprised of a unique id and a 
number of data points describing the shape as output from the tracing program.

### Trajectories

A text file.  Each line corresponds to a whisker segment.  The line is formated
as:

    trajectory_id, frame_id, whisker_id

The frame_id, whisker_id pair uniquely specify the whisker.  The trajectory_id
is an integer that acts like a label.

### Bar

A text file.  Each line corresponds to a frame.  The line is formated as:

    frame_id, x, y

where x, y are real numbers giving the position in pixels, and frame_id is a
natural number indicating the correspondng time point in the movie (in frames).

Graphical interface
---------------------------

See ui/README for instructions on how to use the graphical user interface.

ui2.py --help
