.. specialfiles:

=============
Special files
=============

These files should be present in the :term:`current working directory` when
using the :doc:`command-line-tools` or the :doc:`graphical-user-interface`.

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

These files are automatically generated on tracing if found missing.  Computing
their contents typically requires a few seconds.

The files contain the bank of filters used for processing images to extract the
backbone of whisker-like objects with sub-pixel precision.

The :file:`.detectorbank` files should be deleted before adjusting the
parameters in :file:`default.parameters`.
