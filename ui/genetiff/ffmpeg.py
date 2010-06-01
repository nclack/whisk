from ctypes import *
from ctypes.util import find_library
import numpy
from numpy import zeros
from reader import StackReader
import os,sys

os.environ['PATH']+=';.\\'
cReader = CDLL( find_library("whisk") )

cReader.FFMPEG_Get_Stack_Dimensions.restype = c_int
cReader.FFMPEG_Get_Stack_Dimensions.argtypes = [
  POINTER( c_char ), # filename
  POINTER( c_int ),  # width
  POINTER( c_int ),  # height
  POINTER( c_int ),  # depth
  POINTER( c_int )]  # kind

cReader.FFMPEG_Read_Stack_Into_Buffer.restype = c_int
cReader.FFMPEG_Read_Stack_Into_Buffer.argtypes = [
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
  if(not cReader.FFMPEG_Get_Stack_Dimensions( filename,
                                              byref(width),
                                              byref(height),
                                              byref(depth),
                                              byref(kind) )):
    raise IOError, "Couldn't read dimensions for %s"%filename
  stack = zeros( (depth.value, height.value, width.value), dtype = _bpp[kind.value] )
  cReader.FFMPEG_Read_Stack_Into_Buffer( filename, 
                                         stack.ctypes.data_as( POINTER(c_uint8) ))
  return stack
  
class FFMPEGReader( StackReader ):
  def __init__(self, filename):
    StackReader.__init__(self, readstack( filename ) )


