"""
  Example (in ipython -pylab):

    Plot average whisker angle for whisker 2 from the first session.

    >>> run qc_session.py
    >>> a = annotate_trials(r"F:\CuratedSessions", filename=r"F:\CuratedSessions\all.trialtypes.pickle")
    >>> out = a.by_type_session()
    >>> f = lambda i: gen_trial_matrix(list(seq2measurements(out[str(i)].values()[0])),iwhisker=2,ifeature=5)
    >>> for i in range(1,6): plot(f(i).mean(0))
    >>> legend(out.keys())

"""
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

  return gen_trial_matrix(list(gen_names),iwhisker,ifeature)

def seq2measurements(seq_generator):
  for n in seq_generator:
    yield os.path.splitext(n)[0]+'.measurements'

def select_by_trialid(annotation,trialid):
  for k,v in annotation.data.iteritems():
    if v == trialid:
      yield os.path.splitext(k)[0]+'.measurements'


def gen_trial_matrix(generator,iwhisker=0,ifeature=5):

  filecount = len(list(generator))
  timecount = 0;
  for n in generator:
    table = traj.MeasurementsTable(n)    
    data = table.asarray()
    timecount = data[:,1].max()+1 + 10 #adding a fudge factor
    break

  im = nan*zeros((filecount,timecount));

  for i,n in enumerate(generator):
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

def gen_trial_matrix2(generator,iwhisker=0,ifeatures=[5,6]):

  nfeat = len(ifeatures)
  filecount = len(list(generator))
  timecount = 0;
  for n in generator:
    table = traj.MeasurementsTable(n)    
    data = table.asarray()
    timecount = data[:,1].max()+1 + 10 #adding a fudge factor
    break

  im = nan*zeros((nfeat,filecount,timecount));

  for i,n in enumerate(generator):
    try:
      print "[%5d of %5d] %s"%(i,filecount,n)
      table = traj.MeasurementsTable(n)    
      data = table.asarray()

      mask = data[:,0]==iwhisker; # select whisker
      t = data[mask,1].astype(uint);    # get times
      for j,ifeat in enumerate(ifeatures):
        im[j,i,t] = data[mask,ifeat];# get feature
    except:
      pass

  return im


def robustmean(im):
  out = im.mean(0)
  for icol in find(isnan(out)):
    mask = ~isnan(im[:,icol])
    out[icol] = im[mask,icol].mean()
  return out


import fnmatch
import os
from reader import Reader
import cPickle as pickle
class annotate_trials:
  """
    This class is a little utility that is launched when the class is
    instanced.  It will look for data in the file specified by <filename> and
    match that against .seq files (recursively) found under <root>.  For any
    unmatched files, a window will pop showing a frame.  The user has to click
    on the image to give it keyboard focus and then press a key on the keyboard
    to annotate it.  After each keypress, an image for the next unmatched file
    will be shown, and the process repeated until every .seq file under <root>
    is annotated.

    As annotation is happening, data is saved to <filename>.  If the annotation
    process is interupted, it can be picked up later and completed by
    instancing the class with the same arguments.

    Example:

    >>> a = annotate_trials(r"F:\CuratedSessions",r"F:\all.trialypes.pickle")
    >>> for e in a.data.keys(): print e
    F:\CuratedSessions\LTPANM25668_111609\LTPANM25668_111609_C1_A_0078.seq
    F:\CuratedSessions\LTPANM41663_012310\LTPANM41663_012310_C3_B_0066.seq
    F:\CuratedSessions\LTPANM25668_111609\LTPANM25668_111609_C1_A_0084.seq
    ...
    F:\CuratedSessions\LTPANM41663_012310\LTPANM41663_012310_C2_D_0012.seq
    
  """

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
    try:
      self.n = self.g.next()
    except StopIteration:
      return

    # start annotating if there is any work to do
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

  def by_type_session(self):
    out = {}
    for k,v in self.data.iteritems():
      t = out.get(v,{})
      head = lambda x: os.path.split(x)[0]
      tail = lambda x: os.path.split(x)[-1]
      s = t.get( tail(head(k)), [] )
      s.append(k)
      t[tail(head(k))] = s
      out[v] = t;
    return out
      
def plot_angle_v_curvature(isession,saveto=''):
  a = annotate_trials(r"F:\CuratedSessions", filename=r"F:\CuratedSessions\all.trialtypes.pickle")
  out = a.by_type_session()
  #do 3 whiskers
  for i in range(3):
    f = lambda k: gen_trial_matrix2(
        list(seq2measurements(out[str(k)].values()[isession])),i,[5,6])
    figure()
    # do first 3 trial types, last two are less interesting (more noisy)
    for j in range(1,4):
      im = f(j)
      plot(im[0].ravel(),im[1].ravel(),'.',alpha = 0.1)
    axis([-130,-10,-0.015,0.015])
    legend(map(str,range(1,4)))
    title('Whisker %d'%i)
    if saveto:
      session = out.values()[0].keys()[isession]
      savefig(os.path.join(saveto,session+'__angle_v_curvature__whisker_%d.png'%i))

def plot_specgrams(isession):
  a = annotate_trials(r"F:\CuratedSessions", filename=r"F:\CuratedSessions\all.trialtypes.pickle")
  out = a.by_type_session()
  #do 3 whiskers
  for i in range(3):
    f = lambda k: gen_trial_matrix(
        list(seq2measurements(out[str(k)].values()[isession])),i,5)
    figure()
    for j in range(1,6):
      subplot(5,1,j)
      trials = f(j)
      trials[isnan(trials)] = trials[~isnan(trials)].mean()
      acc = None
      for row in trials:
        pxx,freqs,bins,im=specgram(row,Fs=1000,interpolation='nearest',NFFT=128,noverlap= 64,scale_by_freq=1,hold=0)
        if acc is None:
          acc = log(pxx)
        else:
          acc += log(pxx)
      acc /= float(len(trials))
      cla()
      imshow(acc,interpolation='nearest')
      axis("tight")

def gen_trial_matrix_all_whiskers(n_whiskers=3,ifeatures=[5,6]):
  a = annotate_trials(r"F:\CuratedSessions", filename=r"F:\CuratedSessions\all.trialtypes.pickle")
  out = a.by_type_session()
  n_trials = len(a.data)
  n_trial_type = len(out.keys())
  n_sessions = len(out.values()[0].keys())

  def get_n_timepoints():
    table = traj.MeasurementsTable(
        list(seq2measurements([a.data.keys()[0]]))[0] ) # -_-;
    data = table.asarray()
    return data[:,1].max() + 10 # add a fudge factor

  # each row: trial type id, session id, whisker id
  index = zeros((n_trials*n_whiskers,3),dtype=uint8) 
  im = nan*zeros((len(ifeatures),n_trials*n_whiskers,get_n_timepoints()))

  session_index = {}
  for i,k in enumerate(out.values()[0].iterkeys()):
    session_index[k] = i

  row = 0
  for kTrialType,vTrialType in out.iteritems():
    for kSession,vSession in vTrialType.iteritems():
      for filename in seq2measurements(vSession):
        print "[%5d of %5d] %s"%(row,im.shape[1],filename)
        try:
          data = traj.MeasurementsTable(filename).asarray();
          for iWhisker in xrange(n_whiskers):
            index[row,:] = [int(kTrialType), session_index[kSession], iWhisker]
            mask = data[:,0] == iWhisker;
            for i,ifeat in enumerate(ifeatures):
              im[i,row,data[mask,1].astype(uint32)] = data[mask,ifeat]
            row += 1
        except IOError:
          pass

  return index,im
