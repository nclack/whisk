"""
Author: Nathan Clack
Date  : 2009

Copyright (c) 2009 HHMI. Free downloads and distribution are allowed for any
non-profit research and educational purposes as long as proper credit is given
to the author. All other rights reserved.
"""
from numpy import *
from numpy.linalg import eig

def load():
  from ui.whiskerdata import load_whiskers, load_trajectories
  from ui.genetiff import Reader
  movie = Reader('../../data/seq/whisker_data_0140.seq',adjuststipple=1)
  w,wid = load_whiskers('seq-heal.whiskers')
  t,tid = load_trajectories('seq-heal.trajectories')
  #w,wid = load_whiskers('whisk-vc/whisk-vc/seq.whiskers')
  #movie = Reader('../../data/JF8410_041808_001.tif',adjuststipple=1)
  #w,wid = load_whiskers('test.whiskers')
  #w,wid = load_whiskers('results/seq-hand.whiskers')
  #t,tid = load_trajectories('results/seq-hand.trajectories')
  return w,t,movie

def group(w,t):
  return [ [ w[fid][sid] for fid,sid in e.iteritems() ] for e in t.itervalues() ]

def label(g):
  def _iter():
    for i,v in enumerate(g):
      for e in v:
        e.label = i
        yield e
  return list(_iter())

def features( v ):
  def angle_beg( a ):
    end = max(len(a),10)
    dx = diff(a.x[:end])
    dy = diff(a.y[:end])
    a = arctan2(dx,dy)
    return a.mean()
  def angle_end( a ):
    beg = min(0, len(a)-10)
    dx = diff(a.x[beg:])
    dy = diff(a.y[beg:])
    a = arctan2(dx,dy)
    return a.mean()

  def _feat(a):
    dx = a.x[0]-a.x[-1]
    dy = a.y[0]-a.y[-1]
    return [ a.label, #a.x[0], a.y[0], a.x[-1], a.y[-1], 
             angle_beg(a),
             angle_end(a),
             hypot(dx,dy),
             arctan2(dy,dx) ]
  return array( [_feat(e) for e in v] )
            
def pcaplot(feat):
  from pylab import scatter, subplot
  eval,evec = eig(cov(feat[:,1:], rowvar=0)) #feat is [P X N+1] - first column is label
  idx = argsort(eval)
  px = evec[:,idx[-1]] #largest two vecs # each is [N X 1]
  py = evec[:,idx[-2]]
  pz = evec[:,idx[-3]]
  #subplot(221)
  scatter( dot(px,feat[:,1:].T), dot(py,feat[:,1:].T), 0.5, feat[:,0], faceted=False, hold=0 )
  #subplot(222)
  #scatter( dot(pz,feat[:,1:].T), dot(py,feat[:,1:].T), 0.5, feat[:,0], faceted=False, hold=0 )
  #subplot(223)
  #scatter( dot(px,feat[:,1:].T), dot(pz,feat[:,1:].T), 0.5, feat[:,0], faceted=False, hold=0 )
  return px,py,pz

###
# Plots
#
def plot_left_sides(w,t):
  pass
