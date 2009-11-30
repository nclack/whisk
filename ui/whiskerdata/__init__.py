""" whisker_data

This module provides tools for reading and writing whisker segment and whisker
trajectory related data.

This uses a scheduler to handle non-blocking saves.  This behavior is the
default...and neccesitates one use the close() method to shut down the thead
before the program exits.

Nathan Clack clackn@janelia.hhmi.org
Howard Hughes Medical Institute, JFRC
(c) 2008 All rights reserved
"""

import scheduler
from scheduler import LastOnlyScheduler
import trace
import traj
from trace import Save_Whiskers
from trace import Load_Whiskers 
from copy import deepcopy
import os,sys

from numpy import array,zeros

import pdb

_g_autosave_scheduler = None
#_g_autosave_scheduler.start()

def start_autosave():
  global _g_autosave_scheduler
  _g_autosave_scheduler = LastOnlyScheduler()
  _g_autosave_scheduler.start()

def close():
  global _g_autosave_scheduler
  if _g_autosave_scheduler:
    _g_autosave_scheduler.quit()
    _g_autosave_scheduler = None

def copy_whiskers( whiskers ):
  newthing = {}
  for s,t in whiskers.iteritems():
    newthing[s] = {}
    for u,v in t.iteritems():
      newthing[s][u] = v.copy()
  return newthing

def copy_trajectories( trajectories ):
  newthing = {}
  for s,t in trajectories.iteritems():
    newthing[s] = {}
    for u,v in t.iteritems():
      newthing[s][u] = v
  return newthing

def __save_state(precursor_or_names, *args):
  savers = {'.whiskers':     save_whiskers,
            '.trajectories': save_trajectories,
            '.measurements': save_measurements }
  names = dict([ (k,None) for k in savers.iterkeys() ])

  #
  # Construct output filenames
  #
  if not isinstance(precursor_or_names, str ) and hasattr( precursor_or_names, '__iter__' ):
    #multiple names...try to find names for the different filetypes, otherwise guess
    for fn,(p,e) in zip( precursor_or_names, map( os.path.splitext, precursor_or_names ) ):
      names[e] = fn #build map of extentions --> filenames
    precursor = p   #use the last filename for the precursor
    for e,fn in names.iteritems():
      if fn is None:               # if the extension has no corresponding filename
        names[e] = precursor +  e  #    then construct one using the precursor

  elif isinstance(precursor_or_names,str): 
    #single valued, assume it's a filename or precursor
    precursor = os.path.splitext(precursor_or_names)[0]
    
    for e in names.iterkeys():
      names[e] = precursor + e
  else:
    raise TypeError(" precursor_or_names must be string or iterable ")

  # Measurements and Trajectories files are somewhat redundant.
  # Save to the file that identities were loaded from.
  ext = map( lambda n: os.path.splitext(n)[-1], precursor_or_names )
  if '.trajectories' in ext:
    del names['.measurements']
  elif '.measurements' in ext:
    del names['.trajectories']
  else:  #by default save only to the .measurements file
    del names['.trajectories']

  #
  # Call the individual save functions
  #
  do_nothing = lambda *args: None  #this gets called for unrecognized extensions
  for extension,filename in names.iteritems():
    print 'Saving ',extension,'...',
    sys.stdout.flush()
    savers.get(extension,do_nothing)(filename,*args) 
    print 'Done'

def save_state( precursor_or_names, whiskers, trajectories, facehint ):
  print "save state: "+str(precursor_or_names)
  __save_state(precursor_or_names,whiskers,trajectories, facehint)

def load_state( precursor_or_names ):
  order = ['.whiskers', '.measurements', '.bar' ]
  alias = { '.whiskers'     : '.whiskers',
            '.trajectories' : '.trajectories',
            '.measurements' : '.measurements',
            '.bar'          : '.bar' }
  loaders = {'.whiskers':     load_whiskers,
             '.trajectories': load_trajectories,
             '.bar':          load_bar_centers,
             '.measurements': load_measurements } 
  names = dict([ (alias[k],None) for k in order ])
  
  if not isinstance(precursor_or_names, str ) and hasattr( precursor_or_names, '__iter__' ):
    #multiple names...try to find names for the different filetypes, otherwise guess
    
    precursor_or_names = filter( lambda name: (os.path.splitext(name)[1] in loaders.keys()), precursor_or_names )
    for fn,(p,e) in zip( precursor_or_names, map( os.path.splitext, precursor_or_names ) ):
      names[alias[e]] = fn
    precursor = p
    for e,fn in names.iteritems(): #try to guess
      if fn is None:
        names[alias[e]] = precursor +  e
  elif isinstance(precursor_or_names,str): #single valued, assume it's a filename or precursor
    precursor = os.path.splitext(precursor_or_names)[0]
    for k,v in names.iteritems():
      names[k] = precursor + k
  else:
    raise TypeError(" precursor_or_names must be string or iterable ")
  
  def logload(t,v):
    print "Loaded: ",t
    return v
  print 'Loading...'
  objs = [logload(t,loaders[os.path.splitext(names[t])[1]](names[t])[0]) for t in order]
  objs.append(0) #append a legit trajectory id....assuming 0...this should probably get taken out later
  return objs

def load_measurements( name ):
  try:
    return [traj.MeasurementsTable(name).get_trajectories(),0]
  except IOError:
    print "Couldn't open file. Creating: ", name
    return {},0

def save_measurements( name, *args ):
  whiskers, trajectories, facehint = args[0:3]
  traj.MeasurementsTable( {'whiskers':whiskers, 'facehint':facehint} ).commit_trajectories(trajectories).save(name)
   
def load_whiskers( filename ):
  try:
    w = Load_Whiskers( filename )
    wid = 0;
    if len(w):
      wid = w.values()[0].keys()[0]
    return w,wid      
  except IOError:
    print "Couldn't open file. Creating: ", filename
    return {},0

def save_whiskers( filename, *args ):
  Save_Whiskers(filename, args[0] )

def save_trajectories( filename, *args ):
  f = open( filename, 'w' )
  trajectories = args[1]
  for k,v in trajectories.iteritems():
    for s,t in v.iteritems():
      print >> f, '%d,%d,%d'%(k,s,t)

def load_trajectories( filename ):
  trajectories = {}
  try:
    f = open( filename, 'r' )

    cur = 0;
    for line in f:
      t = [int(x) for x in line.split(',')[:3]]
      if not t[0] in trajectories:
        trajectories[t[0]] = {}
      trajectories[ t[0] ][ t[1] ] = t[2];

    return trajectories, trajectories.keys()[0]
  except:
    print "Couldn't open file. Creating: ", filename
    return {},0

def load_bar_centers( filename ):
  centers = {}
  try:
    f = open( filename, 'r' )
    adapters = [ lambda s: int(s),
                 lambda s: float(s),
                 lambda s: float(s) ]
    data = [[f(x) for f,x in zip(adapters, line.split(' '))] for line in f]
    for fid, x, y in data:
      centers[fid] = (x,y)
  except:
    pass
  return centers,0
