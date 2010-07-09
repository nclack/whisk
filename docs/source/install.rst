.. _installation:

*****************
Installation
*****************

There are two ways to install.  It's simplest to use a pre-built package. See
`Table 1`_ to check if a package is available for your system.  If there isn't
one, you'll need to install by building the source code.  The source code
builds and has been tested on Fedora 10, Windows XP, and OS X 10.5 and 10.6 on
both 32 and 64-bit machines.

.. _`Table 1`:




*Table 1*  Pre-built packages.

.. tabularcolumns: |C|C|C|C|C|

+-------+-----------+--------+-------+------+
|CPU    |OSX 10.5   |OSX 10.6|WinXP  |Win7  |
+=======+===========+========+=======+======+
|i386   | yes, but  | no     |yes    |yes   |
|       | no FFMPEG |        |       |      |
+-------+-----------+--------+-------+------+
|x86_64 | yes?      | yes    | 32bit | 32bit|
+-------+-----------+--------+-------+------+

Quick-start
===========

Install the commmand line tools.
--------------------------------

1. Download the pre-built package for your system.

2. Unzip it.

3. (optional) Add the :file:`{WhiskerTracking}/bin` directory to your environment's :envvar:`PATH`.


Install the Python environment.
-------------------------------

.. note::

  The graphical user interface is written in Python_, a popular cross-platform
  language.  Installation of a Python 2.6 environment, including several Python
  packages, is required to run the graphical user interface, but is not required
  for any automated video analysis.

Install the following in order:

#. `Python 2.6`_

#. `numpy 1.4.1`_

#. `scipy 0.8`_

#. `matplotlib 0.99.3`_

#. `pygame 1.9`_

#. `aggdraw 1.2a3`_

.. _`Python 2.6`: http://www.python.org/download/releases/2.6.5/
.. _`numpy 1.4.1`: http://sourceforge.net/projects/numpy/files/
.. _`scipy 0.8`: http://sourceforge.net/projects/scipy/files/
.. _`matplotlib 0.99.3`: http://matplotlib.sourceforge.net/
.. _`pygame 1.9`: http://www.pygame.org
.. _`aggdraw 1.2a3`: http://effbot.org/zone/aggdraw-index.htm

.. note::

  The recommended method for installing Python and Python libraries varies from
  system to system.  On Windows, use installers (`msi` or `exe` files).  On
  Linux, use your system's buily in package manager.  On OS X 10.5, use `dmg`
  based installers.  OS X 10.6 is a special case.  Many installers do not install
  `x86_64` compatible binaries. See 
  `Installing a 64-bit Python environment on Mac OS X 10.6`_
  for more.

.. warning::

   Installers exist for both Python and the required Python libraries on
   Windows and Mac OS X.  On Mac OS X **do not** install Python itself unless you
   know what you're doing.  Python comes pre-installed on OS X.

.. _Python: http://www.python.org

Install the `Matlab`_ environment.
----------------------------------

Optionally, add :file:`{WhiskerTracking}/matlab` to your Matlab `path`_.  This adds the following m-files:

* :file:`LoadWhiskers.m`
* :file:`LoadMeasurements.m`
* :file:`SaveMeasurements.m`

.. _Matlab: http://www.mathworks.com
.. _path: http://www.mathworks.com/access/helpdesk/help/techdoc/matlab_prog/f10-60956.html

.. todo::

   Add section on installing from source

.. _`Installing a 64-bit Python environment on Mac OS X 10.6`:

.. todo::

   Add section on Installing a 64-bit Python environment on Mac OS X 10.6
