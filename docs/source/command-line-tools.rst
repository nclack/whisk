.. command-line-tools:

Command line tools
==================

Concepts
--------

Interchangeable parts
,,,,,,,,,,,,,,,,,,,,,

There are four command line program to trace and track whiskers in a movie.
Each of these programs represents a different step in the tracking procedure.
By keeping these steps separated, rather then glomming everything into a single
command, one can swap out any of these steps with a customized tool.

The most useful example of this is the classify step. :ref:`classify`
provides a first guess as to which traces are whiskers and which are not using
a few heuristic rules.  These might change form experiment to experiment.  It's
possible to write your own Matlab script, for example, to replace this step,
and then feed the results into :ref:`reclassify`.

Tracing
,,,,,,,

Videos are automatically analyzed to estimate backbone positions (with subpixel
precision) for segments of whiskers, facial hairs, and other dark line-line
objects.  We try to take an approach favoring *false positives*; it is better
to trace too many things than to fail to detect the presence of a whisker.

Each frame of a video is treated independantly.  Subsequent steps will try to
assign labels to traced segments identifying which segments belong to which
whisker.

Measurement
,,,,,,,,,,,

Rather than compare the point-by-point representation of the backbone of a
traced segment, a set of features descriptive of the shape are used for
comparison.

.. _`features table`:

*Table 1:* Features

.. tabularcolumns:: |L|J|

+----------------------------+-----------------------------------------------+ 
|  feature                   | description                                   | 
+============================+===============================================+ 
|       time                 | The index of the frame in the video on        | 
|                            | which the traced segment was found.           | 
+----------------------------+-----------------------------------------------+ 
|        length              | Length of the traced path in pixels.          | 
+----------------------------+-----------------------------------------------+ 
|        tracing score       | Tracing involves finding the optimal          | 
|                            | correlation between a parameterized           | 
|                            | filter and the image at a point.  This        | 
|                            | optimal value is a score.  This is the        | 
|                            | median of these scores evaluated at points    | 
|                            | along a traced segment.                       | 
+----------------------------+-----------------------------------------------+ 
|        angle at follicle   | Degrees of deflection away from the animal's  | 
|                            | left-right axis.                              | 
+----------------------------+-----------------------------------------------+ 
|  mean :term:`curvature`    | The average curvature over the traced path    | 
|                            | in units of `1/pixel`.                        | 
+----------------------------+-----------------------------------------------+ 
|  :term:`follicle` position | (x,y) point in pixels.                        | 
+----------------------------+-----------------------------------------------+ 
|  :term:`tip` position      | (x,y) point in pixels.                        | 
+----------------------------+-----------------------------------------------+ 

Classification
,,,,,,,,,,,,,,

Two steps are used to identify which segements belong to which whiskers
throughout a video uses two steps.  A first guess is made that separates
segments belonging to whiskers from other traced segments.  This first guess is
used to generate statistics based on measured features.  These statistics are
used in a second pass, together with the idea that whiskers maintain an order
along the face, in order to refine the guess and assign specific identities for
different imaged whiskers.

Reference
---------

.. note::

  These programs require :doc:`special-files` to be present in the :term:`current
  working directory`.  

.. _trace:

:program:`trace`
,,,,,,,,,,,,,,,,

Performs fully automated detection and tracing of all whiskers.

**Usage**::

  trace <video> <destination.whiskers>

.. program:: trace

.. cmdoption:: <video>

   The path to the video file.  Can be in :term:`StreamPix SEQ`, :term:`TIFF`, or any
   format readable by :term:`FFMPEG`.

.. cmdoption:: <destination.whiskers>

   The path to the file to which results will be saved.

**Example**::

  trace path/to/data/movie.mp4 path/to/data/result.whiskers

.. _measure:

:program:`measure`
,,,,,,,,,,,,,,,,,,

Measures several properties of the traced segments output from :ref:`trace`.
Outputs a table with an entry for each traced segment and fields for the features
listed in the `features table`_.


**Usage**::

  measure --help
  measure --face <side>         <source.whiskers> <destination.measurements>
  measure --face <x> <y> <axis> <source.whiskers> <destination.measurements>

.. program:: measure

.. cmdoption:: -h, --help

  Displays a help message.

.. cmdoption:: --face <side>, --face <x> <y> <axis>

  `<side>` may be `left`, `right`, `top`, or `bottom`.

  Indicates the side of the image along which the face is oriented.  The
  value effects how follicle and tip position are estimated.
  
  `<side>` is used for videos recorded in such a way that the mouse face lies
  on one side of the image with the whiskers protruding into the field.
  Using `<side>` may not work well if the face is not confined to one side of
  the image or when the whisker pad is located near a corner of the image.

  When `<side>` doesn't work well, use `<x>` and `<y>` should be the
  approximate coordinates of the center of the whisker pad.  `<axis>` should be
  `x` or `h` if the face (the anterior-posterior axis) lies mostly in along the
  image's horizontal axis.  Otherwise, `<axis>` should be `y` or `v` to
  indicate the vertical axis.

.. cmdoption:: <source.whiskers>

  The source file containing the traced whisker shapes.

.. cmdoption:: <destination.measurements>

  The destination file containing the table of computed features.  This file
  will also be used to keep track of whisker identities in later steps.

**Examples**::

  measure --face left path/to/data/result.whiskers path/to/data/result.measurements

::

  measure --face 30 420 x path/to/data/result.whiskers path/to/data/result.measurements

.. _classify:

:program:`classify`
,,,,,,,,,,,,,,,,,,,

Uses a length threshold to seperate whisker segments from other traced segments
(e.g.  facial hairs).  Looks for frames with the expected number of long
segments and provides a preliminary label according to their order on the face.

**Usage**::

  classify --help
  classify <side>         <source.measurements> <destination.measurements> --px2mm <double> -n <int> [--limit<low>:<high>] [--follicle <int>]
  classify <x> <y> <axis> <source.measurements> <destination.measurements> --px2mm <double> -n <int> [--limit<low>:<high>] [--follicle <int>]

.. program:: classify

.. cmdoption:: -h, --help

  Displays a help message.

.. cmdoption:: <side>, <x> <y> <axis>

  `<side>` may be `left`, `right`, `top`, or `bottom`.

  Indicates the side of the image along which the face is oriented.  The
  value effects how follicle and tip position are estimated.
  
  `<side>` is used for videos recorded in such a way that the mouse face lies
  on one side of the image with the whiskers protruding into the field.
  Using `<side>` may not work well if the face is not confined to one side of
  the image or when the whisker pad is located near a corner of the image.

  When `<side>` doesn't work well, use `<x>` and `<y>` should be the
  approximate coordinates of the center of the whisker pad.  `<axis>` should be
  `x` or `h` if the face (the anterior-posterior axis) lies mostly in along the
  image's horizontal axis.  Otherwise, `<axis>` should be `y` or `v` to
  indicate the vertical axis.

.. cmdoption:: <source.measurements>

  The source file containing measured features of traced segments.

.. cmdoption:: <destination.measurements>

  The destination file. This may be the same as `<source.measurements>`.
  Contains the table of measurements in `<source.measurements>` with updated
  assigned identities.

.. cmdoption:: --px2mm <double>

  The pixel size in millimeters.  Should be in units of `mm/px`.

.. cmdoption:: -n <int>

  The number of  segments longer than the length threshold expected in most
  frames.  Typically, this is the number of imaged whiskers.

  If set to -1, :program:`classify` will choose the lowest length threshold
  that maximizes the number of frames with the same number of long segments.

.. cmdoption:: --limit<low>:<high>

  Optional.

  :program:`classify` adaptively finds the a minimum length threshold that
  yields the most number of frames with long whiskers. Use `<low>` and `<high>`
  to bound the length threshold.  These should have in units of `mm`.

.. cmdoption:: --follicle <int>

  Optional.

  Only count follicles that lie on one side of the line specified by this
  threshold (in pixels).  The direction of the line points along the x or y
  axis depending which is closer to the orientation of the mouse's face.
 

**Example**

Run classify for a video with the face on the left of the image with a pixel
size of 40 µm.  Label a segment as a whisker if the segment is longer than a
threshold between 1 mm and 50.0 mm and if the follicle is found on the left of
the vertical line: `x=100 px`. Automatically try to determine the number of
whiskers.::

  classify left source.measurements destination.measurements --px2mm 0.04 -n -1 --limit1.0:50.0 --follicle 100

.. _reclassify:

:program:`reclassify`
,,,,,,,,,,,,,,,,,,,,,

Refines an initial guess of segment identities.

**Usage**::

  reclassify --help
  reclassify -n <int> <source.measurements> <destination.measurements>

.. program:: reclassify

.. cmdoption:: -h, --help

  Displays a help message.

.. cmdoption:: <source.measurements>

  The source file containing measured features of traced segments with initial
  guess's for segment identities.

.. cmdoption:: <destination.measurements>

  The destination file. This may be the same as `<source.measurements>`.
  Contains the table of measurements in `<source.measurements>` with updated
  assigned identities.

.. cmdoption:: -n <int>

  The number of whiskers imaged.  Typically, this is the same number used for
  :option:`classify -n <int>`.

  If set to -1, :program:`reclassify` will use the guess provided by 
  `<source.measurements>`.


**Example**

  reclassify -n 3 source.measurements destination.measurements

.. _whisker_convert:

:program:`whisker_convert`
,,,,,,,,,,,,,,,,,,,,,,,,,,

Converts between different formats available for measurements files.  This
utility is mainly used for updating to newer file formats.

**Usage**::

  whisker_convert -h
  whisker_convert -<source.measurements> <destination.measurements> <format>

.. program:: whisker_convert

.. cmdoption:: -h, -help

  Displays a help message.

.. cmdoption:: <source.measurements>

  The source `measurements` file.

.. cmdoption:: <destination.measurements>

  The destination file. This may be the same as `<source.measurements>`
  (the original will be overwritten).

.. cmdoption:: <format>

  The destination format to use. To get a list of available formats see::

    whisker_convert -h

.. warning::

  The `whiskpoly1` format is a lossy format and may not retain interesting
  aspects of traced whiskers such as the angle near the follicle.  Don't 
  use it.

**Example**::

  whisker_convert source.measurements destination.measurements whiskbin1

.. _measurements_convert:

:program:`measurements_convert`
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

Converts between different formats available for measurements files.  This
utility is mainly used for updating to newer file formats.

**Usage**::

  measurements_convert -h
  measurements_convert -<source.measurements> <destination.measurements> <format>

.. program:: measurements_convert

.. cmdoption:: -h, -help

  Displays a help message.

.. cmdoption:: <source.measurements>

  The source `measurements` file.

.. cmdoption:: <destination.measurements>

  The destination file. This may be the same as `<source.measurements>`
  (the original will be overwritten).

.. cmdoption:: <format>

  The destination format to use. To get a list of available formats see::

    measurements_convert -h

**Example**::

  measurements_convert source.measurements destination.measurements v1
