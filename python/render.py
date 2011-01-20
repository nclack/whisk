"""
Author: Nathan Clack
Date  : 2009

Copyright (c) 2009 HHMI. Free downloads and distribution are allowed for any
non-profit research and educational purposes as long as proper credit is given
to the author. All other rights reserved.
"""
from pylab import *
from numpy import *

import pdb

def doone(im,wv,*args,**kwargs):
  ioff()
  imshow(im,cmap=cm.gray,hold=0)
  for e in wv:
    if e.y.mean() < 328:
      if abs( e.x[-1] - e.x[0] ) > 2:
        tt = linspace(0,1,len(e))
        px = polyfit(tt,e.x,5)
        py = polyfit(tt,e.y,5)
        plot( polyval(px,tt), polyval(py,tt),*args,**kwargs )
  axis('off')
  subplots_adjust(0,0,1,1,0,0)
  show()
  ion()

def doone_w_traj(movie,w,t,fid):
  def getcolor(tid):
    if tid is None:
      return (0.5,0.0,0.0,0.5)
    return cm.gist_rainbow(tid/float(len(t)),alpha=0.5) 
  def gettid(frm,seg):
    for i,traj in t.iteritems():
      if traj.get(frm) == seg:
        return i
    return None

  im = movie[fid]
  wv = w[fid]

  ioff()
  imshow(im,cmap=cm.gray,hold=0)
  for wid,e in wv.iteritems():
    tt = linspace(0,1,len(e))
    px = polyfit(tt,e.x,5)
    py = polyfit(tt,e.y,5)
    plot( polyval(px,tt), polyval(py,tt),
          color = 'r', #getcolor(gettid(fid,wid)),
          linewidth = 1,
          alpha = 1.0)
  axis('off')
  subplots_adjust(0,0,1,1,0,0)
  axis('image')
  show()
  ion()


def doall_w_traj(movie,wvd,t,frames):
  for i,fid in enumerate(frames):
    print fid
    doone_w_traj(movie,wvd,t,fid)
    savefig("render/seq_%04d.png"%i,dpi=96,facecolor = (0,0,0) )

def doall(movie,wvd,*args,**kwargs):
  for fid,wv in wvd.iteritems():
    print fid
    doone(movie[fid],wv.values(),'r')
    savefig("render/render_%04d.png"%fid,dpi=96,facecolor = (0,0,0) )
