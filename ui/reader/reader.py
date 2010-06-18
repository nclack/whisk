"""
This module defines an abstraction for interfacing with "movie-like" objects.

The main point of the interface is to emulate the behavior of Image objects
from the `Python Image Library` (PIL).  For historical reasons this 
interface is important to the whisk/ui/ui2.py program, which is used to proof 
and edit whisker tracking results.

An example implementation of the interface can be found here in the 
`StackReader` class.  This class takes a 3d numpy vector (a `stack`), and
wraps the `IMovieReader` interface around it.

See `tiff.py` for a practical example of how the `StackReader` can be
subclassed, or see `seq.py` for another example implementation of the
`IMovieReader` interface.

---
Author: Nathan Clack <clackn@janelia.hhmi.org>
Date  : 

Copyright 2010 Howard Hughes Medical Institute.
All rights reserved.
Use is subject to Janelia Farm Research Campus Software Copyright 1.1
license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
"""
import pdb
from numpy import floor

class IMovieReader(object):
  """
  The main point of this interface is to emulate the behavior of Image objects
  from the `Python Image Library` (PIL).  For historical reasons this 
  interface is important to the whisk/ui/ui2.py program, which is used to proof 
  and edit whisker tracking results.

  Subclasses can minimally override:

    __getitem__
    __len__
    __del__ (optional)

  Subclasses should minimally define the following properties:

    self.size: 2-tuple 
      describes the image dimensions in pixels

  """
  def __init__(self):
    object.__init__(self)
    self.current_frame_id = 0
  def rewind(self):
    self.current_frame_id = 0
    return self
  def next(self):
    if self.current_frame_id >= len(self):
      raise StopIteration
    self.current_frame_id += 1
    return self[self.current_frame_id]
  def skip(self):
    self.current_frame_id += 1
    if self.current_frame_id >= len(self):
      raise EOFError
    return self
  def seek(self,idx):
    if idx >= len(self):
      raise EOFError
    self.current_frame_id = idx
    return self
  def tell(self):
    return self.current_frame_id
  def close(self):
    self.__del__()
  def __array__(self):
    return self[self.current_frame_id]

  def __getitem__(self,idx):
    pass
  def __len__(self):
    pass
  def __del__(self):
    pass

class StackReader( IMovieReader ):
  """
  This class takes a 3d numpy vector (a `stack`), and wraps the `IMovieReader` interface around it.

  See `tiff.py` for a practical example of how the `StackReader` can be subclassed.
  """
  def __init__( self, data   ):
    IMovieReader.__init__(self)
    self._data   = data  
    self.size = data[0].shape
  def __getitem__(self, idx):
    return self._data[idx]
  def __len__(self):
    return self._data.shape[0]

class TransposedReader( IMovieReader ):
  def __init__(self, reader):
    IMovieReader.__init__(self)
    self._reader = reader
    self.size = tuple(reversed( reader.size ))
  def __getitem__(self, idx):
    return self._reader[idx].T
  def __len__(self):
    return len( self._reader )
  
class AdjustStippledGainReader( IMovieReader ):
  def __init__(self, reader):
    IMovieReader.__init__(self)
    self._reader = reader
    self.size = reader.size
    im = reader[0].astype(float)
    mask = im > im.mean()
    
    evener = lambda x: x - x%2 #makes the dimensions even by truncating the last line if necessary
    xd,yd = map(evener, im.shape)
    hratio = im[:xd:2]/im[1:xd:2]                     # compute ratio of odd and even lines
    hmask  = mask[:xd:2]                              # ...consider only those pixels above the mean
    hgain  = hratio[hmask].mean()                     # compute the correction factor
    hstat  = abs( (hgain-1.0) / hratio[hmask].std() ) # compute a significance statistic
   
    vratio = im[:,:yd:2]/im[:,1:yd:2]
    vmask  = mask[:,:yd:2]
    vgain  = vratio[vmask].mean()
    vstat  = abs( (vgain-1.0) / vratio[vmask].std() ) 
         
    #pdb.set_trace()
    if hstat > vstat:
      self._direction = 'horizontal'
      self._gain = hgain
      self._stat = hstat
    else:
      self._direction = 'vertical'
      self._gain = vgain
      self._stat = vstat

  def __getitem__(self, idx): 
    im = self._reader[idx].copy()
    saturation = floor(255/self._gain)
    mask = im > saturation;
    #pdb.set_trace()
    if self._direction == 'horizontal':          
      im[1::2] *= self._gain
      mask[::2] = 0
      im[mask] = 255
    else:
      im[:,1::2] *= self._gain
      mask[:,::2] = 0
      im[mask] = 255
    return im

  def __len__(self):
    return len( self._reader )

