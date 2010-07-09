.. docglossary:

********
Glossary
********

.. glossary::

  Follicle
    The follicle position is determined as the end-point of a traced segment closest to the face.

  Tip
    The end-point of a traced segment furthest from the face.

  Curvature
    Reciprocal of the radius of curvature.  See the MathWorld_ page for more information.

    .. _MathWorld: http://mathworld.wolfram.com/Curvature.html

  Current working directory
    For more information check 
    `here <http://en.wikipedia.org/wiki/Working_directory>`_.

  StreamPix SEQ
    A video format produced by StreamPix5_, a high speed digital recording
    program.

    .. _StreamPix5: http://www.norpix.com/products/streampix5/streampix5.php

  TIFF
    The `tagged image file format`_.  Often used for single images, the format also
    supports encoding a sequence of images.  We use our own custom TIFF
    library.  It is fully compliant with the `TIFF 6.0 spec`_ except that JPEG
    compression is not supported.

    Although our TIFF library supports multiple bit depths and pixel types, the
    whisker tracking software supports only 8-bit gray scale images and video.

    .. _`tagged image file format`: http://en.wikipedia.org/wiki/Tagged_Image_File_Format
    .. _`TIFF 6.0 spec`: http://partners.adobe.com/public/developer/en/tiff/TIFF6.pdf

  FFMPEG
    FFMPEG_ is a library for reading many different compressed video formats
    (e.g. `mp4` and `avi`).  Many common codecs are supported by this library.
    The specific set of supported codecs may vary between operating systems.

    .. _FFMPEG: http://www.ffmpeg.org
