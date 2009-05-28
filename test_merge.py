"""
A central assmption here is that the sampling points in each segment are consistently ordered.
For example, that 'x' position is monotonically increasing with index (w.x[i] <= w.x[j] iff i<=j).
The particular nature of the ordering isn't relevant, only that it's consistent across whiskers
in a single frame.

One symptom that this is violated is that, on any given frame, many small overlaps will be detected.
Overlaps that should be merged will be missed.
"""
from ui.genetiff import Reader
from numpy import hypot, zeros, vander, array, matrix, matrix, diff, linspace
from numpy import polyder, polymul, polyval, polyadd, polysub, roots
from numpy import float32, sqrt, fabs, arange, real, isreal
from numpy import concatenate, isfinite, mean
import scipy.linalg
from pylab import imshow
import pdb
from trace import Whisker_Seg
import sys
#from pylab import *

def test1():
  w,movie = load()
  shape = movie[0].shape
  
  wvd = w[4500].copy()
  
  scale = 2
  table = build_inverse_table( wvd, shape, scale )
  op = lambda match: wvd[match[0]]
  intsize = lambda interval: interval[1] - interval[0]
  match = get_next_match( table )
  
  ma,mb = match
  bnda, bndb = trace_overlap( (op(ma),ma[1]), (op(mb),mb[1]) )
  
  resolution = build_resolution( op(ma), op(mb), bnda, bndb )
  resolution.prune( lambda e: len(e)<5 )
  g0 = resolution
  resolve( wvd   , table, shape, scale, ma, mb, resolution )
  
  match = get_next_match( table )
  
  ma,mb = match
  bnda, bndb = trace_overlap( (op(ma),ma[1]), (op(mb),mb[1]) )
  
  resolution = build_resolution( op(ma), op(mb), bnda, bndb )
  resolution.prune( lambda e: len(e)<5 )
  g1 = resolution
  wa = op(ma)
  wb = op(mb)
  resolve( wvd   , table, shape, scale, ma, mb, resolution )
  
  g = g1.resolve( g0, wa )
  g = g1.resolve( g,  wb )
  
  match = get_next_match( table )
  
  ma,mb = match
  bnda, bndb = trace_overlap( (op(ma),ma[1]), (op(mb),mb[1]) )
  resolution = build_resolution( op(ma), op(mb), bnda, bndb )
  resolution.prune( lambda e: len(e)<5 )
  g1 = resolution
  wa = op(ma)
  wb = op(mb)
  resolve( wvd   , table, shape, scale, ma, mb, resolution )
  
  g = g1.resolve( g, wa )
  g = g1.resolve( g, wb )
  match = get_next_match( table )
  
  return w,movie

def merge_all( whiskers, shape ):
  for fid, wvd in whiskers.iteritems():
    print fid
    merge_all_in_frame( wvd, shape )

def merge_all_in_frame(wvd, shape):
  scale = 2
  table = build_inverse_table( wvd, shape, scale )

  op = lambda match: wvd[match[0]]
  intsize = lambda interval: interval[1] - interval[0]

  match = get_next_match( table )
  g = WhiskerGroup()
  while match:
 
    ma,mb = match
    wa,wb = op(ma), op(mb)
    bnda, bndb = trace_overlap( (op(ma),ma[1]), (op(mb),mb[1]) )
    resolution = build_resolution( op(ma), op(mb), bnda, bndb )
    resolution.prune( lambda e: len(e)<5 )
    resolve( wvd   , table, shape, scale, ma, mb, resolution )
 
    g = resolution.resolve( g, wa )
    g = resolution.resolve( g, wb )
    match = get_next_match( table )

#    g.plot(hold=0)
#    pdb.set_trace()
  return g
 
#   ma,mb = match
#   bnda, bndb = trace_overlap( (op(ma),ma[1]), (op(mb),mb[1]) )
#   print "%5d: %3d/%3d\t%3d: %3d/%3d"%( ma[0],intsize(bnda), len(op(ma)), mb[0], intsize(bndb), len(op(mb)) ),
#   if intsize(bnda) >= 5 and intsize(bndb) >= 5:
#     print "\t*"
#     resolution = build_resolution( op(ma), op(mb), bnda, bndb )
#     resolution.prune( lambda e: len(e)<5 )
#     resolve( wvd   , table, shape, scale, ma, mb, resolution )
#   else:
#     print ""
#     update_inverse_table( table, op(ma), ma[1], None, shape, scale, bnd = bnda )
#     update_inverse_table( table, op(mb), mb[1], None, shape, scale, bnd = bndb )
#   match = get_next_match( table )

class WhiskerGroup(object):
  """
  WhiskerGroup
  ------------
  
  A putative whisker made of a collection of segments.
  The collection is a directed graph of segments.
  The graph may be disconnected.          
  
  The need for this collection arises from degeneracies which occur during tracing that result when whiskers overlap.
  These can be difficult to resolve a priori, so this representation is used to postpone decision making until more 
  information about whisker structure is available.  For example, information from other time points in the movie.
  """
  def __init__(self, *paths ):
    """ 
    Usage: 
      wg = WhiskerGroup( (la,ma,ra), (lb,mb,rb), (la,m,ra), (ra,m,lb) )
      
      Makes: A directed graph (arrows point from left to right) 
              la-ma-ra
                \  /
                 m
                /  \
              lb-mb-rb                      
    """
    object.__init__(self)
    self._graph = {}
    
    for p in paths:
      # init source node
      for a in p:
        if a:
          self._graph[a] = []
      #for a in p[:-1]:        
      #  if not (a is None or a in self._graph):
      #    self._graph[a] = []

    # make connections
    for p in paths:
      for a,b in zip(p[:-1],p[1:]):
        if a and b:
          self._graph[a].append(b)
          
  def plot(self, *args, **kwargs):
    from pylab import plot, ioff, ion, show, ishold, cla
    ioff()
    if kwargs.get('hold')==0 or not ishold():
      cla()
    kwargs['hold']=1
    for i,w in enumerate(self.iterwhiskers()):
      plot( w.x + 0.1*i, w.y + 0.1*i , *args, **kwargs)
    ion()
    show()
  
  def asdot(self, file = sys.stdout, name = 'G'):
    names = {}
    for i,v in enumerate(self.domain()):
      names[v] = str(id(v)) #str(i) 
    
    print >> file, "digraph %s {"%name
    for k in self._graph.iterkeys():
      print >> file, "\t%s;"%(names[k])
    for k,v in self._graph.iteritems():
      for e in v:
        print >> file, "\t%s -> %s;"%(names[k],names[e])
    print >> file, "}"
    
  def preimage(self,a):
    """ Get preimage of a in graph """
    preimage = []
    for k,v in self._graph.iteritems():
      if a in v:
        preimage.append(k)
    return preimage
  
  def prune(self,condition):
    """
    Remove nodes for which condition is true.
    
    >>> condition = lambda e: len(e)<minlen  # prune if this returns true
    >>> g.prune(condition)
    """      
    for k,v in self._graph.items():
      if condition(k):
        del self._graph[k]
      else:    
        for c in [e for e in v if condition(e)]:
          v.remove(c)  
           
    # remove empty keys
    #for e in [k for k,v in self._graph.iteritems() if len(v)==0]:
    #  del self._graph[e] 
   
  def join(self, a, other, b):
    """ 
    Union the two graphs.
    If a, splice b at a and remove a.     
    Returns self.
    """
    # 1. union graphs    # if self contains other, this is an identity op on self
    for k,v in other._graph.iteritems():
      if k in self._graph:
        t = set(self._graph[k])
        t.update(v)
        self._graph[k] = list(t)
      else:
        self._graph[k] = v

    # 2. move connections from a to b, splice out a
    if a is not None and b is not None:
      preimage = self.preimage(a)
      for e in self._graph[a]:    # make children of a, children of b
        self._graph[b].append(e)
      for e in preimage:          
        self._graph[e].append(b)  # make parents of a, parents of b
        self._graph[e].remove(a)  # disconnect a from parents of a
      del self._graph[a] # delete a from graph
    return self
  
  def domain(self):
    s = set(self._graph.keys())
    #for k,v in self._graph.iteritems():
    #  for e in v:
    #    s.add(e)
    return s
  
  def sources(self):
    return set( [k for k,v in self._graph.iteritems() if len(v)] )
  
  def sinks(self):
    def itersinks():
      for v in self._graph.itervalues():
        for e in v:
          yield e
    return set([e for e in itersinks()])
    
  def isleaf(self, node ):
    if node in self._graph:
      return len(self._graph[node])==0
    else:
      return True

  def iterpaths(self):
    # 1. find nodes with only outgoing edges
    roots = self.domain() - self.sinks()
    # 2. enumerate paths by depth first search starting from each root
    def dfs( node, path = []):
      """ This depth first search performs a specific task: yielding traversed paths """
      path.append(node)      
      if self.isleaf(node):        
        yield path
      else:
        for c in self._graph[node]:
          for p in dfs(c,list(path)):
            yield p
    
    #3. yield
    for r in roots:
      for p in dfs(r,path=[]):              
        yield p 

  def paths(self):
    return list(self.iterpaths())
        
  def iterwhiskers(self):
    """ Iterates over paths through the WhiskerGroup graph, yielding the whisker for each path """
    join = lambda s,t: Whisker_Seg.join(s,t)
    for p in self.iterpaths():
      yield reduce( join, p )

  def whiskers(self):
    return list(self.iterwhiskers())
  
  def __len__(self):
    return len( self.domain() )
    
class ResolutionGroup(WhiskerGroup):
  """ 
  This is a `WhiskerGroup` that arises from an overlap of two whiskers, `wa`
  and `wb`.

  Because a `ResolutionGroup` corresponds to a specific pair-wise overlap,
  there are some restrictions on the group's structure relative to the more
  general `WhiskerGroup`.  For instance the nodes are named according to 
  how they were generated.

  Some rules (besides the names of the nodes):
  1. The (aggregated) middle node should be present iff left nodes are present.
     The (aggregated) middle can not start a path.
  2. The (aggregated) middle can not end a path (must have children)
  3. If there is only one path from the left side to the right side, there should be
     no other paths (that path is the correct merge).
  4. Only include the aggregated middle if there is a crossover path.
  """
  def __init__(self, wa, wb, bnda, bndb ):
    lenbnd = lambda b: b[1] - b[0]
    if lenbnd(bnda) < 2 or lenbnd(bndb) < 2:
      # overlap is too short
      self.middle = None
      self.mainnodes = []
      self.breaks = { }
      WhiskerGroup.__init__( self, (wa,), (wb,) )
    else:
      la,ma,ra = breakout( wa, bnda )
      lb,mb,rb = breakout( wb, bndb )
      m = aggregate_pair(ma,mb) # want the same test point for decision
      
      # Rules 1-3
      if (la or lb) and (ra or rb): # there exists a crossover path
        self.middle = m
        if not la and not rb:
          WhiskerGroup.__init__(self, (lb,m,ra) )
        elif not lb and not ra:
          WhiskerGroup.__init__(self, (la,m,rb) )
        else: # at least two paths from left to right
          WhiskerGroup.__init__(self, (la,ma,ra), (lb,mb,rb), (la,m,rb), (lb,m,ra) )
      else: # no crossover path
        if not ( la or lb):
          WhiskerGroup.__init__(self, (wa)) # XXX
        WhiskerGroup.__init__(self, (la,ma,ra), (lb,mb,rb) )
        self.middle    = None

      self.breaks    = {wa:[la,ma,ra], wb:[lb,mb,rb]}
      self.mainnodes = [e for e in [la,ra,lb,rb,m] if e]
  
  def prune(self, condition):
    if self.middle and condition(self.middle): # the middle is to be pruned
      #interpret this as a case where the overlap is too (something) to justify the confusion
      #...so...undo the breaks
      #     ...update the mainnodes so only `valid` subportions of the whisker
      #        are used to update the table
      self._graph = {}
      for k in self.breaks.iterkeys():
        self._graph[k] = [] 
      self.middle = None
      self.mainnodes = [ e for e in self.mainnodes if not condition(e) ] 
    else:
      for e in [k for k in self.mainnodes if condition(k)]:
        self.mainnodes.remove(e)
      for k,v in self.breaks.iteritems():
        for e in [s for s in v if s and condition(s)]:
          v.remove(e)
      WhiskerGroup.prune(self, condition)

      # Maintain constraints on graph
      # check aggregated middle
      if self.middle:
        if len(self.preimage( self.middle )) == 0: #remove if no parents
          del self._graph[self.middle]
          self.middle = None
      if self.middle:
        if len(self._graph[ self.middle ]) == 0:   #remove if no children
          for e in self.preimage(self.middle):
            self._graph[e].remove( self.middle )
          del self._graph[self.middle]             
          self.middle = None
     

  def resolve(self, g, w ):
    """ Merge self with g at w. """
    if w in set( g.domain() ):
      print "involved"
      if w in self.breaks:
        print "in the breaks"
        tails = g.sinks()   - g.sources()
        roots = g.sources() - g.sinks()
        if w in roots:
          print "\troot"
          g.join( w, self, self.breaks[w][-1] ) # join tail to root
        elif w in tails:
          print "\ttail"
          g.join( w, self, self.breaks[w][0] ) # join root to tail
        else: #middle
          print "\tmiddle"
          g.join( w, self, self.middle )
        return g
    # didn't satisfy above, so...
    g.join(None,self,None) #union graphs, but leave unconnected
    return g

def load():
  from ui.whiskerdata import load_whiskers, load_trajectories
  from ui.genetiff import Reader
  movie = Reader('../../data/seq/whisker_data_0140.seq',adjuststipple=1)
  w,wid = load_whiskers('seq.whiskers')
  #w,wid = load_whiskers('whisk-vc/whisk-vc/seq.whiskers')
  #movie = Reader('../../data/JF8410_041808_001.tif',adjuststipple=1)
  #w,wid = load_whiskers('test.whiskers')
  #w,wid = load_whiskers('results/seq-hand.whiskers')
  #t,tid = load_trajectories('results/seq-hand.trajectories')
  return w,movie


def plot_overlap( wvd, ma, mb, bnda, bndb ):
  from pylab import plot
  op = lambda match: wvd[match[0]]
  wa = op(ma)
  wb = op(mb)
  #plot(wa.x,wa.y)
  #plot(wb.x,wb.y)
  plot(wa.x[bnda[0]:bnda[1]],wa.y[bnda[0]:bnda[1]],'x-')
  plot(wb.x[bndb[0]:bndb[1]],wb.y[bndb[0]:bndb[1]],'x-')

def compute_join_length( px, py, tlow = 0.0, thigh = 1.0 ):
  from scipy.integrate import quad
  xp  = polyder( px, 1 )
  yp  = polyder( py, 1 )
  xp2 = polymul( xp, xp )
  yp2 = polymul( yp, yp )
  p   = polyadd( xp2, yp2 )
  integrand = lambda t: sqrt( polyval( p, t ) )
  return quad(integrand, tlow, thigh) [0]

def compute_join_curvature( px, py ):
  from scipy.integrate import quad
  xp  = polyder( px, 1 )
  xpp = polyder( px, 2 )
  yp  = polyder( py, 1 )
  ypp = polyder( py, 2 )
  pn = polyadd( polymul( xp, ypp ), polymul( yp, xpp )) #numerator
  pd = polyadd( polymul( xp, xp ) , polymul( yp, yp ) ) #denominator
  integrand = lambda t:  fabs(polyval( pn, t )/( polyval( pd, t )**(1.5)) ) 
  return quad(integrand, 0, 1) [0]

def solve_polynomial_join( left, right):
  # method:       parametric cubic matching position and slope of endpoints.
  #               This ends up being cheaper to compute, since the matrix is
  #               known (interval of parameter is always 0 to 1) and so the 
  #               inverse can be precomputed. 
  #    minv is inverse of m, where:
  #         m = array( [  [   a**3,   a**2, a, 1 ],
  #                       [   b**3,   b**2, b, 1 ],  
  #                       [ 3*a**2, 2*a   , 1, 0 ],
  #                       [ 3*b**2, 2*b   , 1, 0 ] ] )
  #    is the matrix for the linear system:
  #         m * coeff = v,
  #    with v = [ x(0) x(1) dx/dt(0) dx/dt(1) ].
  #    Here a = 0 and b = 1 so m and it's inverse is always the same.
  #
  minv = matrix( [[ 2., -2.,  1.,  1.],
                  [-3.,  3., -2., -1.],
                  [ 0.,  0.,  1.,  0.],
                  [ 1.,  0.,  0.,  0.]])
  #take care of cases joining very short segements
  lr = len(right)
  ll = len(left)
  #L = length( right.x, right.y ) + length( left.x, left.y )
  dd = hypot( left.x[0] - right.x[-1], left.y[0] - right.y[-1] )
  nl = ll/4
  nr = lr/4
  slope = lambda v: v[ 0] - v[-1]     # want the total change over the length
  length = lambda x,y: hypot(diff(x),diff(y)).sum()
  if nr < 2 and nl < 2:        
    lnorm = length(  left.x       ,  left.y        ) 
    rnorm = length( right.x       , right.y        ) 
    dly = -diff( left.y ).mean() / lnorm
    dlx = -diff( left.x ).mean() / lnorm
    dry = -diff(right.y ).mean() / rnorm
    drx = -diff(right.x ).mean() / rnorm
    nl = 0
    nr = lr - 1
  elif nr < 2:                        # use the derivative on the other side
    lnorm = length(  left.x[:2*nl],  left.y[:2*nl] ) 
    rnorm = length( right.x       , right.y        ) 
    dly =   slope( left.y[(-2*nl):]  ) / lnorm
    dlx =   slope( left.x[(-2*nl):]  ) / lnorm
    dry = -diff(right.y ).mean() / rnorm
    drx = -diff(right.x ).mean() / rnorm
    nr = lr - 1
    #print dly,dlx,dry,drx
  elif nl < 2:                        # use the derivative on the other side
    rnorm = length( right.x[:2*nr], right.y[:2*nr] ) 
    lnorm = length(  left.x       ,  left.y        ) 
    dry =   slope(right.y[:2*nr]  ) / rnorm
    drx =   slope(right.x[:2*nr]  ) / rnorm
    dly = -diff( left.y ).mean() / lnorm
    dlx = -diff( left.x ).mean() / lnorm
    nl = 0
  else:                               # the "normal" case
    rnorm = length( right.x[:2*nr], right.y[:2*nr] ) 
    lnorm = length(  left.x[:2*nl],  left.y[:2*nl] ) 
    dry =   slope(right.y[:2*nr]  ) / rnorm
    drx =   slope(right.x[:2*nr]  ) / rnorm
    dly =   slope( left.y[(-2*nl):]  ) / lnorm
    dlx =   slope( left.x[(-2*nl):]  ) / lnorm
  rnorm = hypot( left.x[0] - right.x[0], left.y[0] - right.y[0] )
  lnorm = hypot( left.x[-1]- right.x[0], left.y[-1]- right.y[0] )
  if not isfinite(dlx): dlx =(left.x[0]  - right.x[0])/lnorm
  if not isfinite(dly): dly =(left.y[0]  - right.y[0])/lnorm
  if not isfinite(drx): drx =(left.x[-1] - right.x[0])/rnorm
  if not isfinite(dry): dry =(left.y[-1] - right.y[0])/rnorm
   
  ry =  right.y[nr] 
  ly =  left.y[-nl]
  rx =  right.x[nr]
  lx =  left.x[-nl]
  L = hypot( rx-lx, ry-ly )
  print "L:%g"%L
  yv = matrix(   [[  ry                          ],   
                  [  ly                          ],   
                  [  dry * L                     ],   
                  [  dly * L                     ]])
  xv = matrix(   [[  rx                          ],                
                  [  lx                          ],                
                  [  drx * L                     ],  
                  [  dlx * L                     ]])
  cx = minv*xv
  cy = minv*yv
  
  if not (isfinite(cx).any() and isfinite(cy).any()):
    pdb.set_trace()
  return map( lambda t: array(t).squeeze() , (cx,cy) )

def plot_join(px,py,*args,**kwargs):
  from pylab import plot, polyval
  tt = linspace(0,1,50)
  plot( polyval(px,tt), polyval(py,tt), *args, **kwargs )

def solve_for_nearest( px,py,rx,ry ):
  dpx = polyder(px)
  dpy = polyder(py)
  cp = polymul( dpx, px ) + polymul( dpy, py )
  cp = polyadd( cp, -rx*dpx )      
  cp = polyadd( cp, -ry*dpy )
  t = roots(cp)
  t = real(t[isreal(t)])
  t = t[ (t>=0) * (t<=1) ]

  ##tt = linspace(0,1,100)
  ##from pylab import plot
  ##plot( polyval(px,tt), polyval(py,tt), 'k', hold = 0 )
  ##plot( [rx],[ry], 'r.' )
  ##plot( polyval(px,t[isreal(t)*(real(t)>=0)*(real(t)<=1)]),
  ##      polyval(py,t[isreal(t)*(real(t)>=0)*(real(t)<=1)]), 'o' )
  ##pdb.set_trace()

  if len(t):
    if len(t) == 1:
      return t[0]
    else:
      ux = polyval( px, t )
      uy = polyval( py, t )
      d = hypot( ux - rx, uy - ry )
      return t[ d==d.min() ][0]
  else:
    t = array([0.0,1.0])
    ux = polyval( px, t )
    uy = polyval( py, t )
    d = hypot( ux - rx, uy - ry )
    if d[0] < d[1]:
      return 0.0
    else:
      return 1.0

def compute_join_err( left, right, polynomial = None ):
  if not polynomial:
    polynomial = solve_polynomial_join( left, right  )
  px,py = polynomial

  il = arange( len(left) - len(left)/4, len(left ) )
  ir = arange( 0, len(right )/4 )
  if len(left)/4 < 2:
    il = arange(len(left))
  if len(right)/4 < 2:
    ir = arange(len(right))

  utr = [ solve_for_nearest( px, py,  x,  y ) for  x, y in zip( right.x[ir], right.y[ir] ) ]
  utl = [ solve_for_nearest( px, py,  x,  y ) for  x, y in zip(  left.x[il],  left.y[il] ) ]

  dxr = polyval( px, utr ) - right.x[ir]
  dyr = polyval( py, utr ) - right.y[ir]
  dxl = polyval( px, utl ) -  left.x[il]
  dyl = polyval( py, utl ) -  left.y[il]
  
  #d = hypot( dxr, dyr ).sum() + hypot( dxl, dyl ).sum()

  if 0:
    from pylab import plot, ion, ioff, show
    ioff()
    plot(right.x, right.y,'k.-', linewidth=0.5)
    plot( left.x,  left.y,'k.-', linewidth=0.5)
    #plot( polyval( px, tt[ir-ir[0]] ), polyval( py, tt[ir-ir[0]] ) , 'gx-', linewidth=2 )
    #plot( polyval( px, tt[len(ir)+il] ), polyval( py, tt[len(ir)+il] ) , 'mx-', linewidth=2 )
    tt = linspace(0,1,100)
    plot( polyval( px, tt ), polyval( py, tt) , 'k-', linewidth=2 )
    for i in ir:
      plot( [ right.x[i], polyval( px, utr[i-ir[0]] ) ],
            [ right.y[i], polyval( py, utr[i-ir[0]] ) ],'b.-')
    for i in il:
      plot( [  left.x[i], polyval( px, utl[i-il[0]] ) ],
            [  left.y[i], polyval( py, utl[i-il[0]] ) ],'r.-')
    ion(); show();

  print "r:%7.5g l:%7.5g"%( hypot( dxr, dyr ).mean() ,hypot( dxl, dyl ).mean() )
  return hypot( dxl, dyl ).mean() ,hypot( dxr, dyr ).mean() 
  #return d/float( len(dxr)+len(dxl) )


def build_inverse_table( wv, shape, scale ):
  map = {}
  stride = shape[1]/scale
  topx = lambda p: int(p[0]/scale) + stride * int(p[1]/scale)
  for i,w in wv.iteritems():
    for j,p in enumerate(zip(w.x,w.y)):
      idx = topx(p)
      if idx in map:
        map[ idx ].add((i,j))
      else:
        map[ idx ] = set([(i,j)])
  return map

def update_inverse_table( map, w, oldid, newid, shape, scale, bnd = None ):
  stride = shape[1]/scale
  topx = lambda p: int(p[0]/scale) + stride * int(p[1]/scale)
  def opdel(s,oid,nid):
    s.discard(oid)
  def opreplace(s,oid,nid):
    s.discard(oid)
    s.add(nid)
  def opinsert(s,oid,nid):
    s.add(nid)

  if newid is None:
    op = opdel
  elif oldid is None:
    op = opinsert
  else:
    op = opreplace
  
  g = ( e for e in enumerate(zip(w.x,w.y)) )
  if not bnd is None:
    g = ( e for e in enumerate(zip( w.x[bnd[0]:bnd[1]], w.y[bnd[0]:bnd[1]] )) )
  for j,p in g:
    px = topx(p)
    if not px in map:
      map[px] = set()
    op( map[topx(p)], (oldid,j), (newid,j) )

def compute_table_count( table, shape, scale, *args, **kwargs ):
  tosc = lambda e: e/scale
  im = zeros(map(tosc, shape))
  imr = im.ravel()
  for px,s in table.iteritems():
    imr[px] = len(s)
  return im

def get_next_match( map ):
  """ This changes the inverse table by removing hits"""
  todelete = []
  retval = None
  for px,s in map.iteritems():
    if len(s) > 1:
      retval = s.pop(),s.pop()
      if retval[0][0] == retval[1][0]:
        s.add( retval[1] )
        #print retval, s
        retval = None
        continue
      else:
        break
    else:
      todelete.append(px)

  for k in todelete:
    del map[k]

  return retval

def trace_overlap( (wa,i), (wb,j), thresh = 2.0 ):
  # assumes that indexes run along same direction (i.e. x is monitonically
  # increasing along index )
  def dist(ia,ib):
    a,b = wa[ia], wb[ib]
    return hypot( a[0] - b[0], a[1] - b[1] )
  #pdb.set_trace()
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

def breakout( w, bnd ):
  left,rest    = w.split(bnd[0])
  middle,right = rest.split(bnd[1]-bnd[0])
  return left, middle, right

def aggregate_pair( wa, wb ):
  """
  `wa` and `wb` spatially overlap but are indexed differently
  Assume endpoints correspond.

  Approaches:
  1. dp to map corresponding indices
         probably not right, here we have some notion of continuity and endpoint correspondance
         also, too complicated
  2. interpolation - aliased ('nearest')
         sometimes aliasing introduces zigzags, doesn't work
  3. interpolation - linear
  """
  def interp(a, t):
    u =  t*(len(a)-1) 
    s = u.astype(int)
    r = u - s
    d = diff(a)
    return (a[s] + d[s]*r).astype(float32)
  m = max( wa, wb, key = lambda e: len(e) ).copy()
  t = linspace(0,1,len(m))[:-1]
  wax = interp( wa.x     , t )
  way = interp( wa.y     , t )
  wat = interp( wa.thick , t )
  was = interp( wa.scores, t )
  wbx = interp( wb.x     , t )
  wby = interp( wb.y     , t )
  wbt = interp( wb.thick , t )
  wbs = interp( wb.scores, t )
  norm = was + wbs
  m.x      = ( wax*was + wbx*wbs ) / norm
  m.y      = ( way*was + wby*wbs ) / norm
  m.thick  = ( wat*was + wbt*wbs ) / norm
  m.scores = ( was*was + wbs*wbs ) / norm
  return m

def build_resolution(wa, wb, bnda, bndb ):
  return ResolutionGroup(wa,wb,bnda,bndb)
    
def build_resolution_v0(wa, wb, bnda, bndb ):
  la,ma,ra = breakout( wa, bnda )
  lb,mb,rb = breakout( wb, bndb )
  m = aggregate_pair(ma,mb) # want the same test point for decision

  f = lambda e: len(e)>len(m)
  if la and ra and lb and rb:
    if f(la) and f(ra) and f(lb) and f(rb):
      return ( (la,ra), (lb,rb) )

  def score( u, v, side=None ):
    if u and v:
      s = compute_join_err( u,v )
      #px,py = solve_polynomial_join( u,v )
      #s = max(map(abs,( px[0],py[0] )))
      # s0 = compute_join_curvature(px,py) # units: 1/L (average curvature on path)
      # s1 = compute_join_length(px,py)    # units: L   (path length)
      # s = s0*s1                          # unitless   (total curvature)
      if side is None:
        return s
      else:
        return s[side]
    else:
      return None
  def format_merges(*merges):
    merges = [ [x for x in p if x] for p in merges] #remove Nones
    return [p for p in merges if p]                 #remove empties
  def chooseA():
    return format_merges(  (la,ma,ra), (lb,), (rb,)  )
  def chooseB():
    return format_merges(  (la,), (ra,), (lb,mb,rb)  ) 
  def chooseAB():
    return format_merges(  (la,m ,rb), (lb,), (ra,)  )
  def chooseBA():
    return format_merges(  (lb,m ,ra), (la,), (rb,)  )
  chooser = {'aa':chooseA,'ab':chooseAB,'ba':chooseBA,'bb':chooseB}
  
  #pdb.set_trace()

  lc,rc = None,None
  ars,brs,als,bls = None,None,None,None
  if la and lb:
    if( len(la) < 2 and len(lb) > 2 ):
      lc = 'b'
    elif( len(la) > 2 and len(lb) < 2):
      lc = 'a'
    else:
      als = score( la, m, 1 )
      bls = score( lb, m, 1 )
      if als<bls: 
        lc = 'a' 
      else:
        lc = 'b'
  elif la:
    lc = 'a'
  elif lb:
    lc = 'b'
  else:
    lc = 'a'
  
  if ra and rb:
    if( len(ra) < 2 and len(rb) > 2 ):
      rc = 'b'
    elif( len(ra) > 2 and len(rb) < 2):
      rc = 'a'
    else:
      ars = score( m, ra, 0 )
      brs = score( m, rb, 0 )
      if ars<brs: 
        rc = 'a' 
      else:
        rc = 'b'
  elif ra:
    rc = 'a'
  elif rb:
    rc = 'b'
  else:
    rc = 'a'
  
  #check versus the cross-over if applicable
  f = lambda e: len(e)>2     
  if als and ars and bls and brs:
    if f(la) and f(lb) and f(ra) and f(rb):
      acs = mean(score( la, ra ))
      bcs = mean(score( lb, rb ))
      #pdb.set_trace()
      if acs < min(als,ars) and bcs < min(bls,brs):
      #if (als+ars+bls+brs) > 2*(acs+bcs): #if average of split is worse than crossover
        return ( (la,   ra), (lb,   rb) )

  return chooser[lc+rc]()

def resolve( wdict, table, shape, scale, ma, mb, resolution ):
  """
  `resolution` is a ResolutionGroup 

  A resoltion group is required, as opposed to a WhiskerGroup, because some
  specific bookkeeping is required to know which whisker segments to add back
  into the table.
  """
  # first, remove whiskers referenced by ma and mb from table and wv
  op = lambda match: wdict[match[0]]
  update_inverse_table( table, op(ma), ma[0], None, shape, scale ) # remove old: a
  update_inverse_table( table, op(mb), mb[0], None, shape, scale ) # remove old: b
  time = op(ma).time
  del wdict[ma[0]]
  del wdict[mb[0]]
    
  #from pylab import imshow
  #imshow( compute_table_count(table,shape,scale) )
  #pdb.set_trace()

  # second, add back whiskers in resolution
  nodes = resolution.mainnodes
  lastkey = max( wdict.keys() )
  for w in nodes:
    update_inverse_table( table, w, None, lastkey + 1, shape, scale )
    w.time = time
    wdict[ lastkey + 1 ] = w
    lastkey += 1

def resolve_v0( wdict, table, shape, scale, ma, mb, resolution ):
  # first, remove whiskers referenced by ma and mb from table and wv
  op = lambda match: wdict[match[0]]
  update_inverse_table( table, op(ma), ma[0], None, shape, scale ) # remove old: a
  update_inverse_table( table, op(mb), mb[0], None, shape, scale ) # remove old: b
  time = op(ma).time
  del wdict[ma[0]]
  del wdict[mb[0]]

  # second, add back whiskers in resolution
  lastkey = max( wdict.keys() ) 
  for change in resolution:
    w = reduce(lambda s,t: s.join_right(t), change )
    w.time = time
    wdict[ lastkey + 1 ] = w
    update_inverse_table( table, w, None, lastkey + 1, shape, scale )
    lastkey += 1


