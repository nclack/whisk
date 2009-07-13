#!/usr/bin/env python
"""
Performs an initial classification of whisker segments.

It is an `initial` classification because:
  
  1. Whisker segments are classified in a subset of frames.
  
  2. The results of this step are meant to be used to initialize other
  classification/tracking approaches.

Author: Nathan Clack
Date: 2009-05-26
Copyright (c) 2009 HHMI. Free downloads and distribution are allowed as long as proper
credit is given to the author.  All other rights reserved.
"""
import optparse
import os
import warnings

warnings.simplefilter("ignore",UserWarning)
import trace
import summary
import traj
from ui.whiskerdata import save_trajectories
warnings.simplefilter("default",UserWarning)

if __name__ == '__main__':
  parser = optparse.OptionParser( usage = "Usage: %prog source destination" )
  options,args = parser.parse_args()
  if len(args) != 2:
    parser.error( "Expected source and destination file names but recieved %d arguments."%len(args) ) 
  src, dst = args

  if not os.path.exists(os.path.split(dst)[0]): # check path, do this before doing long calculation
    raise IOError, "Could not find destination file %s"%dst

  #
  # Get data from source
  #
  ext = os.path.splitext(src)[1]
  if ext == '.whiskers':
    w = trace.Load_Whiskers(src)
    data = array(list( summary.features(w) ))
  elif ext == '.measurements':
    data = traj.MeasurementsTable(src).asarray()
  else:
    raise IOError, "Source file extension not recognized.  Got: %s"%ext

  traj = summary._simpletraj(data)

  save_trajectories(dst, traj)

