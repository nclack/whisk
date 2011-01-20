"""
Author: Nathan Clack
Date  : 2009

Copyright (c) 2009 HHMI. Free downloads and distribution are allowed for any
non-profit research and educational purposes as long as proper credit is given
to the author. All other rights reserved.
"""
from numpy import zeros, hypot
from trace import Whisker_Seg
import pdb

def load():
  from ui.whiskerdata import load_whiskers, load_trajectories
  from ui.genetiff import Reader
  #movie = Reader('../../data/seq/whisker_data_0140.seq',adjuststipple=1)
  #w,wid = load_whiskers('seq.whiskers')
  #w,wid = load_whiskers('whisk-vc/whisk-vc/seq.whiskers')
  movie = Reader('../../data/JF8410_041808_001.tif',adjuststipple=1)
  w,wid = load_whiskers('test.whiskers')
  #w,wid = load_whiskers('results/seq-hand.whiskers')
  #t,tid = load_trajectories('results/seq-hand.trajectories')
  return w,movie

def merge_all( whiskers, shape, scale = 2 ):
  for fid,wvd in whiskers.iteritems():
    print fid
    wv = merge_frame( wvd, shape, scale ).whiskers()
    wvd.clear()
    wvd.update( [ e for e in enumerate(wv) ] )

def merge_frame( wvd, shape, scale = 2 ):
  table = CollisionTable( wvd, shape, scale )
  r     = Resolution(wvd.values())
  #r     = Resolution()
  
  m = table.next()
  while m:
    o = r.add(m, lambda e: len(e)<5 )
    table.update(o)
    m = table.next()

  return r

def breakout( w, bnd ):
  left,rest    = w.split(bnd[0])
  middle,right = rest.split(bnd[1]-bnd[0])
  return left, middle, right

def trace_overlap( (wa,i), (wb,j), thresh = 2.0 ):
  # assumes that indexes run along same direction (i.e. x is monitonically
  # increasing along index )
  def dist(ia,ib):
    a,b = wa[ia], wb[ib]
    return hypot( a[0] - b[0], a[1] - b[1] )
  bnda = [i,i]
  bndb = [j,j]
  ia,ib = i,j
  ms = 0  
  while ms < thresh and ia > 0 and ib > 0:
    moves = ( ( ia - 1, ib - 1 ),
              ( ia - 1, ib     ),
              ( ia    , ib - 1 ) )
    scores = [dist( iam, ibm ) for iam, ibm in moves]
    ms = min(scores)
    for idx,s in enumerate( scores ):
      if s == ms:
        ia,ib = moves[idx]
        break
  #relax at boundary, move downhill
  if ia == 0 and ib == 0: 
    pass
  elif ia == 0:
    last = ms
    s = dist( ia, ib - 1 )
    while s < last and ib > 1:
      ib -= 1
      last = s
      s = dist( ia, ib - 1 )
  elif ib == 0:
    last = ms
    s = dist( ia - 1, ib )
    while s < last and ia > 1:
      ia -= 1
      last = s
      s = dist( ia - 1, ib )

  bnda[0] = ia
  bndb[0] = ib

  ia,ib = i,j
  ms = 0  
  while  ms < thresh and ia < len(wa)-1 and ib < len(wb)-1:
    moves = ( ( ia + 1, ib + 1 ),
              ( ia + 1, ib     ),
              ( ia    , ib + 1 ) )
    scores = [dist( iam, ibm ) for iam, ibm in moves]
    ms = min(scores)
    for idx,s in enumerate(scores):
      if s == ms:
        ia,ib = moves[idx]
        break
  #relax at boundary, move downhill
  if ia == len(wa)-1 and ib == len(wb)-1: 
    pass
  elif ia == len(wa)-1:
    last = ms
    s = dist( ia, ib + 1 )
    while s < last and ib < len(wb)-2:
      ib += 1
      last = s
      s = dist( ia, ib + 1 )
  elif ib == len(wb)-1:
    last = ms
    s = dist( ia + 1, ib )
    while s < last and ia < len(wa)-2:
      ia += 1
      last = s
      s = dist( ia + 1, ib )

  bnda[1] = ia
  bndb[1] = ib
  return bnda, bndb

class Resolution(object):
  def __init__(self, whiskers = None, shape = None, scale = 2):
    self._paths = set()
    self._index = {}
    if whiskers:
      for w in whiskers:
        self._paths.add( (w,) )
        self._index[w] = set([(w,)])

  def __len__(self):
    return len(self._paths)
  
  def iterwhiskers(self):
    """ Iterates over paths, yielding the whisker for each path """
    join = lambda s,t: Whisker_Seg.join(s,t)
    for p in self._paths:
      yield reduce( join, p )

  def whiskers(self):
    return list(self.iterwhiskers())
    
  def add(self, conflicts, prune):
    """
    `conflicts` is a match returned from CollisionTable.next()  like: ( (whisker,hit), (whisker,hit) )
    `prune` is a function mapping whisker segments to a boolean.  Will prune if true.

    Returns a dict that maps whiskers to corresponding left and right subsegments.
    """
    a,b = conflicts
    paths,ownership = self.compute_pairwise_conflict_paths( a, b, prune )
    paths = set( [self._merge_middles(p) for p in paths] )
    pdb.set_trace()
    self.update( paths, a[0], ownership)
    self.update( paths, b[0], ownership)
    return ownership

  def update( self, newpaths, owner, stubs ):
    if 0:
      from pylab import plot, cla, show
      from tests import plot_whiskers
      cla()
      plot(owner.x,owner.y,'k',linewidth=3)
      plot_whiskers([e for e in p if e],marker='x')
      show()
    issame = lambda a,b: (a is not None) and (a==b) # true implies (b is not None) 
    br = stubs[owner]                     
    if br:
      didmerge = False
      l,m,r = br                               # Filter paths to make sure only new paths   
      for p in newpaths:
        if (not (l or r)) or issame(l,p[0]) or issame(r,p[-1]):  #   corresponding to `owner` are substituted
          self._merge( owner, p )
          didmerge = True
      if not didmerge:             # There was a break
        self._merge( owner, None ) # But there was no parent path found that sourced the break...

  def _merge( self, owner, newp ):
    def substitute( op, a, np ):
      def _iter():
        for i,e in enumerate(op):
          if e == a:
            break
          yield e
        for e in np:
          if e: 
            yield e
        for e in op[i+1:]:
          yield e
      return tuple([e for e in _iter()])
      
    olds = self._index.get(owner)  # fetch paths to update.
    if olds:
      if newp:
        news = set([ substitute( e, owner, newp ) for e in olds ]) # make new paths
        self._paths.update(news)     # add new paths
        for e in newp:
          self._index[e] = news      # add mappings to new paths
        return 1
      else:
        for p in olds:               # remove old paths
          self._paths.discard(p)
        del self._index[owner]       # remove mapping to old paths
    return 0

  @staticmethod
  def _merge_middles( path ):
    if len(path)==4:
      l,m1,m2,r = path
      lm1,rm1 = m1.split( len(m1)/2 )
      lm2,rm2 = m2.split( len(m2)/2 )
      m = Whisker_Seg.join( lm1,rm2 ) 
      return (l,m,r)
    else:
      return path

  @staticmethod
  def compute_pairwise_conflict_paths( a, b, prune ):
    """ 
    a and b are each tuples of (whisker, hitindex), as returned from CollisionTable.next()  

    Returns a set of tuples.
    """
    bnda,bndb = trace_overlap(a,b)
    la,ma,ra = breakout(a[0],bnda)
    lb,mb,rb = breakout(b[0],bndb)
    ownership = {a[0]:(la,ma,ra), b[0]:(lb,mb,rb)}
    if not ma or not mb or prune(ma) or prune(mb):
      ownership = {a[0]:None, b[0]:None}
      pset = set()
      return pset, ownership

    pset =     [  [ la, ma,     ra ], 
                  [ la, ma, mb, rb ], 
                  [ lb, mb, ma, ra ], 
                  [ lb, mb,     rb ] ] 
    #prune
    for p in pset:
      for i,w in enumerate(p):
        if w and prune(w):
          p[i] = None

    #transform to set of tuples
    pset = set( [tuple(e) for e in pset] ) 

    #0. Path's must have legititmate middle nodes.
    for p in list(pset):
      if len(p)==3 and p[1] is None:
        pset.remove(p)
      elif len(p)==4 and ( p[1] is None or p[2] is None ):
        pset.remove(p)

    #reduction
    hasfullpath = False
    # 1. Remove if can't get from left to right
    for p in list(pset):
      if p[0] is None and p[-1] is None:
        pset.discard(p)
      if p[0] is not None and p[-1] is not None:
        hasfullpath = True

    # 2. if only overlap, return composite of overlap 
    if len(pset)==0: #no endpoints - this happens for middle-only overlaps
      m = max( (ma,mb), key = lambda w: w.scores.mean() )
      ownership = {a[0]:(None,m,None), b[0]:(None,m,None)}
      #ownership = {a[0]:None, b[0]:None}
      pset = set( ((None,m,None),) ) #return best scoring
      return pset, ownership

    # 3. if there is at least one path from left to right, remove paths that
    #    don't
    if hasfullpath:
      for p in list(pset):
        if p[0] is None or p[-1] is None:
          pset.discard(p)
    else:
      # 4. if not, path's should not begin or end on merged middles
      for p in list(pset):
        if len(p)==4 and ( p[0] is None or p[-1] is None ):
          pset.discard(p)
          
    if 0:
      from tests import plot_whiskers
      from pylab import cm, clf, show
      cmap = lambda i: cm.spectral( i/float(len(pset)) )
      for i,p in enumerate(pset):
        plot_whiskers([e for e in p if e], color = cmap(i))
      show()

    return pset,ownership

  def plot(self, index = None):
    from tests import plot_whiskers
    from pylab import cm
    cmap = lambda i: cm.spectral( i/float(len(self._paths)) )
    if index is None:
      for i,p in enumerate(self._paths):
        plot_whiskers( [e for e in p if e], color = cmap(i) )
    else:
      p = list(self._paths)[index]
      plot_whiskers( [e for e in p if e], color = cmap(index) )

class CollisionTable(object):
  def __init__(self, wvd, shape, scale):
    """ `wvd` may be either a dict or list of whiskers """
    object.__init__(self)
    self._map = {}
    self._shape  = shape
    self._scale  = scale
    self._stride = stride = shape[1]/scale
    self.topx   = lambda p: int(p[0]/scale) + stride * int(p[1]/scale)
    self._build_inverse_table( wvd )
  
  def _build_inverse_table(self,  wvd ):
    hash = lambda w: enumerate( map(self.topx,zip(w.x,w.y)) )
    g = enumerate(wvd)
    if isinstance(wvd, dict):
      g = wvd.iteritems()
    for i,w in wvd.iteritems():
      for j,idx in hash(w):
        self._map.setdefault(idx,set()).add((w,j))

  def update( self, changes ):
    """ Changes is a dict mapping old whisker segments to new segments """
    last = None
    for w,p in changes.iteritems():
      self.remove(w)
      if p:
        self.add(p[0]) # add back ends
        self.add(p[-1])
        last = p[1]
    if last:
      self.add(last) # add back last middle 

  def add(self, w):
    if not w: return
    hash = lambda e: enumerate( map(self.topx,zip(e.x,e.y)) )
    for i,px in hash(w):
      self._map.setdefault(px,set()).add( (w,i) )

  def remove(self, w):
    if not w: return
    hash = lambda e: enumerate( map(self.topx,zip(e.x,e.y)) )
    for i,px in hash(w):
      s = self._map.get(px)
      if s:
        s.discard( (w,i) )
    
  def next(self):
    """ This changes the inverse table by removing hits.

    Returns a (Whisker_Seg, index),(Whisker_Seg, index)  tuple
            or None, if done.
    """
    todelete = []
    retval = None
    for px,s in self._map.iteritems():
      if len(s) > 1:
        retval = s.pop(),s.pop()
        if retval[0][0] == retval[1][0]: # if it the same whisker hits this pixel twice
          s.add( retval[1] )             #    add one of the hits back, remove the other one
          #print retval, s
          retval = None
          continue
        else:
          break
      else:
        todelete.append(px) # get rid of references to empty pixels

    for k in todelete:
      del self._map[k]

    return retval
  
  def counts( self ):
    tosc = lambda e: e/self._scale
    im = zeros(map(tosc, self._shape))
    imr = im.ravel()
    for px,s in self._map.iteritems():
      imr[px] = len(set( [e for e,i in s] ))
    return im

def test1():
  w,movie = load()
  wvd = w[ 14].copy()
  r = merge_frame(wvd, movie[0].shape)
  r.plot()
  return w,movie,r

def test2():
  from ui.whiskerdata import save_whiskers
  w,movie = load()
  merge_all(w,movie[0].shape)
  save_whiskers('merge.whiskers',w)
  return w,movie

if __name__ == '__main__':
  w,movie,r = test1()
  #w,movie = test2()
