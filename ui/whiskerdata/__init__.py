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
from trace import Save_Whiskers as save_whiskers
from trace import Load_Whiskers 
from copy import deepcopy

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

def __save_state(precursor, whiskers ,trajectories):
  if whiskers:
    save_whiskers( precursor+'.whiskers', whiskers )
  if trajectories:
    save_trajectories( precursor+'.trajectories', trajectories )
  if _g_autosave_scheduler:
    scheduler.time.sleep(1.0) # this forces a pause between successive autosaves

def save_state( precursor, whiskers, trajectories ):
  global _g_autosave_scheduler
  print "save state: "+precursor
  if _g_autosave_scheduler:
    w = copy_whiskers(whiskers)
    t = copy_trajectories(trajectories)
    _g_autosave_scheduler.pushjob( lambda: __save_state(precursor, w, t) )
  else:
    __save_state(precursor,whiskers,trajectories)

def load_state( precursor ):
  whiskers,segid = load_whiskers( precursor+'.whiskers' )
  trajectories,tid = load_trajectories( precursor+'.trajectories' )
  centers = load_bar_centers( precursor + '.bar' )
  return whiskers, trajectories, centers, tid
 
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

def save_trajectories( filename, trajectories ):
  f = open( filename, 'w' )
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
  return centers




