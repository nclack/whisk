#!/usr/bin/env python
"""
This is compatible with batch.py.  Calling:

    batch.py --exe="summary.py" -f *.whiskers path/to/data

will call this as:

    summary.py path/to/data/somefile.whiskers path/to/data/somefile

A more complicated example:

    ./batch.py --exe="./summary.py" -f *.whiskers data/seq/ --args="--imagedir=temp"

will put all the rendered images into ./temp.

Example:

>>> w,movie = load("data/seq/whisker_data_0140.seq","seq-heal.whiskers")
>>> data = classify(w,1500,100)
>>> res = transform_classification_data(data)
>>> traj = make_trajectories( w, res, 0, 4 )
>>> plot_summary_data(w,traj,data)

or 

>>> w,movie = load("data/seq/whisker_data_0140.seq","seq-heal.whiskers")
>>> traj,data = autotraj(w)
>>> plot_summary_data(w,traj,data)

Author: Nathan Clack
Date: 2009-05-26
Copyright (c) 2009 HHMI. Free downloads and distribution are allowed as long as proper
credit is given to the author.  All other rights reserved.
"""
from numpy import *
import numpy
from pylab import *
from features import *
import os
import pdb
import warnings
from warnings import warn

def load(moviename, whiskersname):
  from ui.whiskerdata import load_whiskers, load_trajectories
  from ui.genetiff import Reader
  movie = Reader(moviename,adjuststipple=1)
  w,wid = load_whiskers(whiskersname)
  return w,movie

def trajmask(t):  # returns a mask over frames of the movie - true means trajectory labels were applied there
  mask = zeros( max(t.keys())+1 )
  mask[ t.keys() ] = 1
  return mask

def hist_length(wvd):
  def lengths(wvd):
    for wv in wvd.itervalues():
      for w in wv.itervalues():
        yield integrate_path_length(w)
  L = array(list(lengths(wvd)))
  hist(L, range(ceil(L.max())), edgecolor='none')
  title('Histogram of trace lengths');
  ylabel('Counts')
  xlabel('Path length (px)')

def features(wvd):
  warnings.simplefilter("ignore")
  for fid,wv in wvd.iteritems():
    for wid,w in wv.iteritems():
      try:  # 0  1   2              3                     4                5                6                 7        8       9        10
        yield 0,fid,wid,integrate_path_length(w), median_score(w), root_angle_deg(w,0), mean_curvature(w,0), w.x[0], w.y[0], w.x[-1], w.y[-1]
      except TypeError:
        pass
  warnings.resetwarnings()

def pca(wvd):
  data = array(list(features(wvd)))[3:]
  zscore = lambda v: (v-v.mean())/v.std()
  for i in xrange( data.shape[1] ):
    data[:,i] = zscore(data[:,i])
  eval, evec = eig(cov(data.T))
  idx = argsort(eval)               # ascending
  proj = lambda i: dot(evec[:,i],data.T)
  return map( proj, reversed(idx) ) # descending (first corresponds to largest eigenvalue)

def classify(wvd, minscore = 1090, minlen = 122 ):
  data = array(list(features(wvd)))
  for row in data:
    fid = row[1]
    wid = row[2]
    w = wvd[fid][wid]
    l = integrate_path_length(w)
    s = median_score(w)
    if l > minlen or s > minscore:
      row[0] = 1
    else:
      row[0] = 0
  return data

def transform_classification_data(data):
  res = {}
  for row in data:
    fid = int( row[1] )
    wid = int( row[2] )
    res.setdefault( fid, {} )[wid] = ( int(row[0]), row[3], row[4] )
  return res

def estimate_number_of_trajectories(datadict):
  acc = 0
  for v in datadict.itervalues():
    e = [ row[0] for row in v.itervalues() ]
    acc  += sum(e)
  return acc/float(len(datadict))

def plot_frame_by_class(movie, wvd, datadict, iframe):
  ioff()
  cla()
  imshow(movie[iframe],hold=0, cmap=cm.gray);
  for wid,w in wvd[iframe].iteritems():
    if datadict[iframe][wid][0]:
      plot( w.x, w.y, 'g' )
    else:
      plot( w.x, w.y, 'r' )
  map(axis,["image","off"])
  subplots_adjust(0,0,1,1,0,0)
  ion()
  show()

def render_classes(path,movie,wvd,datadict):
  for iframe in range(len(movie)):
    print "%4d of %d"%(iframe, len(movie))
    plot_frame_by_class(movie,wvd,datadict,iframe)
    savefig(os.path.join(path,"render%04d.png"%iframe))

def make_trajectories( wvd, datadict, side, n=None ):
  if n is None:
    n = round( estimate_number_of_trajectories( datadict ))
  T = {}
  for fid, v in datadict.iteritems():
    good = [ wid for wid,datarow in v.iteritems() if datarow[0] == 1 ] # filter for whiskers in frame that are in the good class
    if len(good) == n: # got the expected number of whiskers
      mapping = argsort([ wvd[fid][wid].y[side] for wid in good  ])
      for i,idx in enumerate(mapping):
        T.setdefault(i,{})[fid] = good[idx]
  return T

def make_trajectories2( data, n=None, sort_column = 8 ):
  datadict = {}
  for row in data:
    fid = int( row[1] )
    wid = int( row[2] )
    datadict.setdefault( fid, {} )[wid] = ( int(row[0]), row[3], row[4], row[sort_column] )

  if n is None:
    n = round( estimate_number_of_trajectories( datadict ))

  T = {}
  for fid, v in datadict.iteritems():
    good = [ wid for wid,datarow in v.iteritems() if datarow[0] == 1 ] # filter for whiskers in frame that are in the good class
    if len(good) == n: # got the expected number of whiskers
      mapping = argsort([ v[wid][3] for wid in good  ])
      for i,idx in enumerate(mapping):
        T.setdefault(i,{})[fid] = good[idx]
  return T

def autotraj(wvd,side=0, data=None):
  """ 
  Uses kmeans to partition whisker segments into two sets: 
  
  class 1. high scoring and long
  class 2. low scoring and short

  The median number of class 1 segments in a frame is expected to correspond
  with the number of interesting whiskers; that is, the trajectories worth
  following.

  Following classification, a simple scheme is used to label segments in frames
  with the correct number of class 1 segments.

  If `data` is not provided, it will be computed from the whisker segments.
  The `data` table is an array with a row for each whisker segment consisting of
  a number of columns (3 + number of measurements).  The first column is a
  classification label, the second is the frame id, and the third is the 
  whisker id.  The `classification label` is overwritten here.

  Returns: traj,data

    'traj': a trajectories dictionary (see ui.whiskerdata.load_trajectories)
    'data': a table of shape measurements for each whisker segment

  Example:

    >>> import summary
    >>> w,movie = summary.load('data/my_movie.seq', 'data/my_movie[heal].whiskers')
    >>> traj,data = summary.autotraj(w)
    >>> summary.plot_summary_data(w,traj,data)

  """
  if data == None:
    data = array(list(features(wvd)));
  traj = _simpletraj(data)
  return traj, data

def _simpletraj(data):
  import scipy.cluster.vq as vq
  ilength = 3
  iscore  = 4
  obs = vq.whiten( data[:,[ilength, iscore]] )
  codebook,distortion = vq.kmeans( obs, 2 )
  idx = None
  if codebook[0,1] > codebook[1,1]:
    idx = 0
  else:
    idx = 1
  code, errs = vq.vq( obs, codebook )
  data[:,0] = (code==idx)
  res = transform_classification_data( data )
  traj = make_trajectories2(data, sort_column = 8 )
  return traj

def commit_traj_to_data_table( traj, data ):
  index = {}
  for i,row in enumerate(data):
    index[ ( row[1], row[2] ) ] = i;

    data[:,0] = -1 #null tid
    for tid, t in traj.iteritems():
      for fid, wid in t.iteritems():
        try:
          data[ index[ (fid,wid) ], 0 ] = tid
        except KeyError:
          warn("Trajectory referenced a non-existent whisker (tid:%d, fid:%d, wid:%d)"%(tid,fid,wid))
  return data

def plot_whiskers_by_trajectory(movie, wvd, traj, iframe):
  #ioff()
  cla()
  imshow(movie[iframe],hold=0, cmap=cm.gray);
  colors = cm.jet([x/float(len(traj)) for x in xrange(len(traj))])
  for wid,w in wvd[iframe].iteritems():
    hit = 0
    for i,t in traj.iteritems():
      if wid == t.get(iframe):
        plot( w.x, w.y, color = colors[i], linewidth = 4, alpha = 0.5 )  
        hit = 1
        break
    if not hit:
        plot( w.x, w.y, 'k', linewidth = 2, alpha = 0.7 )  

  map(axis,["image","off"])
  subplots_adjust(0,0,1,1,0,0)
  #ion()
  show()

def _render_trajectories_single_frame( path, movie, wvd, traj, iframe ):
  try:
    print "%4d of %d"%(iframe, len(movie))
    plot_whiskers_by_trajectory(movie,wvd,traj,iframe)
    savefig(os.path.join(path,"render%04d.png"%iframe))
  except Exception, e:
    print e

def render_trajectories(path,movie,wvd,traj):
  for iframe in range(len(movie)):
    _render_trajectories_single_frame( path, movie, wvd, traj, iframe )


def organize_by_trajectories(wvd,traj):
  """ For each trajectory returns a list of whisker segments """
  xform = lambda i: [ wvd[fid][wid] for fid,wid in traj[i].iteritems()]
  return map( xform, traj.keys() )

def organize_data_by_traj(data,traj):
  def itertrajinv(traj):
    for tid,v in traj.iteritems():
      for fid,wid in v.iteritems():
        yield (fid,wid),tid
  invtraj = dict( [p for p in itertrajinv(traj) ] )
  index = ones( (data.shape[0],1) ) * -1 
  for i,row in enumerate(data):
    index[i] = invtraj.get( (int(row[1]), int(row[2])) , -1 )
  return [ data[ where(index==it)[0],: ] for it in traj.iterkeys() ]

def plot_summary_data(wvd,traj,data):
  clf()
  tsd = organize_data_by_traj( data,traj )
  tmax = 0
  
  mask = trajmask(traj[0])

  ax = subplot(211)
  vmin1,vmax1 = -90,90
  ax.broken_barh( [(i-0.5,1) for i,e in enumerate(mask) if not e], (vmin1,vmax1-vmin1) ,edgecolors=[(0,0,0,0)],facecolors=[(0,0,0,0.5)] )
  xlabel('Time (frames)')
  ylabel('Angle at root (deg)')
  ax = subplot(211)
  
  ax = subplot(212)
  vmin2,vmax2 = -0.008,0.008
  ax.broken_barh( [(i-0.5,1) for i,e in enumerate(mask) if not e], (vmin2,vmax2-vmin2) ,edgecolors=[(0,0,0,0)],facecolors=[0,0,0,0.5] )
  xlabel('Time (frames)')
  ylabel('Mean Curvature (1/px)')

  #warnings.simplefilter("ignore")
  for tdata in tsd:
    tt = tdata[:,1] # frame id == time
    tmax = max(tmax, tt.max())
    th = tdata[:,5]          #array([ root_angle(w,side)*180.0/pi      for w in e])
    k  = tdata[:,6] #array([ mean_curvature(w,side)           for w in e]) 
    subplot(211)
    plot(tt,th)
    subplot(212)
    plot(tt,k )
  #warnings.resetwarnings()

  ax = subplot(211)
  axis((0,tmax,vmin1,vmax1))
  ax = subplot(212)
  axis((0,tmax,vmin2,vmax2))


def plot_summary(wvd,traj,side=0):
  clf()
  ts = organize_by_trajectories( wvd, traj )
  tmax = 0
  
  mask = trajmask(traj[0])

  ax = subplot(211)
  vmin1,vmax1 = -90,90
  ax.broken_barh( [(i-0.5,1) for i,e in enumerate(mask) if not e], (vmin1,vmax1-vmin1) ,edgecolors=[(0,0,0,0)],facecolors=[(0,0,0,0.5)] )
  xlabel('Time (frames)')
  ylabel('Angle at root (deg)')
  ax = subplot(211)
  
  ax = subplot(212)
  vmin2,vmax2 = -0.008,0.008
  ax.broken_barh( [(i-0.5,1) for i,e in enumerate(mask) if not e], (vmin2,vmax2-vmin2) ,edgecolors=[(0,0,0,0)],facecolors=[0,0,0,0.5] )
  xlabel('Time (frames)')
  ylabel('Mean Curvature (1/px)')

  #warnings.simplefilter("ignore")
  for e in ts:
    tt = array([ time(w)                          for w in e])
    tmax = max(tmax, tt.max())
    th = array([ root_angle_deg(w,side)           for w in e])
    k  = array([ mean_curvature(w,side)           for w in e]) 
    subplot(211)
    plot(tt,th)
    subplot(212)
    plot(tt,k )
  #warnings.resetwarnings()

  ax = subplot(211)
  axis((0,tmax,vmin1,vmax1))
  ax = subplot(212)
  axis((0,tmax,vmin2,vmax2))

def plot_distributions_by_class(data):
  nfeat = data.shape[1] - 3 # first 3 columns are class_id, frame_id, and segment_id
  clf()
  mask = data[:,0]==0

  for i in xrange(nfeat):
    ic = 3+i
    subplot(nfeat,1,i)
    avg = data[:,ic].mean()
    wid = 3.0*data[:,ic].std()
    bins = linspace( max( avg-wid, data[:,ic].min() ), min(avg+wid,data[:,ic].max()), 100 )
    #bins = linspace( data[:,ic].min(), data[:,ic].max(), 100 )
    hist( data[mask,ic], bins, ec='none', fc = 'r',alpha=0.5, normed=1 )
    hist( data[~mask,ic], bins, ec='none', fc = 'g',alpha=0.5, normed=1 )

def plot_distributions_by_trajectory(data,traj,cmap = cm.jet):
  clf()

def plot_distributions_by_trajectory_nodata(w,traj,cmap = cm.jet):
  clf()
  ts = organize_by_trajectories(w,traj)

  subplot(311)
  vmax = 0
  v = {}
  for i,tse in enumerate(ts):
    v[i] = array([ integrate_path_length(e) for e in tse ])
    vmax = max( vmax, v[i].max() )
  for i,ve in v.iteritems():
    hist(ve,arange(ceil(vmax)),ec='none',width=1,fc=cmap(i/float(len(v))),alpha = 0.5,normed=1);
  xlabel('Segment length (px)')
  ylabel('Density')

  subplot(312)
  vmax = 0
  v = {}
  for i,tse in enumerate(ts):
    v[i] = array([ median_score(e) for e in tse ])
    vmax = max( vmax, v[i].max() )
  for i,ve in v.iteritems():
    hist(ve,arange(ceil(vmax)),ec='none',width=1,fc=cmap(i/float(len(v))),alpha = 0.5,normed=1);
  xlabel('Median Score')
  ylabel('Density')

  subplot(313)
  vmax = 0
  v = {}
  for i,tse in enumerate(ts):
    v[i] = array([ median_thick(e) for e in tse ])
    vmax = max( vmax, v[i].max() )
  for i,ve in v.iteritems():
    hist(ve,arange(0,ceil(vmax),0.5),ec='none',width=1,fc=cmap(i/float(len(v))),alpha = 0.5,normed=1);
  xlabel('Median Thickness')
  ylabel('Density')

def render_summary_to_file(whiskername,figurename):
  from trace import Load_Whiskers
  assert( os.path.exists(whiskername) )
  w = Load_Whiskers(whiskername)
  traj,data = autotraj(w)
  f = figure()
  title(os.path.split(whiskername[1]))
  plot_summary_data(w,traj,data)
  savefig(figurename)
  close(f)
  return w,traj,data

if 1:
  import optparse
  if __name__ == '__main__':
    usage = "usage: summary.py [options] whiskerfile [destination_image]"
    description = \
"""
This utility makes trajectories from whisker segments and renders a plot of whisker curvature and angle.
The plot is saved to the file name specified by the optional `destination_image` argument.  If this 
argument isn't supplied the filename is automatically determined based on the whisker file name.

The resulting trajectories are not, by default, saved.  However, by specifying the appropriate option they
can be saved to a .trajectories file (the filename is determined by the whisker file name).

The `destination_image` argument may be a filename with or without the extension.  If no extension is specified
the file will be saved as a .png file.  The extension is used to determine the file format to save.
Available formats are pdf, ps, eps, and svg.


"""
    parser = optparse.OptionParser( usage = usage,
                                    description = description )
    parser.add_option('-s','--save-trajectories',
                        dest = "savetraj",
                        action = "store_true",
                        default = False,
                        help = "Causes computed trajectories to be written to file.")
    parser.add_option("--label",
                    help    = "This is used when determining the destination image file name. [default: %default]",
                    dest    = "label",
                    action  = "store",
                    type    = "string",
                    default = "") 
    parser.add_option("--imagedir",
                    help    = "This is used when determining where to save images  [default: %default]",
                    dest    = "imagedir",
                    action  = "store",
                    type    = "string",
                    default = "") 
    options, args = parser.parse_args()
    
    whiskername = args[0]
    trajname = os.path.splitext(whiskername)[0] + '.trajectories'

    destname = None
    if len(args)>1: #if the destination image file name was provided (or some variant thereof)
      beginning,ending = os.path.split( args[1] )
      middle,ending    = os.path.splitext( ending )
      destname = args[1]
      if options.imagedir:
        if beginning:
          warn("The destination image file name was specified with a path.  Going with the --imagedir option.");
        destname = os.path.join( options.imagedir, middle )
      if not ending:
        destname += '.png'
      else:
        destname += ending
    else: #derive destination file name from whisker filename and options
      beginning,ending = os.path.split( whiskername )
      middle,ending    = os.path.splitext( ending )
      lblpart = ''
      if options.label:
        lblpart = '[%s]'%options.label
      if options.imagedir:
        beginning = options.imagedir
      destname = os.path.join( beginning, middle + lblpart + '.png' )

    destdir = os.path.split(destname)[0]
    if destdir: #not working directory
      assert( os.path.isdir(destdir) )

    w,traj,data = render_summary_to_file(whiskername,destname)
    numpy.save( os.path.splitext(destname)[0] + '.npy', data );
    if options.savetraj:
      from ui.whiskerdata import save_trajectories
      save_trajectories( trajname, traj )
  
    
