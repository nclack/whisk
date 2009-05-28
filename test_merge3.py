from tests import plot_whiskers
from ui.whiskerdata.trace import Whisker_Seg
from numpy import *
import pdb

def load():
  from ui.whiskerdata import load_whiskers, load_trajectories
  from ui.genetiff import Reader
  movie = Reader('data/seq/whisker_data_0140.seq',adjuststipple=1)
  w,wid = load_whiskers('seq.whiskers')
  #movie = Reader('../../data/W0.tif',adjuststipple=1)
  #w,wid = load_whiskers('w0-grid.whiskers')
  #w,wid = load_whiskers('whisk-vc/whisk-vc/seq.whiskers')
  #movie = Reader('data/JF8410_041808_001.tif',adjuststipple=1)
  #w,wid = load_whiskers('test.whiskers')
  #movie = Reader('data/lorenz/090519-19a_0035.seq',adjuststipple=1)
  #w,wid = load_whiskers('lorenz.whiskers')
  #w,wid = load_whiskers('results/seq-hand.whiskers')
  #t,tid = load_trajectories('results/seq-hand.trajectories')
  return w,movie

def check_bounds(wvd,shape):
  for fid, wv in wvd.iteritems():
    for i,w in wv.iteritems():
      for x,y,t,s in w:
        if x<0 or x>=shape[1] or y<0 or y>=shape[0]:
          print "out of bounds"
          pdb.set_trace()
        if not ( w.x.flags.contiguous and w.y.flags.contiguous ):
          print "not contiguous"
          pdb.set_trace()
  

def fix(wvd,movie,scale=2, signal_per_pixel = 0, max_dist = 60, max_angle = 20.*pi/180.):
  shape = movie[0].shape
  for fid,wv in wvd.items():
    print fid
    table = CollisionTable( wv, shape, scale )
    r = set( resolution( table, wv ) )
    for j,l in choose_gaps(movie[fid],r,signal_per_pixel,max_dist,max_angle):
      e = reduce( Whisker_Seg.join, j )
      r.discard( j[0] )
      r.discard( j[-1] )
      r.add(e)
    wvd[fid] = dict( [ p for p in enumerate(r) ] )
  return wvd
    
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

def compute_join_angle( px, py ):
  from scipy.integrate import quad
  xp  = polyder( px, 1 )
  yp  = polyder( py, 1 )
  integrand = lambda t:  arctan2(polyval(yp, t), polyval(xp, t))  
  return quad(integrand, 0, 1) [0]

def _compute_intensity( im, x, y ):
  if ( x<0 ).any()              or  \
     ( x>=im.shape[1] ).any()   or  \
     ( y<0 ).any()              or  \
     ( y>=im.shape[0] ).any():
     return inf
  p = set( p for p in zip(x,y) )
  score = 0
  for j,i in p:
    score += im[i,j]
  return score/len(p)

def compute_join_intensity( im, px, py ):
  tt = linspace(0,1,50)
  x = array( [round(polyval(px,t))  for t in tt] )
  y = array( [round(polyval(px,t))  for t in tt] )
  return _compute_intensity(im,x,y)

def compute_join_score( im, px, py, thick = 2 ):
  tt = linspace(0,1,50)
  dpx = polyder(px)
  dpy = polyder(py)
  dL2 = polymul(dpx,dpx) + polymul(dpy,dpy)
  ux  = polyval( px,tt )
  uy  = polyval( py,tt )
  dx  = diff(ux) #polyval( px,tt )
  dy  = diff(uy) #polyval( py,tt )
  dx  = r_[dx[0],dx]
  dy  = r_[dy[0],dy]
  dL  = sqrt( dx**2 + dy**2 )

  a = _compute_intensity(im, ux, uy )
  b = _compute_intensity(im, ux + thick*dy/dL , uy - thick*dx/dL )
  c = _compute_intensity(im, ux - thick*dy/dL , uy + thick*dx/dL )
  return (2*a - b - c)/4.0

def solve_polynomial_join( left, right, reverse = 0):
  """
  Solves for a parametric cubic polynomial curve joining the right side of left
  to the left side of right.  The curve matches slope and position at it's
  boundaries and is parameterized from 0 to 1; 0 being the left boundary and 1
  being the right.

  method:       parametric cubic matching position and slope of endpoints.
                This ends up being cheap to compute, since the matrix is
                known (interval of parameter is always 0 to 1) and so the 
                inverse can be precomputed. 
     minv is inverse of m, where:
          m = array( [  [   a**3,   a**2, a, 1 ],
                        [   b**3,   b**2, b, 1 ],  
                        [ 3*a**2, 2*a   , 1, 0 ],
                        [ 3*b**2, 2*b   , 1, 0 ] ] )
     is the matrix for the linear system:
          m * coeff = v,
     with v = [ x(0) x(1) dx/dt(0) dx/dt(1) ].
     Here a = 0 and b = 1 so m and it's inverse is always the same.
  
  """
  minv = matrix( [[ 2., -2.,  1.,  1.],
                  [-3.,  3., -2., -1.],
                  [ 0.,  0.,  1.,  0.],
                  [ 1.,  0.,  0.,  0.]])
  #take care of cases joining very short segements
  lr = len(right)
  ll = len(left)
  #L = length( right.x, right.y ) + length( left.x, left.y )
  #dd = hypot( left.x[0] - right.x[-1], left.y[0] - right.y[-1] )
  nl = ll/4
  nr = lr/4
  slope = lambda v: v[ 0] - v[-1]                   # want the total change over the length
  #slope = lambda v: diff(v).mean()
  length = lambda x,y: hypot(diff(x),diff(y)).sum() # euclidian distance in pixels
  #
  # Compute slope at boundary.
  # Uses a number of points near the boundary to compute slope.
  # Need to account for edge cases where one or both sides
  # consist of very few points.
  #
  if nr < 2 and nl < 2: 
    lnorm = length(  left.x       ,  left.y        ) 
    rnorm = length( right.x       , right.y        ) 
    dly =  diff( left.y ).mean() / lnorm
    dlx =  diff( left.x ).mean() / lnorm
    dry =  diff(right.y ).mean() / rnorm
    drx =  diff(right.x ).mean() / rnorm
    nl = 0
    nr = lr - 1
  elif nr < 2:                        # use the derivative on the other side
    lnorm = length(  left.x[:nl],  left.y[:nl] ) 
    rnorm = length( right.x       , right.y        ) 
    dly =  -slope( left.y[(-nl):]  ) / lnorm
    dlx =  -slope( left.x[(-nl):]  ) / lnorm
    dry =  diff(right.y ).mean() / rnorm
    drx =  diff(right.x ).mean() / rnorm
    nr = lr - 1
    #print dly,dlx,dry,drx
  elif nl < 2:                        # use the derivative on the other side
    rnorm = length( right.x[:nr], right.y[:nr] ) 
    lnorm = length(  left.x       ,  left.y        ) 
    dry =  -slope(right.y[:nr]  ) / rnorm
    drx =  -slope(right.x[:nr]  ) / rnorm
    dly =  diff( left.y ).mean() / lnorm
    dlx =  diff( left.x ).mean() / lnorm
    nl = 0
  else:                               # the "normal" case
    rnorm = length( right.x[:nr], right.y[:nr] )       # Compute path length of right border region
    lnorm = length(  left.x[(-nl):],  left.y[(-nl):] ) # Compute path length of left  border region
    dry =  -slope(right.y[:nr]  ) / rnorm              # Compute dy/dl for right side
    drx =  -slope(right.x[:nr]  ) / rnorm              # etc...
    dly =  -slope( left.y[(-nl):]  ) / lnorm
    dlx =  -slope( left.x[(-nl):]  ) / lnorm
  rnorm = hypot( left.x[0] - right.x[0], left.y[0] - right.y[0] ) 
  lnorm = hypot( left.x[-1]- right.x[0], left.y[-1]- right.y[0] )
  if not isfinite(dlx): dlx =(left.x[0]  - right.x[0])/lnorm
  if not isfinite(dly): dly =(left.y[0]  - right.y[0])/lnorm
  if not isfinite(drx): drx =(left.x[-1] - right.x[0])/rnorm
  if not isfinite(dry): dry =(left.y[-1] - right.y[0])/rnorm
   
  if reverse:
    dlx = -dlx
    dly = -dly
    drx = -drx
    dry = -dry

  ry = right.y[ 0]             ## right.y[nr] 
  ly = left.y[-1 ]             ## left.y[-nl] 
  rx = right.x[ 0]             ## right.x[nr] 
  lx = left.x[-1 ]             ## left.x[-nl] 
  L = hypot( rx-lx, ry-ly )    # Approximate dl/dt  
  print "L:%g"%L
  yv = matrix(   [[  ly                          ],   
                  [  ry                          ],   
                  [  dly * L                     ],    # dy/dt = dy/dl * dl/dt
                  [  dry * L                     ]])
  xv = matrix(   [[  lx                          ],                
                  [  rx                          ],                
                  [  dlx * L                     ],  
                  [  drx * L                     ]])
  cx = minv*xv
  cy = minv*yv
  
  if not (isfinite(cx).any() and isfinite(cy).any()):
    pdb.set_trace()
  return map( lambda t: array(t).squeeze() , (cx,cy) )

def plot_join(px,py,*args,**kwargs):
  from pylab import plot, polyval
  tt = linspace(0,1,50)
  plot( polyval(px,tt), polyval(py,tt), *args, **kwargs )

def plot_test(px,py,thick=2):
  from pylab import plot 
  tt = linspace(0,1,50)
  dpx = polyder(px)
  dpy = polyder(py)
  dL2 = polymul(dpx,dpx) + polymul(dpy,dpy)
  ux  = polyval( px,tt )
  uy  = polyval( py,tt )
  dx  = diff(ux) #polyval( px,tt )
  dy  = diff(uy) #polyval( py,tt )
  dx  = r_[dx[0],dx]
  dy  = r_[dy[0],dy]
  dL  = sqrt( dx**2 + dy**2 )

  plot( ux, uy , '.-')
  plot( ux + thick*dy/dL , uy - thick*dx/dL ,'-')
  plot( ux - thick*dy/dL , uy + thick*dx/dL ,'-' )

def filter_ends( wv, min_score, shape, border = 10 ):
  """
  Return candidate ends for joining.

  Returns an iterator yielding (Whisker_Seg, side).
  """
  maxy, maxx = map( lambda x: x - border, shape )
  minx, miny = border, border
  test_point = lambda x,y: x>minx and x<maxx and y > miny and y < maxy
  bordertest = lambda e,side: test_point( e.x[side], e.y[side] )
  scoretest  = lambda e,side: e.scores[side] > min_score
  sides = [0,-1]
  for e in wv:
    for s in sides:
      if bordertest(e,s) and scoretest(e,s):
        yield e,s

def plot_candidate_ends(im, wv, min_score, border = 10):
  from pylab import plot, imshow, cm, ion,ioff, show, text
  left,right = group_ends( list(filter_ends(wv,min_score,im.shape, border)) ) 

  ioff()
  #imshow(im,cmap=cm.gray,hold=0)
  m = {0:'ro',-1:'gs'}
  for i,e in enumerate(left):
    s = 0
    text(e.x[s],e.y[s],str(i),color=m[s][0])
    plot([e.x[s]],[e.y[s]],m[s])
  for i,e in enumerate(right):
    s = -1
    text(e.x[s],e.y[s],str(i),color=m[s][0])
    plot([e.x[s]],[e.y[s]],m[s])
  show()
  ion()

def group_ends( ends ):
  return [e for e,s in ends if s == 0], [e for e,s in ends if s == -1]

def end_direction(w, side, n=16):
  a = 0
  b = min( n, len(w) )
  if side != 0:
    a = -b
    b = -1
  dx = diff( w.x[a:b] ).mean()
  dy = diff( w.y[a:b] ).mean()
  return dx,dy

def make_joining_whisker(px,py,dist,lthick,lscore,rthick,rscore):
  w = Whisker_Seg()
  tt = linspace(0,1,round(dist))
  w.x = polyval(px,tt).astype(float32)
  w.y = polyval(py,tt).astype(float32) 
  w.thick  = polyval( [rthick-lthick,lthick], tt ).astype(float32) 
  w.scores = polyval( [rscore-lscore,lscore], tt ).astype(float32)  
  return w

def choose_gaps(im,wv, signal_per_pixel = 0.0, max_dist=60, max_angle = pi/4.):
  left,right = group_ends( list(filter_ends(wv,100,im.shape)) ) 
  theta = lambda w,side: reduce(arctan2, reversed( end_direction(w,side) ) ) 
  dtheta = lambda left,right: fabs(theta(left,0) - theta(right,-1))
  for i,a in enumerate(left):
    for j,b in enumerate(right):
      dx = a.x[ 0]-b.x[-1]
      dy = a.y[ 0]-b.y[-1]
      d = hypot(dx,dy)
      dth = dtheta(a,b)
      v = end_direction(a,0)
      norm = hypot(*v)
      proj = dot( v/norm, (dx,dy) )
      # jth: angle change from a to direct line joining a,b
      jth = fabs(arctan2( hypot(*( dx-proj*v[0]/norm, dy-proj*v[1]/norm )) , proj )) 
      #print i,j,
      #print "\tD: %g Proj: %g Theta: %g"%(d,proj,jth*180/pi)
      l=0;
      if d < max_dist and jth < max_angle and proj > 0:
        px,py = solve_polynomial_join( b, a )
        l = compute_join_score(im,px,py)
        if l < -signal_per_pixel:
          #plot_test(px,py)
          print "\tScore: %g Theta: %g"%(l,jth*180/pi)
          e = make_joining_whisker(px,py,d,b.thick[-1],b.scores[-1],a.thick[ 0],a.scores[ 0])
          yield (b,e,a),l

def gap_measures(im,wv):
  pmetric = lambda p: sqrt(dot(p[:-1],p[:-1]))
  left,right = group_ends( list(filter_ends(wv,100,im.shape)) ) 
  shape = (len(left),len(right) )
  d = zeros( shape )
  l = zeros( shape )
  c = zeros( shape )
  cx = zeros( shape )
  cy = zeros( shape )
  for i,a in enumerate(left):
    for j,b in enumerate(right):
      dx = a.x[0 ]-b.x[-1]
      dy = a.y[0 ]-b.y[-1]
      d[i,j] = hypot(dx,dy)
      px,py = solve_polynomial_join( b, a )
      lpx,lpy = solve_polynomial_join( a, a, reverse = 1 )
      rpx,rpy = solve_polynomial_join( b, b, reverse = 1 )
      cx[i,j] = max( pmetric( px - lpx ) , pmetric( px - rpx ) )
      cy[i,j] = max( pmetric( px - lpx ) , pmetric( py - rpy ) )
      #l[i,j] = compute_join_length(px,py)
      l[i,j] = compute_join_score(im,px,py)
      plot_test(px,py)
      #c[i,j] = compute_join_curvature(px,py)
      #if sqrt( px[0]**2 + py[0]**2 ) <  50.0:
      #  plot_join(px,py)
  return d,l,cx,cy

def trace_overlap( (wa,i), (wb,j), thresh = 2.0 ):
  # DONE: does not assume that indexes run along same direction
  def dist(ia,ib):
    a,b = wa[ia], wb[ib]
    return hypot( a[0] - b[0], a[1] - b[1] )
  # determine relative direction of indexing
  ia,ib = i,j
  if ia == len(wa)-1 or  ib == len(wb)-1:
    if ia != 0 and ib != 0:
      dax = wa.x[ia-1] - wa.x[ia]
      day = wa.y[ia-1] - wa.y[ia] 
      dbx = wb.x[ib-1] - wb.x[ib] 
      dby = wb.y[ib-1] - wb.y[ib]
    elif ia == 0:
      dax = wa.x[ia+1] - wa.x[ia]
      day = wa.y[ia+1] - wa.y[ia] 
      dbx = - wb.x[ib-1] + wb.x[ib] 
      dby = - wb.y[ib-1] + wb.y[ib]
    elif ib == 0:
      dax = - wa.x[ia-1] + wa.x[ia]
      day = - wa.y[ia-1] + wa.y[ia] 
      dbx = wb.x[ib+1] - wb.x[ib] 
      dby = wb.y[ib+1] - wb.y[ib]
  else:
    dax = wa.x[ia+1] - wa.x[ia]
    day = wa.y[ia+1] - wa.y[ia] 
    dbx = wb.x[ib+1] - wb.x[ib] 
    dby = wb.y[ib+1] - wb.y[ib]
  stepa = -1; #only need to keep track of one direction
  enda  = 0; 
  notend = lambda i,n: i>n
  if( abs(dax) > abs(day) ): #determine by x change
    if( dax*dbx < 0 ): #have different signs
      stepa = 1
      enda = len(wa)
      notend = lambda i,n: i<n-1
  else: #determine by y change
    if( day*dby < 0 ): #have different signs
      stepa = 1
      enda = len(wa)
      notend = lambda i,n: i<n-1
    
  bnda = [i,i]
  bndb = [j,j]
  ms = 0  
  while ms < thresh and notend(ia,enda) and ib > 0:
    moves = ( ( ia + stepa, ib - 1 ),
              ( ia + stepa, ib     ),
              ( ia       , ib - 1 ) )
    scores = [dist( iam, ibm ) for iam, ibm in moves]
    ms = min(scores)
    for idx,s in enumerate( scores ): #choose best move
      if s == ms:
        ia,ib = moves[idx]
        break
  #relax at boundary, move downhill
  if not notend(ia,enda) and ib == 0: 
    pass
  elif not notend(ia,enda):
    last = ms
    s = dist( ia, ib - 1 )
    while s < last and ib > 1:
      ib -= 1
      last = s
      s = dist( ia, ib - 1 )
  elif ib == 0:
    last = ms
    s = dist( ia + stepa, ib )
    while s < last and notend(ia,enda-stepa):
      ia += stepa
      last = s
      s = dist( ia + stepa, ib )

  bnda[0] = ia
  bndb[0] = ib

  #flip direction
  if stepa == -1:
    stepa = 1
    enda = len(wa)
    notend = lambda i,n:i<n-1
  else:
    stepa = -1
    enda = 0
    notend = lambda i,n: i>n

  ia,ib = i,j
  ms = 0  
  while  ms < thresh and notend(ia,enda) and ib < len(wb)-1:
    moves = ( ( ia + stepa, ib + 1 ),
              ( ia + stepa, ib     ),
              ( ia        , ib + 1 ) )
    scores = [dist( iam, ibm ) for iam, ibm in moves]
    ms = min(scores)
    for idx,s in enumerate(scores):
      if s == ms:
        ia,ib = moves[idx]
        break
  #relax at boundary, move downhill
  if not notend(ia,enda) and ib == len(wb)-1: 
    pass
  elif not notend(ia,enda):
    last = ms
    s = dist( ia, ib + 1 )
    while s < last and ib < len(wb)-2:
      ib += 1
      last = s
      s = dist( ia, ib + 1 )
  elif ib == len(wb)-1:
    last = ms
    s = dist( ia + stepa, ib )
    while s < last and notend(ia,enda-stepa):
      ia += stepa
      last = s
      s = dist( ia + stepa, ib )

  bnda[1] = ia
  bndb[1] = ib
  bnda.sort()
  return bnda, bndb

def resolution(table, wvd):
  rest = set(wvd.values())
  match = table.next()
  while match:
    keep,discard = merge(match)
    if discard: 
      for a in discard:
        table.remove( a )
      for a in keep:
        yield a
      for a,i in match:
        rest.discard(a)
    match = table.next()
  for a in rest:
    yield a

def pairwise_merge( match ):
  overhang = 8
  wa = match[0][0]
  wb = match[1][0]
  bnda, bndb = trace_overlap(*match)
  iscomplete = lambda bnd,w:  bnd[0] < overhang and bnd[1] >= len(w)-overhang
  if iscomplete(bnda,wa) or iscomplete(bndb,wb):
    sa = wa.scores.sum()
    sb = wb.scores.sum()
    if sa > sb:
      return wa,None
    else:
      return None,wb
  return None,None

def merge( match ):
  dep = dict( [ (e[0],0) for e in match ] )

  #iterate through all pairs and mark those who are contained in another whisker
  # The pairwise merge should impose a strict ordering
  match = list(match)
  for i,ma in enumerate(match):
    for j,mb in enumerate(match[ (i+1): ]):
      ra,rb = pairwise_merge( (ma,mb) )
      if ra or rb:
        if not ra:
          dep[ma[0]] = 1
        if not rb:
          dep[mb[0]] = 1
  # partition into two sets.  Those to keep and those to discard.
  # Those to keep depend on none of the others.
  return [ k for k,v in dep.iteritems() if v==0 ], \
         [ k for k,v in dep.iteritems() if v!=0 ]

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
    g = enumerate(wvd)
    if isinstance(wvd, dict):
      g = wvd.iteritems()
    for i,w in g:
      self.add(w)

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
    for i,px in hash(w): # scan back through and remove repeat hits on a pixel
      for x in [e for e in self._map[px] if e[0] == w][1:]:
        self._map[px].remove(x)

  def remove(self, w):
    if not w: return
    hash = lambda e: enumerate( map(self.topx,zip(e.x,e.y)) )
    for i,px in hash(w):
      s = self._map.get(px)
      if s:
        s.discard( (w,i) )
    
  def __iter__(self):
    m = self.next()
    while m:
      yield m
      m = self.next()

  def next(self):
    """ This changes the inverse table by removing hits.

    Returns a (Whisker_Seg, index),(Whisker_Seg, index)...  tuple
            or None, if done.
    """
    todelete = []
    retval = None
    for px,s in self._map.iteritems():
      todelete.append(px) # get rid of references to visited pixels
      if len(s) > 1:
        retval = s
        break

    for k in todelete:
      del self._map[k]

    return retval
  
  def counts( self ):
    tosc = lambda e: e/self._scale
    im = zeros(map(tosc, self._shape))
    imr = im.ravel()
    for px,s in self._map.iteritems():
      imr[px] = len(s) #len(set( [e for e,i in s] ))
    return im
