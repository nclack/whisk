import features
from numpy import zeros,array,histogram,linspace, float32, float64, log2

class EmmissionDistributions(object):
  """

  Emmission distributions

  1. Project a whisker segment to a feature vector

  2. Estimate P(segment|class) using known whiskers and trajectories

  3. Define a function to evaluate P(segment|class).

  Two classes are involved in this model:

    Junk
      Whisker segments not involved in a trajectory

    Whiskers
      Whisker segments involved in some trajectory

  """
  def __init__(self, wvd=None, traj=None):
    object.__init__(self)
    self._features = (( "Length(px)"               , features.integrate_path_length ), # Each function maps
                      ( "Median score"             , features.median_score          ), #   whisker segment --> real
                      ( "Angle at folicle (deg)"   , features.root_angle_deg        ),
                      ( "Mean curvature (1/px)"    , features.mean_curvature        ),
                      ( "Follicle x position (px)" , features.follicle_x            ),
                      ( "Follicle y position (px)" , features.follicle_y            ))
    #bins for feature histograms
    self._feature_bin_deltas = zeros( len(self._features) )
    self._feature_bin_mins   = zeros( len(self._features) )

    self._distributions  =  {"whisker":[], "junk": []} # stored as log2( probability density )
    if not (wvd is None or traj is None):
      self.estimate(wvd,traj)

  def feature(self, seg ):
    return array([ f(seg) for name,f in self._features ])  
  
  @staticmethod
  def _count_whiskers(wvd):
    nrows = 0
    for fid,v in wvd.iteritems():
      nrows += len(v)
    return nrows

  def _all_features(self, wvd, traj):
    data = zeros( ( self._count_whiskers(wvd), len(self._features)+3 ) ) #the extra 3 cols are for class id, fid, wid
    def iter():
      for fid,v in wvd.iteritems():
        for wid,w in v.iteritems():
          yield fid,wid,self.feature(w)
    def itertraj():
      for tid,v in traj.iteritems():
        for fid,wid in v.iteritems():
          yield fid,wid
    labelled = set( list(itertraj()) )
    for i,(fid,wid,fv) in enumerate(iter()):
      data[i,0] = (fid,wid) in labelled  
      data[i,1] = fid
      data[i,2] = wid
      data[i,3:] = fv
    return data

  def estimate(self, wvd, traj):
    data = self._all_features(wvd,traj)
    mask = data[:,0]==0
    nfeat = len(self._features)

    for i in xrange(nfeat):
      ic = 3+i
      v = data[:,ic]     # Determine bins to compute histograms over for each feature
      avg = v.mean()     # Bounds are limited to 3 times the standard deviation 
      wid = 3.0*v.std()  #  or the max/min; whichever gives the smaller interval.
      mn  = v.min()      #
      mx  = v.max()      #
      b = linspace( max( avg-wid, mn ), min(avg+wid,mx), 32 )

      counts,b = histogram( v[mask], bins   = b )
      counts = counts.astype(float64) + 1.0 #add one to each bin...gets rid of zeros
      counts /= counts.sum()
      self._feature_bin_deltas[i] = b[1] - b[0]
      self._feature_bin_mins[i]   = b[0]
      self._distributions["junk"].append( log2(counts) ) # all counts>0
      
      counts,b = histogram( v[~mask], bins   = b )
      counts = counts.astype(float64) + 1.0 #add one to each bin...gets rid of zeros
      counts /= counts.sum()
      self._distributions["whisker"].append( log2(counts) ) # all counts>0
    
  def _discritize(self, fv):
    return array( [ floor((val-bmin)/db) for val,db,bmin in zip(fv, \
                                                                self._feature_bin_deltas, \
                                                                self._feature_bin_mins) ] )

  def evaluate(self, seg, state ):
    data = self.feature(seg)
    idx = self._discritize( data )
    logp = [ self._distributions[state][i][j] for i,j in enumerate(idx) ]
    return sum(logp)

class Model(object):
  def __init__(self, nwhiskers):
    object.__init__(self)
    def iterstates(n):
      yield "start"
      for i in xrange(n):
        yield "junk%d"%i
        yield "whisker%d"%i
      yield "end"
    self._nwhiskers = nwhiskers
    self.states = list(iterstates(nwhiskers))
    self._transitions = {} # map source -> ( map destination -> log2 prob )

  @staticmethod
  def _itertrajinv(traj):
    for tid,v in traj.iteritems():
      for fid,wid in v.iteritems():
        yield (fid,wid),tid

  def train_time_independent(wvd,traj):
    T = zeros( (4,4), astype=float64 ) #four states start, whisker, junk, end (0,1,2,3 respectively)
    invtraj = dict( [p for p in self._itertrajinv(traj) ] )
    classify = lambda fid,wid: 1 if invtraj.has((fid,wid)) else 2 
    for fid,wv in wvd.iteritems():
      state = 0
      for wid in wv.iterkeys():
        next = classify(fid,wid)
        T[state, next] += 1
        state = next
      T[state,3] += 1

    #normalize to make T a stochastic matrix
    for row in T:
      row /= row.sum()

    #
    # map to the "real" model
    #
    names = {0:"start",1:"whisker%d",2:"junk%d",3:"end"}
    def map_state(state,time):
      n = names[state]
      if state in (1,2):
        n = n%time
      return n
    # start
    for i,logp in T[0]:                #src                  #dst
      self._transitions.setdefault( map_state(0,-1), {} )[map_state(i,0)] = logp
    #middle to end
    for i,row in enumerate(T[1:-1]): # no start as source state or end state
      src = i+1
      for dst,logp in enumerate(row):
        for time in xrange( self._nwhiskers ):
          self._transitions.setdefault( map_state(src,time), {} )[map_state(dst,time)] = logp


  def train_time_dependent(wvd,traj):
    pass

