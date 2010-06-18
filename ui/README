================
Whisker tracking
================

------------
File formats
------------

Whiskers
````````

Whiskers can be stored in either a text or binary format.  There is a command
line utility, `whisker_convert`, provided to convert between different
supported formats.  `batch.py` can be used to speed up the process of
converting many files.

The most detailed description of each format can be found in the corresponding
`whisker_io_*.c` files implimenting the readers/writers for each format.

In general, each whisker segment in a file is comprised of a unique id and a 
number of data points describing the shape as output from the tracing program.

Trajectories
````````````

A text file.  Each line corresponds to a whisker segment.  The line is formated
as:

    trajectory_id, frame_id, whisker_id

The frame_id, whisker_id pair uniquely specify the whisker.  The trajectory_id
is an integer that acts like a label.

Bar
```

A text file.  Each line corresponds to a frame.  The line is formated as:

    frame_id, x, y

where x, y are real numbers giving the position in pixels, and frame_id is a
natural number indicating the correspondng time point in the movie (in frames).

-------------------
Graphical interface
-------------------

See ui/README for instructions on how to use the graphical user interface.

ui2.py --help


----------------
Batch processing
----------------

To benifit from desktop PC's with multiple CPU's (or multiple cores), multiple
processes should be started simultaneosly to analyze multiple movies. A script 
called `batch.py` is provided to facilitate this.  See `batch.py --help` for 
details.

Some example command lines:

./batch.py --help

./batch.py /path/to/data/directory

    Will process all *.seq files in the directory using all cpu cores
    Results will be placed in data directory.

./batch.py --exe="./path/to/whisk" /path/to/data -f *.tif -n 2

    Will process all *.tif files in the data directory with the specified
    executable using 2 cpus (2 worker processes).  Results will be placed in
    data directory.
