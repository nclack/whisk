"""
Author: Nathan Clack <clackn@janelia.hhmi.org>
Date  : 

Copyright 2010 Howard Hughes Medical Institute.
All rights reserved.
Use is subject to Janelia Farm Research Campus Software Copyright 1.1
license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
"""
from ctypes import *
import os

_myers = CDLL('./libreader.so')

class FILE(Structure):
  pass

class Tiff_Reader(Structure):
  """ 
  Potential operations:
      Open( filename, endian, lsm )
      Rewind
      Advance
      EOF

      Copy
      Free
      Kill
      Reset
      @static Usage
  """                                           #typedef struct          
  _fields_ = [( "flip"      , c_int   ),        #  { int      flip;      
              ( "ifd_no"    , c_int   ),        #    int      ifd_no;    
              ( "lsm"       , c_int   ),        #    int      lsm;       
              ( "first_ifd" ,c_uint32 ),        #    uint32   first_ifd; 
              ( "ifd_offset",c_uint32 ),        #    uint32   ifd_offset;
              ( "file_size" ,c_int    ),        #    int      file_size; 
              ( "input"     ,POINTER( FILE ) ) ]#    FILE    *input;     
                                                #  } Treader;
  @staticmethod
  def usage():
    return _myers.Tiff_Reader_Usage()
  
_myers.Open_Tiff_Reader.restype = POINTER( Tiff_Reader );
_myers.Open_Tiff_Reader.argtypes = [
          POINTER( c_char ),
          POINTER( c_int  ),
          c_int ]

_myers.Rewind_Tiff_Reader.restype = None
_myers.Rewind_Tiff_Reader.argtypes = [ POINTER( Tiff_Reader ) ]

_myers.Advance_Tiff_Reader.restype = c_int
_myers.Advance_Tiff_Reader.argtypes = [ POINTER( Tiff_Reader ) ]

_myers.End_Of_Tiff.restype = c_int
_myers.End_Of_Tiff.argtypes = [ POINTER( Tiff_Reader ) ]

_myers.Free_Tiff_Reader.restype = None
_myers.Free_Tiff_Reader.argtypes = [ POINTER( Tiff_Reader ) ]

_myers.Kill_Tiff_Reader.restype = None
_myers.Kill_Tiff_Reader.argtypes = [ POINTER( Tiff_Reader ) ]

_myers.Reset_Tiff_Reader.restype = None
_myers.Reset_Tiff_Reader.argtypes= []

_myers.Tiff_Reader_Usage.restype = c_int
_myers.Tiff_Reader_Usage.argtypes= []

class Tiff_Writer(Structure):
  """
  Potential operations:
      Open( filename, lsm )
      Close 

      Copy
      Free
      Kill
      Reset
      @static Usage
  """                                           #typedef struct          
  _fields_ = [( "flip"      , c_int   ),        #  { int      flip;      
              ( "ifd_no"    , c_int   ),        #    int      ifd_no;    
              ( "lsm"       , c_int   ),        #    int      lsm;       
              ( "eof_offset",c_uint32 ),        #    uint32   eof_offset;
              ( "ifd_linko" ,c_uint32 ),        #    uint32   ifd_linko; 
              ( "ano_count" ,c_uint32 ),        #    uint32   ano_count; 
              ( "ano_linko" ,c_uint32 ),        #    uint32   ano_linko; 
              ( "annotation",POINTER( c_char )),#    char    *annotation;
              ( "output"    ,POINTER( FILE ) ) ]#    FILE    *output;    
                                                #  } Twriter;            
  @staticmethod
  def usage():
    return _myers.Tiff_Writer_Usage()

_myers.Open_Tiff_Writer.restype = POINTER( Tiff_Writer );
_myers.Open_Tiff_Writer.argtypes = [
          POINTER( c_char ),
          c_int ]

_myers.Close_Tiff_Writer.restype = None
_myers.Close_Tiff_Writer.argtypes = [ POINTER( Tiff_Writer ) ]

_myers.Free_Tiff_Writer.restype = None
_myers.Free_Tiff_Writer.argtypes = [ POINTER( Tiff_Writer ) ]

_myers.Kill_Tiff_Writer.restype = None
_myers.Kill_Tiff_Writer.argtypes = [ POINTER( Tiff_Writer ) ]

_myers.Reset_Tiff_Writer.restype = None
_myers.Reset_Tiff_Writer.argtypes= []

_myers.Tiff_Writer_Usage.restype = c_int
_myers.Tiff_Writer_Usage.argtypes= []

class Tif_Tag( Structure ):             #typedef struct     
  _fields_ = [( "label"   , c_uint16 ), #  { uint16  label; 
              ( "type"    , c_uint16 ), #    uint16  type;  
              ( "count"   , c_uint32 ), #    uint32  count; 
              ( "value"   , c_uint32 ) ]#    uint32  value; 
                                        #  } Tif_Tag;       

class Tiff_IFD(Structure):
  """
  Potential Operations
    Create( numtags )
    Read( Tiff_Reader )
    Write( Tiff_Writer )
    Print( stream )

    Get_Tag( label )
    Set_Tag( label )
    Delete_Tag( label )

    Copy
    Pack
    Free
    Kill
    Reset
    @static Usage
  """                                                #typedef struct        
  _fields = [ ( "data_flip"  , c_int ),              #  { int     data_flip;
              ( "numtags"    , c_int ),              #    int     numtags;  
              ( "initags"    , c_int ),              #    int     initags;  
              ( "maxtags"    , c_int ),              #    int     maxtags;  
              ( "tags"       , POINTER( Tif_Tag) ),  #    Tif_Tag *tags;    
              ( "vmax"       , c_int ),              #    int      vmax;    
              ( "veof"       , c_int ),              #    int      veof;    
              ( "vsize"      , c_int ),              #    int      vsize;   
              ( "values"     , POINTER( c_uint8 ) ), #    uint8   *values;  
              ( "dsize"      , c_int ),              #    int      dsize;   
              ( "data"       , POINTER( c_uint8 ) ) ]#    uint8   *data;    
                                                     #  } TIFD;
  @staticmethod
  def usage():
    return _myers.Tiff_Writer_Usage()

_myers.Create_Tiff_IFD.restype = POINTER( Tiff_IFD )
_myers.Create_Tiff_IFD.argtypes = [ c_int ]

_myers.Read_Tiff_IFD.restype = POINTER( Tiff_Reader )
_myers.Read_Tiff_IFD.argtypes = [ POINTER( Tiff_IFD ) ] 

_myers.Write_Tiff_IFD.restype = c_int
_myers.Write_Tiff_IFD.argtypes = [ 
          POINTER( Tiff_Writer ), 
          POINTER( Tiff_IFD    ) ] 

_myers.Print_Tiff_IFD.restype = None
_myers.Print_Tiff_IFD.argtypes = [ 
          POINTER( Tiff_IFD ),
          POINTER( FILE ) ]

_myers.Free_Tiff_IFD.restype = None
_myers.Free_Tiff_IFD.argtypes = [ POINTER( Tiff_IFD ) ]

_myers.Kill_Tiff_IFD.restype = None
_myers.Kill_Tiff_IFD.argtypes = [ POINTER( Tiff_IFD ) ]

_myers.Reset_Tiff_IFD.restype = None
_myers.Reset_Tiff_IFD.argtypes= []

_myers.Tiff_IFD_Usage.restype = c_int
_myers.Tiff_IFD_Usage.argtypes= []
