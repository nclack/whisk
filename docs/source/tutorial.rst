.. automated-tracing-tutorial:

================
Tracing Tutorial
================

1. Start with a video with data similar to that in `Figure 1`_.  For this
   tutorial, the video is located in :file:`data/movie.mp4`.

   .. _`Figure 1`:

   .. figure:: images/tut-data-raw.png
     :alt: Video frame with raw data.
     :align: right
     :width: 2in

     *Figure 1* Raw data.

2. Trace the whiskers.  From the command line, run::

    trace data/movie.tif data/movie.whiskers

   This step requires the most processing time (a few minutes for the
   4,599 frame video in the example).
   If this is your first time running :program:`trace` it will take some
   extra time to generate two :file:`.detectorbank` files.

3. Generate measurements of the traced shapes.  These are used in later 
   steps to determine which traced objects belong to which whiskers.
   From the command line, run::

     measure --face left data/movie.whiskers data/movie.measurements

   In this video the face is on the left side of the frame.  Knowing where
   the face is tells :program:`measure` which side of traced objects
   should be considered the :term:`follicle`.

4. Usually, the tracing step identifies too many objects (e.g. facial hairs
   are often traced).  Use :program:`classify` to generate a first guess of
   which traced shapes are actually whiskers.  Here, the strategy is to be
   conservative.  It's ok to identify some whisker shapes as "not a whisker,"
   but identifying, for example, facial hairs as whiskers is more difficult to
   tolerate in later steps.

   Run :program:`classify` from the command line::

    classify data/movie.measurements data/movie.measurements left --px2mm 0.04 -n 3

   Here, the same file is used as the source and destination file.  Again, the
   face is specified to be on the left side of the image. A length threshold is
   used to descriminate between traced whisker segments and other traced
   shapes; the `px2mm` parameter provides the spatial scale.  Finally, we
   specify that we should expect 3 whiskers in the field throughout most of the
   video.  See :program:`classify` for more options.

5. Using the results from the previous step, identify which traced objects belong
   to which whiskers throughout the video.  From the command line, run::

    reclassify data/movie.measurements data/movie.measurements -n 3

   Again, the same file is used as input and output.

6. View the results.  You can load and plot results in `Matlab`_ or `Python`_ using
   the included interfaces.  You can also browse the results in the provided
   graphical user interface (see `Figure 2`_).  From the command line, run::

    python ui2.py data/movie.tif

   :program:`ui2.py` will guess the names of the :file:`.whiskers` and
   :file:`.measurements` file.  You can also explicitly add these files on the
   command line.
   
  .. _`Figure 2`:

  .. figure:: images/ui2-post-reclassify-screenshot.tiff
    :alt: Screenshot of ui2.py looking at the :program:`reclassify` result.
    :align: right
    :width: 4in

    *Figure 2* Screenshot of ui2.py looking at the :program:`reclassify` result.
    Identified whiskers are labelled red, green, and blue.  Light yellow traces
    are "non-whisker" traces.  The green ball identifies the follicle of the
    selected whisker.

.. _Python: http://www.python.org
.. _Matlab: http://www.mathworks.com
