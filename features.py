import numpy as np
from scipy.integrate import quad
import re

def shape_from_whiskers(wvd):
  x,y = 0,0
  for wv in wvd.itervalues():
    for w in wv.itervalues():
      x = max(x,w.x.max())
      y = max(y,w.y.max())
  return x+1,y+1

def helper_face_point(shape, directive):
  helpers = {
   'top'    : lambda : (  shape[1]/2 ,  -shape[0]/2) ,
   'left'   : lambda : ( -shape[1]/2 ,   shape[0]/2) ,
   'bottom' : lambda : (  shape[1]/2 , 3*shape[0]/2) ,
   'right'  : lambda : (3*shape[1]/2 ,   shape[0]/2)
  }

  m = re.search("(/d+),(/d+)",directive)
  if m:
    if len(m.groups())==2:
      return tuple(map(int,m.groups()))
    else:
      raise Exception, 'Could not interpret directive: %s'%directive
  else:
    try:
      return helpers[directive]()
    except KeyError:
      print "Available directives"
      for k in helpers.iterkeys():
        print '\t',k
      raise Exception, 'Could not use supplied directive: %s'%directive

def make_side_function(cx,cy):
  """ returns (follicle index, dx) """
  d2 = lambda e,side: (e.x[side]-cx)**2 + (e.y[side]-cy)**2
  s  = lambda e: (0,1) if d2(e,0)<d2(e,-1) else (-1,-1)
  return s

def make_comparitor(cx,cy):
  side = make_side_function(cx,cy)
  angle = lambda e: np.arctan2(e.y[side(e)]-cy,e.x[side(e)]-cx)
  return lambda s,t: cmp( angle(s), angle(t) )

def follicle_x(w,side):
  side,dx = check_side(w,side)
  return w.x[side]

def follicle_y(w,side):
  side,dx = check_side(w,side)
  return w.y[side]

def integrate_path_length(w):
  return np.sqrt( np.diff(w.x)**2 + np.diff(w.y)**2 ).sum()

def cumulative_path_length(w):
  return np.concatenate(([0],np.sqrt( np.diff(w.x)**2 + np.diff(w.y)**2 ).cumsum() ))

def median_score(w):
  return np.median(w.scores)

def median_thick(w):
  return np.median(w.thick)

def root_angle_rad(w, side, dx, n=16):
  n = min(n, len(w.x)/2)
  if side == 0:
    return np.arctan2( dx*np.diff(w.y[n:(2*n)]), dx*np.diff(w.x[n:(2*n)]) ).mean()
  elif side == -1:
    return np.arctan2( dx*np.diff(w.y[(-2*n):-n]), dx*np.diff(w.x[(-2*n):-n]) ).mean()

def root_angle_deg(w, side, dx, n=16):
  n = min(n, len(w.x)/2)
  return root_angle_rad(w,side,dx,n) * 180.0/np.pi

def root_curvature(w,side,dx,n=16):
  n = min(n, len(w.x)/4)
  L = cumulative_path_length(w)
  tt = L/L.max()
  teval = tt[n] if side==0 else tt[-n]
  px = np.polyfit(tt[n:-n],w.x[n:-n],2)
  py = np.polyfit(tt[n:-n],w.y[n:-n],2)
  xp  = np.polyder( px, 1 )
  xpp = np.polyder( px, 2 )
  yp  = np.polyder( py, 1 )
  ypp = np.polyder( py, 2 )
  pn = np.polyadd( polymul( xp, ypp ), np.polymul( yp, xpp )) #numerator
  pd = np.polyadd( polymul( xp, xp ) , np.polymul( yp, yp ) ) #denominator
  kappa = lambda t:  np.polyval( pn, t )/( np.polyval( pd, t )**(1.5)) # d Tangent angle/ds 
  return dx*kappa(teval)

def mean_curvature(w,side,dx,n=16):
  n = min(n, len(w.x)/4)
  L = cumulative_path_length(w)
  tt = L/L.max()
  teval = tt[n] if side==0 else tt[-n]
  px = np.polyfit(tt[n:-n],w.x[n:-n],2)
  py = np.polyfit(tt[n:-n],w.y[n:-n],2)
  xp  = np.polyder( px, 1 )
  xpp = np.polyder( px, 2 )
  yp  = np.polyder( py, 1 )
  ypp = np.polyder( py, 2 )
  pn = np.polyadd( np.polymul( xp, ypp ), np.polymul( yp, xpp )) #numerator
  pd = np.polyadd( np.polymul( xp, xp ) , np.polymul( yp, yp ) ) #denominator
  kappa = lambda t:  dx*np.polyval( pn, t )/( np.polyval( pd, t )**(1.5)) # d Tangent angle/ds * ds/dt 
  return quad(kappa,0,1,epsrel=1e-3)[0]

def time(w):
  return w.time

