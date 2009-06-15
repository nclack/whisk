from pylab import *
import pdb

try:
  from enthought.mayavi import mlab
  from enthought        import tvtk

  def view3d(stk):
    vol = mlab.pipeline.volume( mlab.pipeline.scalar_field(stk) )
    vol.volume_mapper.blend_mode = 'maximum_intensity'
    vol.volume_mapper.sample_distance = 0.5
    vol.volume_property.interpolation_type = 'nearest'
    
    ctf = tvtk.util.ctf.save_ctfs(vol.volume_property)
    ctf['alpha'] = [[stk.min(),0.0],[stk.max(),1.0]]
    tvtk.util.ctf.load_ctfs(ctf,vol.volume_property)
    return vol
except ImportError:
  from warnings import warn
  warn("Couldn't import mayavi.mlab.  3D visualization disabled.")

def load():
  from ui.whiskerdata import load_whiskers, load_trajectories
  w,wid = load_whiskers('test.whiskers')
  #w,wid = load_whiskers('results/seq-hand.whiskers')
  #t,tid = load_trajectories('results/seq-hand.trajectories')
  return w#,t

###
# PCA
#

###
# Distance
#

def wdist_trunc( a,b, n=50 ):
  idx = min( a.x.shape[0], b.x.shape[0] )
  def interp( a, x ):
    f,i = modf(x)
    b = a[i]
    d = a[i+1] - b;
    return b + d * f;
  samples = linspace( 0, idx-1, n, endpoint=False )
  return array([ hypot( interp(b.x,s)-interp(a.x,s), interp(b.y,s)-interp(a.y,s) ) for s in samples ]).mean()

def wdist(a,b,n=50):
  def interp(a,x):
    """ x is from 0 to 1 """
    idxf = (a.shape[0]-1) * x
    f,i = modf(idxf)
    b = a[i]
    d = a[i+1] - b;
    return b + d * f;
  samples = linspace( 0, 1, n, endpoint = True  )
  return array([ hypot( interp(b.x,s)-interp(a.x,s), interp(b.y,s)-interp(a.y,s) ) for s in samples[:-1] ]).mean()

###
# Ordering and aligment
#

def whisker_cmp(a,b):
  return cmp( a.y[0], b.y[0] )

def wvdsort(wvd):
  return sorted( wvd.values(), whisker_cmp )

def wvdsort_get_keys(wvd):
  return [x[0] for x in sorted(wvd.items(), cmp=whisker_cmp, key = lambda x:x[-1]) ]

def plot_whiskers( wv, *args, **kwargs ):
  #xform = lambda a: 2*((a/2).round().astype(int))
  texcludes = ['marker','linestyle','markersize','linewidth']
  tkwargs = {} #'backgroundcolor':'w'}
  for k,v in kwargs.iteritems():
    if not k in texcludes:
      tkwargs[k] = v
  g = (el for el in enumerate(wv)) #default generator
  if hasattr(wv,'iteritems'):
    g = (el for el in wv.iteritems())
  else:
    g = ( (hex(id(e)),e) for e in wv )

  for i,w in g:
    #plot( xform(w.x), xform(w.y), *args, **kwargs)
    plot( w.x, w.y, *args, **kwargs)
    idx = where( w.x == w.x.max() )[0][0]
    text( w.x[idx], w.y[idx], str(i), *args, **tkwargs )
  #axis('image')
  show()

###
# DP
#

def dpmatrix( av, bv ):
  m = zeros(( len(av)+1, len(bv)+1 ))
  m[:,0] = inf
  m[0,:] = inf
  m[0,0] = 0.0
  for i,a in enumerate(av):
    for j,b in enumerate(bv):
      cost = wdist(a,b)
      m[i+1,j+1] = cost + min( m[ i  , j+1 ],
                               m[ i+1, j   ],
                               m[ i  , j   ] )
      #pcolor(m[1:,1:],vmin=0,vmax=100,hold=0)
      #pdb.set_trace()
  return m

def traceback(dp):
  def choose(i,j):
    if i<=0 or j<=0:
      raise IndexError
    idx = [[i-1,j],[i-1,j-1],[i,j-1]]
    v = array( [ dp[r,c] for r,c in idx ] )
    best = where( v == v.min() )[0][0]
    return idx[best]
  p = [ [x-1 for x in dp.shape] ]
  try:
    while 1:
      p.append( choose( *p[-1] ) )  
  except IndexError:
    pass
  return array(p)[:-1] - 1

def traceforward(dp):
  def choose(i,j):
    idx = [[i+1,j],[i+1,j+1],[i,j+1]]
    v = array( [ dp[r,c] for r,c in idx ] )
    best = where( v == v.min() )[0][0]
    return idx[best]
  p = [ [ 1,1 ] ]
  try:
    while 1:
      p.append( choose( *p[-1] ) )  
  except IndexError:
    pass
  return array(p) - 1

def dptrace_to_merge_program_1(seq ):
  """
  Doesn't seem to work.

  I'm pretty sure that the way I'm computing the alignment table
  means that I can't tell the direction to merge just using the 
  table.

  I have a hunch I should be able to though...
  """
  ii,jj = where( diff(seq,axis=0)==0 )
  for i,j in zip(ii,jj):
    seq[i + j][j] = -1
  return seq

def dptrace_to_merge_program( av, bv, seq ):
  ii,jj = where( diff(seq,axis=0)==0 ) #find inserts
  for i,j in zip(ii,jj):
    d0 = wdist( av[seq[i  ][0]], bv[seq[i  ][1]] )
    d1 = wdist( av[seq[i+1][0]], bv[seq[i+1][1]] )
    if d0 < d1:  #set next for insert
      seq[i + 1][j] = -1
    else:        #set current for insert
      seq[i    ][j] = -1
  return seq

def wplot_by_merge_program( av, bv, merge, cmap = cm.jet ):
  colors = cmap( linspace( 0, 1, merge.shape[0] ) )
  for i,(m,c) in enumerate(zip( merge,colors )):
    txt = "%d"%i
    if m[0]>=0:
      a = av[m[0]]
      plot( a.x    ,a.y    ,'--', color=c )
      text( a.x[-1],a.y[-1],txt , color=c )
    if m[1]>=0:
      a = bv[m[1]]
      plot( a.x    ,a.y    ,      color=c )
      text( a.x[-1],a.y[-1],txt , color=c )

def dpdemo( w, frm0, frm1 ):
  av = wvdsort(w[frm0])
  bv = wvdsort(w[frm1])
  dp = dpmatrix( av, bv )
  trb = traceback  ( dp )
  tr = traceforward( dp )
  m  = dptrace_to_merge_program( av, bv, trb.copy() )

  figure(1)
  plot_whiskers( av, color='k' )
  plot_whiskers( bv, color='r' )
  title( 'Traces' )
  #axis('image')

  figure(2);
  pcolor(dp[1:,1:],vmax=dp[-1,-1]*3);
  colorbar()
  plot( trb[:,1]+0.5, trb[:,0]+0.5 ,'w', linewidth=2 )
  plot( tr[:,1]+0.5, tr[:,0]+0.5 ,'w:', linewidth=2 )
  xlabel('Frame %d'%frm1, color='r')
  ylabel('Frame %d'%frm0, color='k')
  title('Cost matrix')
  axis('tight')

  figure(3);
  wplot_by_merge_program( av, bv, m )
  title('Merge')
  #axis('image')

###
# MODEL BUILDING
#

class MergeVoter( object ):
  def __init__(self):
    object.__init(self)
    self._tally = {} # whisker -> [trajectory -> votes]

  def update( self, data, mergeprogram ):
    """ 
    Updates the votes according to the mergeprogram using the target data.

    mergeprogram should be the merge from an alignment of the target whiskers to the models 
    """
    for i,(a,b) in enumerate(mergeprogram): 
      if b>=0:     # then insert or substitute, (b<0 --> deletion)
        obj = data[b]
        if a<0:    #insert
          # figure out who to merge before 
          # Assumes the ordering of the merge is not known, but that it is ordered
          if i == 0:
            idx = 1
          elif i == len(mergeprogram) - 1:
            idx = i-1
          else:
            idx = max( mergeprogram[i-1][0], mergeprogram[i+1][0] ) 
          self._models.insert(idx, [obj] )
        else:      #substitute
          self._models[a].append( obj )


class MergeModelBuilder( object ):
  def __init__(self):
    object.__init__(self)
    self._models = [[]]

  def update( self, data, mergeprogram ):
    """ 
    Updates the model according to the mergeprogram using the target data.

    mergeprogram should be the merge from an alignment of the target whiskers to the models 
    """
    for i,(a,b) in enumerate(mergeprogram): 
      if b>=0:     # then insert or substitute, (b<0 --> skip)
        obj = data[b]
        if a<0:    #insert
          # figure out who to merge before 
          # Assumes the ordering of the merge is not known, but that it is ordered
          if i == 0:
            idx = 1
          elif i == len(mergeprogram) - 1:
            idx = i-1
          else:
            idx = max( mergeprogram[i-1][0], mergeprogram[i+1][0] ) 
          self._models.insert(idx, [obj] )
        else:      #substitute
          self._models[a].append( obj )

class ModelBuilder( MergeModelBuilder ):
  def __init__(self, wvdict):
    MergeBuilder.__init__(self)
    av = wvdsort(wvdict)
    self._models = [ [x] for x in av ]

  def align( self, target ):
    """
    `target` should be a dict of whisker segments

    Returns the merge program.
    """
    target = wvdsort( target )
    source = [ x[-1] for x in self._models ] # unpack the latest whiskers from the model
    dp = dpmatrix( source, target )
    trb = traceback(dp)
    return dptrace_to_merge_program( source, target, trb )

  def update( self, target, mergeprogram ):
    target = wvdsort( target )
    return MergeBuilder.update(self, target, mergeprogram )

  def plot_most_recent( self, *args, **kwargs ):
    wv = [ x[-1] for x in self._models ]
    plot_whiskers( wv, *args, **kwargs )


###
# TRAJECTORY BUILDING
#

class VotingTrajectoryBuilder():
  pass
class ModelingTrajectoryBuilder( MergeModelBuilder ):
  def __init__(self, whiskers, frameid=0):
    MergeBuilder.__init__(self)
    wids = wvdsort_get_keys( whiskers[frameid] )
    self._models =  [ [(frameid, wid)] for wid in wids ]
    self._whiskers = whiskers
    self._first_fid = frameid
    self._last_fid = frameid

  def update(self, frameid, mergeprogram):
    wids = wvdsort_get_keys( self._whiskers[frameid] )
    self._last_fid = frameid
    return MergeBuilder.update(self, [(frameid,wid) for wid in wids], mergeprogram )

  def plot(self, cmap = cm.gist_rainbow):
    colors = cmap( linspace( 0, 1, len(self._models) ) )
    ioff()
    for tid, (sequence,c) in enumerate( zip(self._models,colors) ):
      txt = "%d"%tid
      for fid, wid in sequence:
        a = self._whiskers[fid][wid]
        attenuation = 1.0 - ((fid-self._first_fid) / (0.001 + (self._last_fid-self._first_fid) ))
        cc = [x*attenuation for x in c]
        plot(a.x    ,a.y        ,color=cc , alpha = attenuation)
        text(a.x[-1],a.y[-1],txt,color=cc , alpha = attenuation)
    show()
    ion() 

  def compile(self):
    t = {}
    for tid, sequence in enumerate(self._models):
      if not tid in t.keys():
        t[tid] = {}
      for fid,wid in sequence:
        if not fid in t[tid]:
          t[tid][fid] = {}
        t[tid][fid] = wid
    return t

  def save( self, filename ):
    from ui.whiskerdata import save_trajectories
    save_trajectories( filename, self.compile() )

def demo_builders( whiskers, frames ):
  mb = ModelBuilder( whiskers[frames[0]] )
  tb = TrajectoryBuilder( whiskers, frames[0] )
  for f in frames[1:]:
    print f
    m = mb.align( whiskers[f] )
    mb.update(whiskers[f], m)
    tb.update(f, m)
  tb.plot()
  return tb,mb
