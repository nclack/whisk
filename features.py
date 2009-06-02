from numpy import *
from scipy.integrate import quad

def check_side(w,side):
  if side == 0:
    return (0,1) if w.x[0] < w.x[-1] else (-1,-1)
  else:
    return (-1,1) if w.x[0] < w.x[-1] else (0,-1)

def follicle_x(w,side=0):
  side,dx = check_side(w,side)
  return w.x[side]

def follicle_y(w,side=0):
  side,dx = check_side(w,side)
  return w.y[side]

def integrate_path_length(w):
  return sqrt( diff(w.x)**2 + diff(w.y)**2 ).sum()

def cumulative_path_length(w):
  return concatenate(([0],sqrt( diff(w.x)**2 + diff(w.y)**2 ).cumsum() ))

def median_score(w):
  return median(w.scores)

def median_thick(w):
  return median(w.thick)

def root_angle_rad(w, side=0, n=16):
  n = min(n, len(w.x)/2)
  side,dx = check_side(w,side)
  if side == 0:
    return arctan2( dx*diff(w.y[n:(2*n)]), dx*diff(w.x[n:(2*n)]) ).mean()
  elif side == -1:
    return arctan2( dx*diff(w.y[(-2*n):-n]), dx*diff(w.x[(-2*n):-n]) ).mean()

def root_angle_deg(w, side=0, n=16):
  n = min(n, len(w.x)/2)
  return root_angle_rad(w,side,n) * 180.0/pi

def root_curvature(w,side=0,n=16):
  n = min(n, len(w.x)/4)
  side,dx = check_side(w,side)
  L = cumulative_path_length(w)
  tt = L/L.max()
  teval = tt[n] if side==0 else tt[-n]
  px = polyfit(tt[n:-n],w.x[n:-n],2)
  py = polyfit(tt[n:-n],w.y[n:-n],2)
  xp  = polyder( px, 1 )
  xpp = polyder( px, 2 )
  yp  = polyder( py, 1 )
  ypp = polyder( py, 2 )
  pn = polyadd( polymul( xp, ypp ), polymul( yp, xpp )) #numerator
  pd = polyadd( polymul( xp, xp ) , polymul( yp, yp ) ) #denominator
  kappa = lambda t:  polyval( pn, t )/( polyval( pd, t )**(1.5)) # d Tangent angle/ds 
  return dx*kappa(teval)

def mean_curvature(w,side=0,n=16):
  n = min(n, len(w.x)/4)
  side,dx = check_side(w,side)
  L = cumulative_path_length(w)
  tt = L/L.max()
  teval = tt[n] if side==0 else tt[-n]
  px = polyfit(tt[n:-n],w.x[n:-n],2)
  py = polyfit(tt[n:-n],w.y[n:-n],2)
  xp  = polyder( px, 1 )
  xpp = polyder( px, 2 )
  yp  = polyder( py, 1 )
  ypp = polyder( py, 2 )
  pn = polyadd( polymul( xp, ypp ), polymul( yp, xpp )) #numerator
  pd = polyadd( polymul( xp, xp ) , polymul( yp, yp ) ) #denominator
  kappa = lambda t:  dx*polyval( pn, t )/( polyval( pd, t )**(1.5)) # d Tangent angle/ds * ds/dt 
  return quad(kappa,0,1,epsrel=1e-3)[0]

def time(w):
  return w.time

