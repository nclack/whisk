from pylab import *
from numpy import *
import trace
import pdb
#
# LOAD FIELDS
#
from ui.genetiff import Reader
from trace import *
#movie = Reader('../../data/S0.tif',adjuststipple=1)
#movie = Reader('../../data/seq/whisker_data_0140.seq',adjuststipple=1)
movie = Reader('../../data/JF8410_041808_001.tif',adjuststipple=1)
im = movie[0]
#h,m,s = compute_seed_fields( im, 4  )

#
# TRACING
#

def draw_whisker( image, w, thick, color ):
  th = arctan2( diff(w.y) , diff(w.x) ).mean() + pi/2  
  ox,oy = thick*cos(th),thick*sin(th)
  xy0 = r_[w.x+ox,w.y+oy].reshape(2,len(w.x)).T
  xy1 = r_[w.x-ox,w.y-oy].reshape(2,len(w.x)).T
  xy = r_[xy0,xy1[::-1]]

  draw_poly( image, xy, color )

def draw_poly( image, xy, color ):
  edges = [ ((x0,x1),(y0,y1)) for (x0,y0),(x1,y1) in zip( xy[:-1], xy[1:] ) ]
  edges.append( ((xy[-1][0],xy[0][0]), (xy[-1][1],xy[0][1])) )
  runs = {}
  for e in edges:
    yrange = range( floor(min(e[1])), ceil(max(e[1])+1 ))
    n = float(len(yrange))
    for i,y in enumerate( yrange ):
      if y < 0 or y >= image.shape[0]:
        continue
      t = i/n
      x = e[0]
      if not y in runs:
        runs[y] = [ floor(x[0] + t * ( x[1] - x[0] )) ]
      else:
        r = runs[y]
        x =  floor(x[0] + t * ( x[1] - x[0] ))
        r.append( x )
        runs[y] = [min(r), max(r)]
  #sanitize
  for y,(x0,x1) in runs.iteritems():
    x0 = max(0,x0)
    x1 = min( image.shape[1], x1 ) 
    runs[y] = (x0,x1)
  #render
  for y,(x0,x1) in runs.iteritems():
    image[y][ int(x0):int(x1) ] = color

def trace_from_fields( image ):
  h,m,st = compute_seed_fields_windowed_on_objects( image, maxr = 4, window = (0.4,0.4) )
  mask = (st>0.4).astype(int)
  yy,xx = where(mask)
  th = m[yy,xx]
  dx = 100 * cos( th )  # these get cast as integers
  dy = 100 * sin( th )
  count = 0
  wv = []
  #imshow(mask,hold=0)
  for s in zip(xx,yy,dx,dy):
    if mask[s[1],s[0]]==1:
      #plot( [s[0]],[s[1]],'r.' )
      #axis("image")
      sd = cSeed(*map(int,s) )
      #print sd #, h[s[1],s[0]], st[s[1],s[0]]
      count += 1
      w = trace.Trace_Whisker( sd , image )
      if w:
        #print "\tGot whisker"
        draw_whisker( mask, w, 2, 3 )
        #imshow(mask)
        wv.append(w)
  #print "Efficiency: %g"%( len(wv)/float(count))
  #print "\t%5d whiskers found from %d seeds with %d attempted tracings"%( len(wv), xx.shape[0], count)

  return wv

def trace_movie_from_fields( movie ):
  w = {}
  for i,im in enumerate(movie):
    print "Frame %5d of %5d"%(i, len(movie))
    w[i] = dict([p for p in  enumerate(trace_from_fields( im )) ])
  return w

#
# PLOT FIELDS
#

def plotfields(im):
  h,m,s = compute_seed_fields( im, 5 )
  figure()
  subplots_adjust(0.05, 0.05, 0.95, 0.95, 0.1, 0.1 )
  subplot(2,2,1)
  imshow(im,cmap=cm.gray)
  axis("off")
  subplot(2,2,2)
  imshow(h,cmap=cm.jet)
  axis("off")
  colorbar()
  subplot(2,2,2)
  subplot(2,2,3)
  imshow((m+pi)*180/pi,hold=0,cmap=cm.hsv,vmin=0,vmax=360)
  axis("off")
  colorbar()
  subplot(2,2,4)
  axis("off")
  imshow(s,cmap=cm.spectral,vmin=s.max()*0.9)
  colorbar()

def plot_fields_multimax(im, maxr = xrange(4,8,2) ):
  figure()
  subplots_adjust( 0, 0, 1, 1, 0.05, 0.05 )
  for idx, r in enumerate(maxr):
    h,m,s = compute_seed_fields( im, r )
    
    subplot( 3, len(maxr), idx +             1 )
    imshow( h, cmap = cm.jet )
    axis('off')
    subplot( 3, len(maxr), idx + 1*len(maxr) + 1 )
    imshow( (m+pi)*180/pi, cmap = cm.hsv, vmin=0,vmax=360 )
    axis('off')
    subplot( 3, len(maxr), idx + 2*len(maxr) + 1 )
    imshow( s, cmap = cm.spectral, vmin = s.max()*0.9);
    axis('off')


#
# IMAGE PYRAMID
#

def _impyramid( im ):
  evener = lambda x: x - x%2
  R,C = map(evener, im.shape)
  a = im[ :R:2,  :C:2]/4;
  b = im[ :R:2, 1:C:2]/4;
  c = im[1:R:2,  :C:2]/4;
  d = im[1:R:2, 1:C:2]/4;

  out = zeros( (R,C) )
  out = (a+b+c+d);
  return out

rcompose = lambda x,f: f(x)
pyramid  = lambda im, n: reduce( rcompose, (im,) + n*(_impyramid,) ) 

def eg():
  figure()
  subplots_adjust( 0, 0, 1, 1, 0.05, 0.05 )
  for i in xrange(4):
    subplot(1,4,i+1)
    imshow( pyramid(im, i ), cmap=cm.gray )
    axis('off')

#
# MULTIRESOLUTION FIELDS
#
def plot_fields_multiresolution( im, levels = range(3), maxr = 16  ):
  figure()
  subplots_adjust( 0, 0, 1, 1, 0.05, 0.05 )
  for idx, i in enumerate(levels):
    t = pyramid( im, i )
    h,m,s = compute_seed_fields( t, maxr/(2**i) )

    subplot( 4, len(levels), idx + 1 )
    imshow( t, cmap = cm.gray )
    axis('off')
    subplot( 4, len(levels), idx + len(levels) + 1 )
    imshow( h, cmap = cm.jet )
    axis('off')
    subplot( 4, len(levels), idx + 2*len(levels) + 1 )
    imshow( (m+pi)*180/pi, cmap = cm.hsv, vmin=0,vmax=360 )
    axis('off')
    subplot( 4, len(levels), idx + 3*len(levels) + 1 )
    imshow( s, cmap = cm.jet, vmin = 0, vmax = 0.5 );
    axis('off')

