import sys
from matplotlib import cm,rc
from matplotlib.pylab import * #imshow,draw,colorbar,clf,clabel,axis,savefig
from numpy import *
import roc,trace

rc('image',interpolation='nearest');
rc('figure.subplot',bottom=0.05,left=0.05,top=0.95,right=0.95);
rc('figure',facecolor='w');

def play_seed_angles(video,maxr=9,thresh=0.99,alpha=0.75,bg=63,every=slice(None),renderto=None):
  mask = video[::100].max(axis=0)>bg
  f = lambda im: trace.compute_seed_fields_windowed(im,maxr=maxr,maxiter=1,window=(0.0,0.0))
  def make(im):
    angles = abs(f(im)[1]*180/pi);
    out = cm.hsv(angles/180.0);
    out[:,:,3] = mask*(f(im)[2]>thresh)*alpha;
    return out,angles

  rc('image',interpolation='nearest');
  rc('figure.subplot',bottom=0.05,left=0.05,top=0.95,right=0.95);
  rc('figure',facecolor='w');
  for i,im in enumerate(video[every]):
    out,angles = make(im)
    clf();
    imshow(angles,cmap=cm.hsv); 
    colorbar().set_label('Angle (degrees)');
    imshow(im,hold=0,cmap=cm.gray);
    imshow(out,hold=1);
    axis('off');
    draw();
    if renderto:
      print 'Writing ',renderto%i
      sys.stdout.flush()
      savefig(renderto%i);

### NOTES
#
# o  It's interesting to look at precision vs recall as a function of r.
#    o Curve converges as r increases.  Almost done at r=5 (precision w.in 5%)
#      As r increases, recall gets better but precision stays about the same.
# o  Another predicate would be if the seed was on the traced curve...
#    but I'm not sure that would be more interpretable.  With
#    is_trace_generated precision is the fraction of productive seeds,
#    and recall is the fraction of productive seeds called.
#
#    Not sure what the minimal recall should be.
#    o Using the whisker trace as truth: this would be a minimally sufficient
#      set of points.  That is, we want to have very close to 100% recall 
#      on these.
#
#    o Perhaps look at roc curves on grid lines only.
#
# o Why are some productive seeds low scoring?
#   - because there is some smutz in the "is_trace_generated" mask
#     [x] Cleaned this up by filtering for 'real' whiskers 
#
def is_trace_generated(image,r=10,thresh=100):
  """ is_trace_generated(image) -> image mask
  A predicate to use for generating roc curves against a seed's salience.

  >>> truth  = is_trace_generated(image);
  >>> scores = trace.compute_seed_fields_windowed(image,maxr=4,maxiter=1,window=(0.0,0.0))[2]
  >>> precision,recall,thresholds = roc(scores,truth)
  """
  out = zeros(image.shape)
  f = lambda im: trace.compute_seed_fields_windowed(im,maxr=r,maxiter=1,window=(0.0,0.0))
  angle_radians = f(image)[1];
  for y in xrange(0,image.shape[0]):
    for x in xrange(0,image.shape[1]):
      seed = trace.cSeed(x,y,int(cos(angle_radians[y,x])*100),int(sin(angle_radians[y,x])*100))
      t = trace.Trace_Whisker(seed,image)
      if(t and len(t.x)>thresh):
        out[y,x] = 1
  return out

def trace_all(image,r=10,thresh=100):
  out = []
  f = lambda im: trace.compute_seed_fields_windowed(im,maxr=r,maxiter=1,window=(0.0,0.0))
  angle_radians = f(image)[1];
  for y in xrange(0,image.shape[0]):
    for x in xrange(0,image.shape[1]):
      seed = trace.cSeed(x,y,int(cos(angle_radians[y,x])*100),int(sin(angle_radians[y,x])*100))
      t = trace.Trace_Whisker(seed,image)
      if(t and len(t.x)>thresh):
        out.append(t)
  return out

def genmask(video,border=10):
  mask = video[::100].max(axis=0)>63;
  mask[:border,:] = 0
  mask[-border:,:] = 0
  mask[:,:border] = 0
  mask[:,-border:] = 0
  return mask

