#!/usr/bin/env python
"""
Author: Nathan Clack
Date  : 2009-07-06

Copyright (c) 2009 HHMI. Free downloads and distribution are allowed for any
non-profit research and educational purposes as long as proper credit is given
to the author. All other rights reserved.
"""
import warnings
import trace
import numpy
from numpy import zeros
from numpy import polyval, polyfit, linspace, arange
from numpy import where, vstack

from scipy.ndimage import zoom

import pdb

def _thinking(args):
  print '.',
  return args

def analyze_decimation_error_types_single_frame( im, scale=0.5, xform = zoom ):
  ims  = xform(im,scale)
  shape = ims.shape
  px_scale = shape[0] / float( im.shape[0] )
  hits = lambda (x,sc): set( zip(*where(pixelize( trace.find_segments(x), sc, shape ))) )
  gold,test = map(hits, ((im ,px_scale   ), 
                         (ims,1.0        )) )
  false_positives = test - gold
  false_negatives = gold - test
  common = gold & test
  torate = lambda x: len(x)/float(len(gold)) 
  # print "false positive rate : %f"%(torate( false_positives ))
  # print "false negative rate : %f"%(torate( false_negatives ))
  # print "true  positive rate : %f"%(torate( common ))
  # print "sum                 : %f"%sum(map( torate, (false_positives, false_negatives, common) ) )
  return map(torate, (false_positives, false_negatives, common) )

def analyze_decimation_error_types( movie, frames = None, xform = zoom ):
  if frames is None:
    frames = range(len(movie))

  scale = linspace(0.05,1.0,20)
  errs = [ _thinking(
    vstack([
      analyze_decimation_error_types_single_frame(movie[ iframe ],s, xform=xform) 
      for s in scale 
    ]) )
    for iframe in frames
  ]

  return errs



def pixelize( wv, scale, shape, im=None ):
  """
  `shape`  should be a tuple describing the dimensions of the destination image.
           That is, it should be independant of `scale`.
  `scale`  is a factor that will be multiplied with the x and y coordinates from
           whisker segments in `wv` before rendering to the destination image.
  `wv`     should be an iterable collection of trace.Whisker_Seg.
  `im`     The destination image.  Default: None
           If None, a new one will be made.
           When a 
  """
  if im is None:
    im = zeros(shape)
  pnt2px   = lambda (x,y): (int(x*scale), int(y*scale))   # (x,y);scale --> px
  fit = lambda v: polyfit( arange(len(v)), v, 5 )
  #seg2pxs  = lambda e: map( pnt2px, zip(e.x,e.y))     # whisker segment --> pixels
  def seg2pxs(e):
    n = max( map( len, (e.x,e.y) ) )
    tt = linspace(0,n-1,2*(n-1))
    warnings.simplefilter( "ignore", numpy.RankWarning )
    px,py = map( fit, (e.x,e.y) )
    warnings.simplefilter( "default", numpy.RankWarning )
    return [pnt2px(p) for p in zip( polyval(px,tt), polyval(py,tt) )]

  for pixels in map( seg2pxs, wv ):
    for x,y in set(pixels):
      try:
        im[y,x] += 1
      except IndexError:
        pass
  return im

if __name__=='__main__':
  import unittest
  from ui.genetiff import Reader

  class Test_Pixelize( unittest.TestCase ):
    def setUp(self):
      self.movie   = Reader('data/curated/huber/whisker_data_0024.seq')
      self.gold_wv = trace.find_segments(self.movie[0])

    def test_SomePixelsHit(self):
      out = pixelize( self.gold_wv, 1.0, self.movie[0].shape )
      self.failUnless( out.any() )

    def test_PixelsOnlyHitOnce(self):
      out = pixelize( self.gold_wv, 1.0, self.movie[0].shape )
      self.failIf( (out>1).any() )
      
  unittest.main()

