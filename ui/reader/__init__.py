"""
Author: Nathan Clack <clackn@janelia.hhmi.org>
Date  : 

Copyright 2010 Howard Hughes Medical Institute.
All rights reserved.
Use is subject to Janelia Farm Research Campus Software Copyright 1.1
license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
"""
import warnings
warnings.simplefilter( "ignore", UserWarning )
from seq import SeqReader
from tiff import TiffReader
from ffmpeg import FFMPEGReader
from reader import TransposedReader, AdjustStippledGainReader
import os
warnings.simplefilter( "default", UserWarning )


def Reader(filename, transpose = False, adjuststipple = False):
  """
  Factory function for generating `IMovieInterface` objects.
  
  Uses the filename's extension to determine the filetype.
  """
  ret = None
  ext = os.path.splitext( filename )[-1]
  def is_ffmpeg_ext(y)
    std = [ '.mp4', '.mov', '.avi', '.mpg',  '.mp3' ]
    test = lambda x: y==x
    return any(map(test,std))
  if ext == '.seq':
    ret = SeqReader( filename )
  elif ext == '.tiff' or ext == '.tif' or ext=='.lsm':
    ret = TiffReader( filename )
  elif is_ffmpeg_ext(ext):
    ret = FFMPEGReader( filename )
  else:
    raise Exception,"File type (%s) not supported."%ext
  if adjuststipple:
    ret = AdjustStippledGainReader( ret )
  if transpose:
    ret = TransposedReader( ret ) 
  return ret

# TODO
# class Writer:
#   """ Writer
#
#   Manages writing to a Tiff file.
#
#   Not all output types seem to work reliably
#
#   >>> import numpy
#   >>> a = ((2**16-1)*numpy.eye(100)).astype('uint16')
#   >>> writer = Writer('test.tif')
#   >>> writer.push(a);
#   >>> writer.close()
#   """
