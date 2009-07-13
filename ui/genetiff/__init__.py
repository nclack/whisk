import warnings
warnings.simplefilter( "ignore", UserWarning )
from seq import SeqReader
from tiff import TiffReader
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
  if ext == '.seq':
    ret = SeqReader( filename )
  elif ext == '.tiff' or ext == '.tif':
    ret = TiffReader( filename )
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
