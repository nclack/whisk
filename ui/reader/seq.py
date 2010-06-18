"""
Author: Nathan Clack <clackn@janelia.hhmi.org>
Date  : 

Copyright 2010 Howard Hughes Medical Institute.
All rights reserved.
Use is subject to Janelia Farm Research Campus Software Copyright 1.1
license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
"""
from ctypes import *
from ctypes.util import find_library
import numpy
from numpy import zeros
from reader import IMovieReader
import os,sys

if sys.platform == 'win32':
  lib = 'whisk.dll'
else:
  lib ='libwhisk.dylib'

#dllpath = os.path.join(*os.path.split(__file__)[:-1])
#cReader = CDLL(os.path.join( dllpath,lib) )
os.environ['PATH']+=';.\\'
cReader = CDLL( find_library("whisk") )

_bpp = { 8: numpy.uint8,
        16: numpy.uint16,
        32: numpy.float32,
        64: numpy.float64 }

class FILE(Structure):
  pass

class SeqFile(Structure):                             # typedef struct                     
  _fields_ = [( "width"         , c_uint    ),        # { unsigned int     width;       
              ( "height"        , c_uint    ),        #   unsigned int     height;      
              ( "bitdepth"      , c_uint    ),        #   unsigned int     bitdepth;    
              ( "bitdepthreal"  , c_uint    ),        #   unsigned int     bitdepthreal;
              ( "sizebytes"     , c_uint    ),        #   unsigned int     sizebytes;   
              ( "fmt"           , c_uint    ),        #   unsigned int     fmt;         
              ( "nframes"       , c_uint    ),        #   unsigned int     nframes;     
              ( "truesize"      , c_uint    ),        #   unsigned int     truesize;    
              ( "framerate"     , c_double  ),        #   double  framerate;            
              ( "starttime"     , c_double  ),        #   double  starttime;            
              ( "fp"            , POINTER( FILE ))];  #   FILE   *fp;                   
                                                      # } SeqReader;
cReader.Seq_Open.restype  = POINTER( SeqFile )
cReader.Seq_Open.argtypes = [ POINTER( c_char ) ]

cReader.Seq_Close.restype = POINTER( SeqFile )
cReader.Seq_Close.argtypes = [ POINTER( SeqFile ) ]

cReader.Seq_Read_Image_To_Buffer.restype = c_int
cReader.Seq_Read_Image_To_Buffer.argtypes= [
  POINTER( SeqFile ),
  c_int,
  POINTER( c_uint8 ) ]

cReader.Seq_Read_Images_To_Buffer.restype = c_int
cReader.Seq_Read_Images_To_Buffer.argtypes= [
  POINTER( SeqFile ),
  c_int,                 #start
  c_int,                 #stop
  c_int,                 #step
  POINTER( c_uint8 ) ]

cReader.Seq_Read_Stack_To_Buffer.restype = c_int
cReader.Seq_Read_Stack_To_Buffer.argtypes= [
  POINTER( SeqFile ),
  POINTER( c_uint8 ) ]

cReader.Seq_Time_Stamp.restype = c_double
cReader.Seq_Time_Stamp.argtypes = [
  POINTER( SeqFile ),
  c_int ]

cReader.Seq_Time_From_Start.restype = c_double
cReader.Seq_Time_From_Start.argtypes = [
  POINTER( SeqFile ),
  c_int ]

def readstack( filename ):
  ps = cReader.Seq_Open( filename )
  s = ps.contents
  shape = s.nframes, s.height, s.width
  kind = _bpp[ s.bitdepthreal ]
  stack = zeros( shape, dtype = kind )
  cReader.Seq_Read_Stack_To_Buffer( ps, stack.ctypes.data_as( POINTER( c_uint8 ) ))
  cReader.Seq_Close(ps)
  return stack

class SeqReader(IMovieReader):
  def __init__(self, filename):
    IMovieReader.__init__( self )
    self._reader = cReader.Seq_Open( filename )
    self.size = self._reader.contents.height, self._reader.contents.width
    self.buffer = zeros( self.size, dtype = _bpp[ self._reader.contents.bitdepthreal ] )
    self._creader = cReader

  def __getitem__(self,idx):
    if isinstance(idx, slice):
      shape = [len( range(len(self))[idx] ) ]
      shape.extend(self.size)
      indices = idx.indices( len(self) )
      buffer = zeros( shape, dtype = _bpp[ self._reader.contents.bitdepthreal ] )
      
      cReader.Seq_Read_Images_To_Buffer( self._reader, indices[0], indices[1], indices[2], 
                                         buffer.ctypes.data_as( POINTER( c_uint8 ) ))   
      return buffer
    else:
      if idx <= -len(self) or idx >= len(self):
        raise IndexError(idx)
      cReader.Seq_Read_Image_To_Buffer( self._reader, 
                                        idx%len(self),
                                        self.buffer.ctypes.data_as( POINTER( c_uint8 ) ))
      return self.buffer
  def __len__(self):
    return self._reader.contents.nframes
  def __del__(self):
    self._creader.Seq_Close( self._reader )
