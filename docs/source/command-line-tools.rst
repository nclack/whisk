.. command-line-tools:

Command line tools
==================

Concepts
--------

Interchangable parts
,,,,,,,,,,,,,,,,,,,,

There are four command line program to trace and track whiskers in a movie.
Each of these programs represents a different step in the tracking procedure.
By keepin these steps separated, rather then glomming everything into a single
command, one can swap out any of these steps with a customized tool.

The most useful example of this is the classify step. :program:`classify`
provides a first guess as to which traces are whiskers and which are not using
a few heuristic rules.  These might change form experiment to experiment.  It's
possible to write your own Matlab script, for example, to replace this step,
and then feed the results into :program:`reclassify`.

Special Files
-------------

.. warning::

  The programs relying on these files search for them in the calling directory.
  A message will be printed to the command line console indicating if one of
  these files is missing.

The :file:`default.parameters` file.
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

This file contains parameters and settings used during error reporting,
:program:`trace`, and the last step of whisker tracking, :program:`reclassify`.

.. warning::

  The file itself contains some comments describing the parameters.  However,
  the use of this file to change parameters has not been extensively tested.


.. todo::

   Describe parameters.

The :file:`.detectorbank` files.
,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

Two :file:`.detectorbank` are used to perform whisker tracing.  If these files
are not found when tracing is first initiated, they will be automatically
generated; computing their contents typically requires a few seconds.

The files contain the bank of filters used for processing images to extract the
backbone of whisker-like objects with sub-pixel precision.

The :file:`.detectorbank` files should be deleted before adjusting the
parameters in :file:`default.parameters`.
   
.. trace:

:program:`trace`
----------------

Traces all the whiskers.  The result is sent to <destination.whiskers>.

.. program:: trace

.. cmdoption:: <video>

   The path to the video file.  Can be in `StreamPix SEQ`, `TIFF`, or any
   format readable by FFMPEG_ such as `mpeg4`.

   .. _FFMPEG: http://www.ffmpeg.org

.. cmdoption:: <destination.whiskers>


  
