import traj
from numpy import *
from pylab import *

def trial_matrix(root,iwhisker=0,ifeature=3):
  """ image plot of a feature for a whisker where each row is a trial
  """

  import fnmatch
  import os

  def gen_names(root):
    for r,dirnames,filenames in os.walk(root):
      for filename in fnmatch.filter(filenames,'*.measurements'):
        yield os.path.join(r,filename)

  filecount = len(list(gen_names(root)))
  timecount = 0;
  for n in gen_names(root):
    table = traj.MeasurementsTable(n)    
    data = table.asarray()
    timecount = data[:,1].max()+1 + 10 #adding a fudge factor
    break

  im = nan*zeros((filecount,timecount));

  for i,n in enumerate(gen_names(root)):
    try:
      print "[%5d of %5d] %s"%(i,filecount,n)
      table = traj.MeasurementsTable(n)    
      data = table.asarray()

      mask = data[:,0]==iwhisker; # select whisker
      t = data[mask,1].astype(uint);    # get times
      v = data[mask,ifeature];    # get feature
      im[i,t] = v;         # fill row
    except:
      pass

  return im

import fnmatch
import os
from reader import Reader
import cPickle as pickle
class annotate_trials:

  def __init__(self,root,iframe=1000,filename="default.trialtypes.pickle"):

    try:
      self.load(filename)
    except IOError:
      self.data = {}
    self.default_filename = filename

    def gen_names(root):
      for r,dirnames,filenames in os.walk(root):
        for filename in fnmatch.filter(filenames,'*.seq'):
          name = os.path.join(r,filename) 
          if not self.data.has_key(name):
            yield name

    self.iframe = iframe
    self.g = gen_names(root)
    self.n = self.g.next()
    movie  = Reader(self.n,adjuststipple=True)
    self.f = figure()
    def totitle(name):
      return os.path.split(name)[-1]
    def onkeypress(event):
      print totitle(self.n), event.key
      self.data[self.n] = event.key
      if(self.default_filename):
        self.save(self.default_filename)

      self.n = self.g.next()
      print "Loading: "+totitle(self.n)
      movie = Reader(self.n,adjuststipple=True)
      print "Showing"
      clf()
      imshow(movie[self.iframe],interpolation='nearest')
      self.drawrefs()
      grid(True)
      axis('image')
      title(totitle(self.n))
      draw()


    self.f.canvas.mpl_connect('key_press_event',onkeypress)
    imshow(movie[self.iframe],interpolation='nearest')
    self.drawrefs()
    grid(True)
    title(totitle(self.n))
    axis('image')
    gray()  
    show()

  def drawrefs(self):
    #bar positions - specific to Leo's curated data
    xs = [129,179,228,275,426]
    ys = [88,88,88,88,88]
    for i,(x,y) in enumerate(zip(xs,ys)):
      text(x,y,str(i+1),color='r')
    plot(xs,ys,'r.')

  def save(self,filename):    
    pickle.dump(self.data,open(filename,"w"));
    self.default_filename = filename

  def load(self,filename):
    self.data = pickle.load(open(filename,"r"));
    self.default_filename = filename
