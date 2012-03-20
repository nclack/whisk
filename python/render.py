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
  #ioff()
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
  #show()
  #ion()

def doone_w_traj(movie,w,t,fid,fps):
  def getstyle(tid):
    if tid is None:
      return {
          'color'    : (0.3,0.3,0.1,0.0),
          'linewidth': 0.5,
          'linestyle': '-',
          'alpha'    : 0.0}
    else:
      return {
          'color'    : cm.gist_rainbow(tid/float(len(t)),alpha=1.0),
          'linewidth': 3.0,
          'linestyle': '--',
          'alpha'    : 1.0}

  def gettid(frm,seg):
    for i,traj in t.iteritems():
      if traj.get(frm) == seg:
        return i
    return None

  im = movie[fid]
  wv = w[fid]

  #ioff()
  imshow(im,cmap=cm.gray,hold=0)
  for wid,e in wv.iteritems():
   #tt = linspace(0,1,len(e))
   #px = polyfit(tt,e.x,5)
   #py = polyfit(tt,e.y,5)
   #plot( polyval(px,tt), polyval(py,tt),
   #      **getstyle(gettid(fid,wid)))
    plot( e.x,e.y,
          **getstyle(gettid(fid,wid)))
  text(80,20,"% 5d ms"%int(1000.0*fid/double(fps)),
      family  ='sans-serif',
      fontsize='large',
      weight='bold',
      color='w',
      horizontalalignment='right')
  axis('off')
  subplots_adjust(0,0,1,1,0,0)
  axis('image')
  #show()
  #ion()


def doall_w_traj(movie,wvd,t,frames,dest="render/seq_%04d.png",fps=1000):
  for i,fid in enumerate(frames):
    print fid
    doone_w_traj(movie,wvd,t,fid,fps)
    savefig(dest%fid,dpi=96,facecolor = (0,0,0),bbox_inches="tight",pad_inches=0 )

def doall(movie,wvd,dest="render/render_%04d.png"):
  for fid,wv in wvd.iteritems():
    print fid
    doone(movie[fid],wv.values(),'r')
    savefig(dest%fid,dpi=96,facecolor = (0,0,0),bbox_inches='tight')

if __name__=='__main__':
  import sys
  import argparse
  parser = argparse.ArgumentParser(description="Render tracking results.")
  parser.add_argument('vname_',metavar='video',help='Input video name.  Whiskers and measurements data files should have corresponding names.')
  parser.add_argument('-minframe',type=int,default=0,help='First frame to render.')
  parser.add_argument('-maxframe',type=int,default=-1,help='Last frame to render.')
  parser.add_argument('dest_',nargs='?',metavar='destination',default='render/%05d.png',help="Specify where rendered images should be saved.")
  parser.add_argument('-fps',type=int,default=1000,help="Frame rate used for labelling images with a timestamp.")
  N = parser.parse_args()
  def fnames(vidname):
    import os
    core = os.path.splitext(vidname)[0]
    return vidname,core+'.whiskers',core+'.measurements'
  def load(vidname):
    import reader,trace,traj
    vname,wname,mname = fnames(vidname)
    v = reader.Reader(vname,adjuststipple=0)
    w = trace.Load_Whiskers(wname)
    m = traj.MeasurementsTable(mname)
    return v,w,m
  v,w,m = load(N.vname_)
  if N.maxframe<0:
    N.maxframe=len(v)-1
  ioff()
  doall_w_traj(v,w,m.get_trajectories(),range(N.minframe,N.maxframe+1),N.dest_,N.fps)


  


