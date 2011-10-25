"""
  >>> meta = meta()
"""
import traj
from numpy import *
from pylab import *
import pdb
import os

#ROOT = r"H:\DOM3" 
ROOT = r"I:\data\DHO"
META = os.path.join(ROOT,"DHO.yaml")

def backup_measurements(root,backupname):

  import fnmatch
  import os
  import shutil

  def gen_names(root):
    for r,dirnames,filenames in os.walk(root):
      for filename in fnmatch.filter(filenames,'*.measurements'):
        yield os.path.join(r,filename)

  def subroot(root,name):
    s = os.path.split  #alias
    j = os.path.join
    head,post = s(name)
    while head != root:
      head,tail = s(head)
      post = j(tail,post)
    return post

  def ensure(name):
    s = os.path.split  #alias
    isfname = lambda x: os.path.splitext(x)[-1] != '' 
    def gen():
      h,t = s(name)
      while t:
        yield h
        h,t = s(h)
    for n in reversed(list(gen())):
      if not os.path.exists(n):
        os.mkdir(n)
      
  broot = backupname #os.path.join(root,backupname)

  N = len(list(gen_names(root)))
  for i,e in enumerate(gen_names(root)):
    d = os.path.join(broot,subroot(root,e))
    print "[%6d of %6d] %s"%(i,N,d)
    ensure(d)
    shutil.copy( e,d )

def loadmeta():
  import yaml
  return yaml.load(open(META,"r"))

def savemeta(meta):
  import yaml
  return yaml.dump(meta,open(META,"w"))

def asdate(d):
  import datetime
  fmts = [
      '%Y-%m-%d',
      '%m-%d-%Y',
      '%Y/%m/%d',
      '%m/%d/%Y',
      '%m-%d-%y',
      '%m/%d/%y',
      '%y-%m-%d',
      '%y/%m/%d',
      ]
  ifmt = 0
      
  if type(d) is str:
    while ifmt<len(fmts):
      try:
        return asdate(datetime.datetime.strptime(d,fmts[ifmt]))
      except ValueError:
        ifmt+=1
    if ifmt>=len(fmts):
      raise ValueError("Could not parse string: %s"%d)
  elif type(d) is datetime.datetime:
    return datetime.date(d.year,d.month,d.day)
  elif type(d) is datetime.date:
    return d

def aspath(anm,date,root,tid,ext):
  import os
  dstr = date.strftime("%m%d%y")
  return reduce(os.path.join,[
    root,
    anm,
    anm+'x'+dstr,
    anm+'x'+dstr+('_%04d'%tid)+ext])

def gen_trials_w_keys(meta,anm=None,date=None):
  import os
  if anm is None:
    for anm in meta.iterkeys():
      for v in gen_trials_w_keys(meta,anm,None):
        yield v
  else:
    anm = anm.upper()
    if date is None:
      for date in (x for x in meta.get(anm,{}).iterkeys() if type(x) is datetime.date):
        for v in gen_trials_w_keys(meta,anm,date):
          yield v
    else:
      date = asdate(date) #<date> can be passed in as a string or datetime object
      for itrial,tdata in enumerate(meta.get(anm,{}).get(date,{}).get('trials',[])):
        fname = aspath(anm,date,ROOT,tdata['id'],'.measurements')
        if os.path.exists(fname):
          yield (anm,date,itrial),tdata

def gen_measurements_w_keys(meta,anm=None,date=None):
  for (a,d,i),v in gen_trials_w_keys(meta,anm,date):
    fname = aspath(a,d,ROOT,v['id'],'.measurements')
    yield (a,d,i),fname

def gen_measurements(meta,anm=None,date=None): 
  for (anm,date,itrial),fname in gen_measurements_w_keys(meta,anm,date):
    yield fname

def head(g):
  return g.next()

def countof_frames(fname):
  v = traj.MeasurementsTable(fname).asarray()[:,1].max() 
  print fname, v
  return int(v)

def update_meta(meta):
  for (anm,date,itrial),fname in gen_measurements_w_keys(meta):
    meta[anm][date]['trials'][itrial]['nframes'] = countof_frames(fname)

def filter_trial_type(meta,typestr='CR'):
  import copy
  out = copy.deepcopy(meta)
  for (anm,date,itrial),tdata in gen_trials_w_keys(meta):
    if tdata['typestr']!=typestr:
      out[anm][date]['trials'].remove(tdata) 
  return out

class UnknownFrameRate(Exception):
  def __init__(self,value):
    self.value = value
  def __str__(self):
    return repr(self.value)

def frame_delta_ms(nframes):
  d = { 1559: 4,
        1560: 4,
        1561: 4,
        3119: 2,
        3121: 2,
        3122: 2 } # reject everything else
  try:
    return d[nframes]
  except KeyError:
    raise UnknownFrameRate(nframes)

def wrap(th):
  if(th>0.0):
    return th-180.0
  else:
    return th+180.0
def awrap(th):
  v = 180.0*ones(th.shape);
  v[th>0.0] = -180.0;
  return th+v

def build_histograms(fname,min_ms,max_ms,norm=True):
  data = traj.MeasurementsTable(fname).asarray()  
  out = {}

  nframes = data[:,1].max()
  dt = frame_delta_ms(nframes)

  wmask = (data[:,0]    ==0)      # select whisker 0
  tmask = (data[:,1]*dt >=min_ms) \
        * (data[:,1]*dt <=max_ms) # select epoch
  
  # angle of most posterior whisker
  mask = wmask*tmask
  nn,xx = histogram(awrap(data[mask,5]),arange(-180.0,180.0,1.0),density=norm)
  out['angle'] = (xx[1:],nn)
  
  # avg angle change between whiskers in the same frame
  nwhiskers = int(data[:,0].max())+1
  nframes   = int(data[:,1].max())+1
  angles    = nan*zeros((nframes,nwhiskers)) 
  for i in xrange(nwhiskers):
    mask = (data[:,0]==i)*tmask
    angles[data[mask,1].astype(int),i] = awrap(data[mask,5])
  dth = diff(angles,axis=1).mean(axis=1)
  nn,xx = histogram(dth,arange(-180.0,180.0,1.0),density=norm)
  out['spread'] = (xx[1:],nn)

  #all angles
  mask = tmask*(data[:,0]>=0)*(data[:,0]<=2) # actually only accept most posterior 3
  nn,xx = histogram(awrap(data[mask,5]),arange(-180.0,180.0,1.0))
  out['all'] = (xx[1:],nn)
  return out

def hist_matrix(meta,anm,date,min_ms=0,max_ms=2000,key='angle',norm=True):
  ntrials = len(list(gen_trials_w_keys(meta,anm,date)))
  t=build_histograms(head(gen_measurements(meta)),min_ms,max_ms,norm=norm)
  nbins = t[key][0].shape[0]

  m = nan*zeros((ntrials,nbins))
  for i,fname in enumerate(gen_measurements(meta,anm,date)):
    try:
      m[i,:] = build_histograms(fname,min_ms,max_ms,norm=norm)[key][1]
    except UnknownFrameRate:
      pass
  return m[~isnan(m).all(axis=1),:] #filter out trials with unknown frame rate

def plot_hist_matrix_means(meta,anm,**kwargs):
  days = sorted([d for d in map(str,meta[anm]) if type(d) is datetime.date])
  for d in days:
      out = hist_matrix(meta,anm,d,**kwargs)
      plot(out)
  legend(days)

