#!/usr/bin/env python
"""
Generate or update table of whisker segment based measurements.

Interesting measures of whisker shape such as angle and curvature are computed
and stored in a table for later use.  The table also stores the unique id of
each segment (corresponding to the source .whiskers file) and a label
classifying each segment (e.g. to identify to which it belongs).

The particular behavior (whether to try to update a table or build a new one)
is based on the filetypes of the input sources.  The filetypes are determined
by the file extensions.  Valid extenstions are:

  .whiskers
  .measurements
  .trajectories


Author: Nathan Clack
Date: 2009-05-26
Copyright (c) 2009 HHMI. Free downloads and distribution are allowed as long as
proper credit is given to the author.  All other rights reserved.
"""
import warnings
warnings.simplefilter("ignore",UserWarning)
import optparse
import os
import numpy
import trace
import traj
import summary
from ui.whiskerdata import load_trajectories
warnings.simplefilter("default",UserWarning)

def ext_cmp(*args):
  return reduce( cmp, map( lambda x: os.path.splitext(x)[-1], args ) )

def _fmt(s):
  import re
  return re.sub('\s+',' ',s)

if __name__ == '__main__':
  parser = optparse.OptionParser( usage = "Usage: %prog source[s] dest [options]",
                                  description = _fmt(
                                    "`sources` should include a whiskers file or   \
                                    a measurements file.   Optionally, a           \
                                    trajectories file may be  included."
                                  ) 
                                )
  options,args = parser.parse_args()
  dst = args.pop()
  args.sort( cmp = ext_cmp ) #sort sources by extension (expect .measurements to sort before .trajetories)

  # check existence of files and output directory
  if not len(args) in [1,2]:
    parser.error( "Expected 1 or 2 source files as arguments but recieved %d."%len(args) )
  if not os.path.exists(os.path.split(dst)[0]): 
    raise IOError, "Could not find destination file %s"%dst
  if not all( map( os.path.exists, args )):
    raise IOError, "Could not find one or more input source files."


  if all(map( lambda f: os.path.splitext(f)[-1] in ['.trajectories','.whiskers'], args )):
    if len(args)==1:
      src = args[0]
      tfile = None
    else:
      src = args[1]   #know the order b.c. already sorted by file extenstion
      tfile = args[0]
    w = trace.Load_Whiskers( src )
    data = numpy.array(list( summary.features(w) ))
    if tfile:
      t,tid = load_trajectories( tfile )
      summary.commit_traj_to_data_table( t, data ) 
    
  elif all(map( lambda f: os.path.splitext(f)[-1] in ['.trajectories','.measurements'], args )):  
    if len(args) != 2:
      parser.error( "Expected 2 source filenames. Got %d."%len(args) ) 
    msrc, tsrc = args
    assert os.path.splitext(msrc)[-1] == '.measurements'
    assert os.path.splitext(tsrc)[-1] == '.trajectories'

    t,tid = load_trajectories( tsrc )
    data = traj.MeasurementsTable( msrc ).asarray()
    summary.commit_traj_to_data_table( t, data ) 
  else:
    raise parser.error( "An unexpected combination of source file types was provided." )

  table = traj.MeasurementsTable(data)
  table.update_velocities().save(dst)
