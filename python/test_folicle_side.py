#!/usr/bin/env python
"""
Author: Nathan Clack
Date  : 2009

Copyright (c) 2009 HHMI. Free downloads and distribution are allowed for any
non-profit research and educational purposes as long as proper credit is given
to the author. All other rights reserved.
"""

def helper_face_point(shape, directive):
  helpers = {
   'top'    : lambda : (  shape[1]/2 ,  -shape[0]/2) ,
   'left'   : lambda : ( -shape[1]/2 ,   shape[0]/2) ,
   'bottom' : lambda : (  shape[1]/2 , 3*shape[0]/2) ,
   'right'  : lambda : (3*shape[1]/2 ,   shape[0]/2)
  }

  try:
    return helpers[directive]()
  except KeyError:
    print "Available directives"
    for k in helpers.iterkeys():
      print '\t',k
    raise Exception, 'Could not use supplied directive: %s'%directive


def make_side_function((cx,cy)):
  d2 = lambda e,side: (e.x[side]-cx)**2 + (e.y[side]-cy)**2
  s  = lambda e: 0 if d2(e,0)<d2(e,-1) else -1
  return s

def make_comparitor(point):
  from numpy import arctan2
  cx,cy = point
  side = make_side_function(point)
  angle = lambda e: arctan2(e.y[side(e)]-cy,e.x[side(e)]-cx)
  return lambda s,t: cmp( angle(s), angle(t) )

