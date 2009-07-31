from ctypes import *
from ctypes.util import find_library
import numpy
from numpy import zeros
from reader import StackReader
import os,sys

#if sys.platform == 'win32':
#  libext = '.dll'
#else:
#  libext ='.so'

#dllpath = os.path.join(*os.path.split(__file__)[:-1])
#cReader = CDLL(os.path.join( dllpath,"libreader%s"%libext) )
os.environ['PATH']+=';.\\'
cReader = CDLL( find_library("whisk") )

cReader.Get_Stack_Dimensions_px.restype = c_int
cReader.Get_Stack_Dimensions_px.argtypes = [
  POINTER( c_char ), # filename
  POINTER( c_int ),  # width
  POINTER( c_int ),  # height
  POINTER( c_int ),  # depth
  POINTER( c_int )]  # kind

cReader.Read_Tiff_Stack_Into_Buffer.restype = c_int
cReader.Read_Tiff_Stack_Into_Buffer.argtypes = [
  POINTER( c_char ),   # filename
  POINTER( c_uint8 ) ] # buffer

_bpp = {  1   :   numpy.uint8,
          2   :   numpy.uint16,
          4   :   numpy.float32,
          8   :   numpy.float64 }

def readstack( filename ):
  width, height, depth, kind = c_int(0),c_int(0),c_int(0),c_int(0)
  if not os.path.exists(filename):
    raise IOError, "File not found. (%s)"%filename 
  cReader.Get_Stack_Dimensions_px( filename,
                                   byref(width),
                                   byref(height),
                                   byref(depth),
                                   byref(kind) )
  stack = zeros( (depth.value, height.value, width.value), dtype = _bpp[kind.value] )
  cReader.Read_Tiff_Stack_Into_Buffer( filename, 
                                        stack.ctypes.data_as( POINTER(c_uint8) ))
  return stack
  
class TiffReader( StackReader ):
  def __init__(self, filename):
    StackReader.__init__(self, readstack( filename ) )

