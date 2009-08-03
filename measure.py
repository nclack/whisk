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
                                    "`sources` should include a whiskers file or         \
                                    a measurements file.   Optionally, a                 \
                                    trajectories file may be  included.                  \
                                    `dest` is the output filename.  By default it is     \
                                    written as a `MeasurementsTable` file.  However,     \
                                    if the extenstion is `.mat` it will be written as a  \
                                    Matlab data file.                                    \
                                                                                         \
                                    If a `whiskers` file is used,                        \
                                    Some hint must be provided to determine which        \
                                    side of a whisker segment is the follicle.  If       \
                                    the face is on the side of the image, then           \
                                    specify which side (left, right, top, bottom).       \
                                    Alternatively, provide the center of the head as     \
                                    an (x,y) pair.  See the 'face' option.               \
                                    Currently, this is set up for experiments where the  \
                                    mouse is head-fixed.                                 \
                                    "
                                  ) 
                                )
  parser.add_option("--face",
                    help    = "A hint describing where the face is.  Can be a side of the image (left,right,top,bottom) or an x,y pair, e.g. --face=40,30 [default: %default].",
                    default = "left", 
                    action  = "store",
                    type    = "string",
                    dest    = "face")
  options,args = parser.parse_args()
  dst = args.pop()
  args.sort( cmp = ext_cmp ) #sort sources by extension (expect .measurements to sort before .trajetories)

  # check existence of files and output directory
  if not len(args) in [1,2]:
    parser.error( "Expected 1 or 2 source files as arguments but recieved %d."%len(args) )
  if not os.path.exists(os.path.split(dst)[0]): 
    if os.path.split(dst)[0]!='':
      raise IOError, "Could not find destination path %s"%os.path.split(dst)[0]
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
    data = numpy.array(list( summary.features(w, options.face) ))
    if tfile:
      t,tid = load_trajectories( tfile )
      summary.commit_traj_to_data_table( t, data ) 
    
  elif all(map( lambda f: os.path.splitext(f)[-1] in ['.trajectories','.measurements'], args )):  
    sources = dict( map( lambda f: (os.path.splitext(f)[-1], f), args ) )

    
    try:
      data = traj.MeasurementsTable( sources['.measurements'] ).asarray()
    except KeyError:
      raise UserException, "A .measurements file must be provided as one of the source files."

    try:
      t,tid = load_trajectories( sources['.trajectories'] )
      summary.commit_traj_to_data_table( t, data ) 
    except KeyError:
      pass
  else:
    raise parser.error( "An unexpected combination of source file types was provided." )

  table = traj.MeasurementsTable(data).update_velocities()
  if os.path.splitext(dst)[-1] == '.mat':
    table.save_to_matlab_file( dst )
  else:
    table.save(dst)
