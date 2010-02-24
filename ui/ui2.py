#!/usr/bin/env python
# Todo list
# =========
#
# ui
# --
# XXX  : add a read-only state -- no longer a concern since autosave is off
# TODO : Add ability to cut/erase parts of a whisker segment
# TODO : Add ability to identify multiple whisker segments as part of the same
#        whisker
# TODO : Add ability to block out certain background elements 
#
# - Low priority
# TODO : display area under cursor as zoomed in lower corner (toggle on/off)
# TODO : documentation, tutorial, help messages
# TODO : add ability to set status text
# TODO : add ability to display help text
# FIXME: displayed frame number doesn't reflect currently displayed frame at end of mouseclick
#
# Whisker tracking
# ----------------
#
#
# Notes
# =====
# 
# Representation of whiskers
# --------------------------
#
# Legend:
#     -->     indicates a key value mapping
#     {}      indicates a set of multiple objects (usually in a dict)
#
# The whiskers dict is:
#
#   - frame_id --> { segment_id --> whisker_data } 
#
#   - whiskers is a dict with several fields describing the segment geometry
#     The segment_id is only unique within the context of a single frame.
#
# The trajectories dict is:
# 
#   - trajectory_id --> { frame_id --> segment_id}
#

import os, sys, traceback, pygame
from numpy import * 
import genetiff
import re
import aggdraw
import trace
import whiskerdata
from whiskerdata import save_state, load_state
import colorsys
import optparse

from matplotlib import cm #color mappings

import datetime #for default label

import pdb

pygame.surfarray.use_arraytype('numpy')

drawing_modes = { 0: "outline",   # see draw_whisker
                  1: "backbone",
                  2: "none" }

# aa line drawing through pygame sucks on os x
drawlines = pygame.draw.aalines
try:
  if os.uname()[0] == 'Darwin':
    drawlines = pygame.draw.lines
except:
  pass

g_render_plane_surface = None
def render_plane(screen, im, inc=1, goto=None, scale=1, baseline=None, mode=None):
  try:
    if goto != None:
      im.seek(goto)
    else:
      c = im.tell()
      if inc<0 and (c+inc) < 0:
        im.seek(len(im)-1)
      else:
        im.seek( im.tell()+inc )
  except EOFError:
    im.seek(0) #loop
    
  a = array(im)
  
  global g_render_plane_surface
  if not g_render_plane_surface:
    g_render_plane_surface = pygame.Surface( a.T.shape, 0, 8, (0xFF,0xFF,0xFF,0xFF) )

  pygame.surfarray.blit_array( g_render_plane_surface ,a.T)
  s = g_render_plane_surface.convert(screen)
  
  if baseline != None:
    a = (a - baseline - 1).astype(float)
    a = (255*(a-a.min())/(a.ptp())).astype(uint8)
  
  if scale != 1:
    width,height = s.get_size()
    s = pygame.transform.scale( s, (int(width*scale), int(height*scale) ) )
  return s,a

def get_follicle_pos( vx, vy, scale, facehint ):
  side = {'top'   : lambda xs, ys: ys[0] <  ys[-1],
          'bottom': lambda xs, ys: ys[0] >= ys[-1],
          'left'  : lambda xs, ys: xs[0] <  xs[-1],
          'right' : lambda xs, ys: xs[0] >= xs[-1]}
  def use_xy_facepos(xs,ys): #this acts as a default "side" function.
    """ Parses facehints of the form "x y" or "x,y" 
        Should return 0 when the end of xs,ys is closest to x,y and
                      1 when the beginning is closest.
    """
    xy = facehint.split()
    if(len(xy)==1): #split on space didn't work, try comma
      xy = facehint.split(',')
    #assume things worked by this point
    x = int(xy[0]) * scale
    y = int(xy[1]) * scale
    d = lambda idx: (xs[idx]-x)**2 + (ys[idx]-y)**2
    #print d(0)<d(-1),xs[0],ys[0],xs[-1],ys[-1],x,y
    return d(0) < d(-1)

  eval_idx = [-1,0] # if side function evals true, want index 0, else -1 (end)
  idx = eval_idx[ side.get(facehint,use_xy_facepos)(vx,vy) ]
  return vx[idx], vy[idx]
  
def draw_whisker( surf, w, radius=12, color=(0,255,255,200) , scale=1, color2=(0,255,255,200), thick = 0.0, selected=False, mode = None, facehint = 'top'): # DHO, changed default radius value.
  if len(w.x) == 0:
    return

  y = (w.y+0.5) * scale
  x = (w.x+0.5) * scale

  #build whisker polygons
  if not mode or drawing_modes[mode["draw"]] == "outline":
    th = arctan2( diff(w.y), diff(w.x) )
    eth = zeros( (th.shape[0]+1,), dtype = th.dtype )
    eth[0] = th[0]
    eth[1:] = th
    cs = (w.thick + thick) * cos(pi/2. - eth)
    ss = (w.thick + thick) * sin(pi/2. - eth)
    p = list( zip(x - cs, y + ss) )
    p.extend( reversed( zip(x + cs,y - ss) ) )
    p.append( p[0] )
  elif drawing_modes[mode["draw"]] == "backbone":
    p = list( zip(x , y ) )
  else:
    p = None

  if p and len(p) > 1:
    try:
      if selected:
        drawlines( surf, color2, 0, p )
      else:
        drawlines( surf, color, 0, p )
    except:
      pdb.set_trace()
  if selected:
    #xp = 40.* floor( max(x[0]-2*radius,0) / 40.) #snap x position to a rough lattice
    #xp = max(xp,radius);
    pygame.draw.circle( surf, (0,255,0,200), # DHO, changed color to always be green.
        get_follicle_pos( x, y, scale, facehint ), 
        int(radius))

def draw_bar( surf, x, y, radius, color, scale=1 ):
  pygame.draw.circle( surf, color, 
              ( scale*(x+1.0), 
                scale*(y+1.0) ), 
              int(scale*radius),
              1)
  
def distance( (x,y), w):
  r = (w.x-x)**2 + (w.y-y)**2
  return sqrt(r.min())
      
def calc_seed( a, p, d  ):
  return trace.compute_seed( a,p, d )
# sd = trace.compute_seed( a,p, d )
# if not sd:
#   return None
# pdb.set_trace()
# return [sd.xpnt, sd.ypnt, 2*d, 2*d*sd.ydir/sd.xdir]

def load_icon():
  from os.path import join, split
  from inspect import getfile
  path = getfile(load_icon)
  #filename = join(split(join(os.getcwd(), __file__ ))[0],'icon.png')
  filename = join(split(path)[0],'icon.png')
  icon = pygame.image.load(filename)
  pygame.display.set_icon(icon)

def main( filename, 
          whiskers_file_name, 
          transpose, 
          startframe=0, 
          cursor_size = 10,
          show_cursor_pos = False,
          noadjuststripes = False,
          prefix_label = "",
          show_fps = False,
          facehint = 'top' ):
  """ main( moviename, tracking_data_prefix ) --> (whiskers, trajectories)
  
  Opens a window to browse through a movie to review and correct traced
  whiskers.
  """
  pygame.init()
  desktop_info = pygame.display.Info()
  desktop_size = desktop_info.current_w, desktop_info.current_h
  pygame.mouse.set_cursor((8, 8), (4, 4), 
        (24, 24, 24, 231, 231, 24, 24, 24), 
        (0, 0, 0, 0, 0, 0, 0, 0))
  font = pygame.font.SysFont('times',24);
  pygame.key.set_repeat(200, 10)
  
  im = genetiff.Reader( filename, transpose, not noadjuststripes )
  im.seek( startframe )
  #abg = calc_background( im )
  N = len(im)
  
  load_icon()
  pygame.display.set_caption( 'Whisk: '+os.path.split(filename)[-1], 'Whisk');
  
  data_width,data_height = im.size
  
  s = 0.6 * min(map(lambda p: p[0]/float(p[1]), zip(desktop_size, reversed(im.size)) ))
  scale = (s,s)
  size = width,height = [int(s*e) for s,e in zip(scale,reversed(im.size))]
  flags =  pygame.DOUBLEBUF | pygame.HWSURFACE | pygame.RESIZABLE
  
  screen = pygame.display.set_mode( size , flags )
  
  mode = {}
  mode["auto"] = True
  mode["tracing"] = False
  mode["showcursorpos"] = show_cursor_pos
  mode["showfps"] = show_fps
  mode["draw"] = 0
  
  clock = pygame.time.Clock()
  
  whiskers, trajectories, bar_centers, current_whisker = load_state(whiskers_file_name)
  #whiskerdata.merge.merge_all( whiskers, im[0].shape ) 
  state = whiskers, trajectories, bar_centers
  DIRTY = 0
  
  textbgrect = pygame.Rect(10,10,1,1);
  bg,a = render_plane(screen, im, inc=0, scale=scale[0])
  screen.blit(bg,(0,0))
  pygame.display.flip() #for doublebuf
  
  bg,a = render(screen, im, current_whisker, state, bg, scale[0], inc=0, mode=mode, facehint=facehint)
  pygame.display.flip() #for doublebuf  
  cursor_rect = None
  last = bg;
  while 1:
    fps = clock.get_fps()
    
    p = pygame.mouse.get_pos() 
    if not cursor_rect is None:
      cursor_rect = bg.get_clip().clip(cursor_rect)
      if cursor_rect.size:
        s = bg.subsurface( cursor_rect )
        screen.blit( s, cursor_rect.topleft )
    cursor_rect = pygame.draw.circle( screen, 
                                      (255,0,0,255), 
                                      p, 
                                      int(cursor_size*scale[0]), 1 )
    for event in pygame.event.get():
      #print event
      if event.type == pygame.QUIT:
        if DIRTY:
          save_state( whiskers_file_name, whiskers, trajectories, facehint ); # <-- autosave
          DIRTY = 0
        whiskerdata.close()
        pygame.quit()
        return whiskers,trajectories
      
      elif event.type == pygame.VIDEORESIZE:
        cursor_rect = None
        size = width,height = event.size
        scale = width/float(a.shape[1]), height/float(a.shape[0])
        scale = min(scale)
        width  = int( scale * a.shape[1]  )
        height = int( scale * a.shape[0]  )
        size = width,height
        scale = scale,scale
        screen = pygame.display.set_mode( size, flags ) 
        bg,a = render(screen, im, current_whisker, state, bg, scale[0], inc=0, mode=mode, facehint=facehint)
      
      elif event.type == pygame.MOUSEBUTTONDOWN:
        #print mode, scale
        #print event
        if event.button == 4:   # mouse wheel up
          bg,a = render(screen, im, current_whisker, state, bg, scale[0], inc=1, mode=mode, facehint=facehint)
        
        elif event.button == 5: # mouse wheel down
          bg,a = render(screen, im, current_whisker, state, bg, scale[0], inc=-1, mode=mode, facehint=facehint)
        
        elif event.button == 1: # mouse left click
          # mark a whisker segment and advance
          # 
          DIRTY = 1
          p = event.pos   
          ps = [ round(x/s) for s,x in zip(scale,p) ] #map screen coordinates to data coordinates
          iframe = im.tell()
          
          # find closest segment in frame
          best = None
          bestd = inf;
          inc = 1;
          if mode["tracing"]:
            inc = 0;
          if iframe in whiskers:
            for segid,wseg in whiskers[ iframe ].iteritems():
              d = distance( ps, wseg )
              if d < bestd:
                best = segid
                bestd = d
            if bestd < cursor_size :
              #unlabel 'best' in existing trajectories
              for tid, v in trajectories.iteritems():
                if v.get(iframe,None) == best:
                  del v[iframe]
              #found a whisker segment so link it into the trajectory
              if not current_whisker in trajectories:
                trajectories[current_whisker] = {}
              trajectories[ current_whisker ][ iframe ] = best;
              inc = 1;
            else:
              #didn't find a whisker segment.  If there was a segment linked into the trajectory,
              #  remove it.  Presumably, it would have been found if the user had wanted it.
              if current_whisker in trajectories:
                if iframe in trajectories[current_whisker]:
                  del trajectories[current_whisker][iframe]
          
          bg,a = render(screen, im, current_whisker, state, bg, scale[0], inc=inc, mode=mode, facehint=facehint)
          if bestd < cursor_size:
            fid = max(im.tell()-1,0)
            traj = trajectories[current_whisker]
            if fid in traj:
              sid = traj[fid]
              draw_whisker(screen,whiskers[max(im.tell()-1,0)][sid],color=(0,0,0),color2=(0,0,0),scale=scale[0],thick=1.0, mode=mode,facehint=facehint)
          cursor_rect = pygame.draw.circle( screen, (  0,255,0,255), p, int(cursor_size*scale[0]), 1 ) 
          
          if mode["auto"]:
            e = { 'pos':[ int( scale[0]*ps[0] ),
                          int( scale[0]*ps[1] ) ], 
                  'button':1 } #left click 
            if ( bestd < cursor_size ) and im.tell() < N-1: # continue if nearby hit and not at end of movie
              # update position and send a left  click
              w = whiskers[iframe][best];
              #search for closest and update
              d2 = (w.y-ps[1])**2 + (w.x-ps[0])**2
              iy = where( d2 == d2.min() )[0][0]
              e['pos'][0] = int( scale[0]*w.x[ iy ] )
              e['pos'][1] = int( scale[0]*w.y[ iy ] )              
              pygame.event.post( pygame.event.Event( pygame.MOUSEBUTTONDOWN, e ) )
            else:
              if 0:
                mode["tracing"] = False
                e['button'] = 3            #send a right click
                pygame.event.post( pygame.event.Event( pygame.MOUSEBUTTONDOWN, e ) )

            if im.tell() == N-1:
              mode["auto"] = False
              mode["tracing"] = False
        
        elif event.button == 3: # mouse right click
          DIRTY = 1
          p = event.pos   
          ps = [ round(x/s) for s,x in zip(scale,p) ] #map screen coordinates to data coordinates
          iframe = im.tell()
          #pdb.set_trace()
          a = a.astype( float )
          a = (255*(a-a.min())/(a.ptp())).astype(uint8)
          sd = calc_seed( a, ps , 8 ) #cursor_size)
          inc = 0
          if sd:
            w = trace.Trace_Whisker( sd, a )
            if w:
              inc = 1
              #append to whiskers dict
              if not iframe in whiskers:
                whiskers[iframe] = {}
              fw = whiskers[ iframe ] 
              if fw:
                wid = max(fw.keys()) + 1
              else: 
                wid = 0;
              w.id = wid
              w.time = iframe
              fw[ wid ] = w
              #append to trajectory
              try:
                if not current_whisker in trajectories:
                  trajectories[current_whisker] = {}
                trajectories[ current_whisker ][ iframe ] = wid;
              except KeyError, ke:
                print 80*'-'
                print current_whisker, iframe
                print "current_whisker in trajectories: ", current_whisker in trajectories
                print "iframe in current_whisker      : ", iframe          in trajectories[current_whisker]
                print 80*'-'
                raise ke
              
              bg,a = render(screen, im, current_whisker, state, bg, scale[0], inc=inc, mode=mode, facehint=facehint)
              if w:
                draw_whisker(screen,w,color=(0,0,0),color2=(0,0,0),scale=scale[0],thick=1.0,mode=mode,facehint=facehint)
              cursor_rect = pygame.draw.circle( screen, (  0,255,0,255), p, int(cursor_size*scale[0]), 1 ) 
              
              if mode["auto"]:
                if im.tell() < N-1: # stop if get to end of movie, or lost
                  w = whiskers[iframe][wid];
                  d2 = (w.y-ps[1])**2 + (w.x-ps[0])**2
                  iy = where( d2 == d2.min() )[0][0]
                  mode["tracing"] = True 
                  try:
                    e = pygame.event.Event( pygame.MOUSEBUTTONDOWN, 
                                          pos=( int( scale[0]*w.x[ iy ] ),
                                                int( scale[0]*w.y[ iy ]) ), 
                                          button=3)
                  except IndexError, inderr:
                    print 80*'-'
                    print 'scale:= ', repr(scale)
                    print 'w:= ', repr(w)
                    print 'len(w.y):=%d'%len(w.y)
                    print 'iy:= %d'%iy
                    print 80*'-'
                    raise inderr
                  pygame.event.post(e)
                if im.tell() == N-1:
                  mode["auto"] = False
            else:
              mode["tracing"] = False
              
        else:
          print "Mouse button press: %d"%event.button
          
      elif event.type == pygame.KEYDOWN:
        mode["tracing"] = False
        # process mods: 
        # note - should do this only when left or right are pressed
        mult = 1;
        if event.mod & pygame.KMOD_SHIFT:
          mult *= 10;
        if event.mod & pygame.KMOD_CTRL:
          mult *= 10;
        if event.mod & pygame.KMOD_ALT:
          mult *= 10;
        
        if event.key == pygame.K_d:
          if event.mod & pygame.KMOD_SHIFT:
            pdb.set_trace()
          else:
            mode["draw"] = (mode["draw"]+1)%len(drawing_modes)
            bg,a = render(screen, im, current_whisker, state, bg, scale[0], inc=0, mode=mode, facehint=facehint)

        elif event.key == pygame.K_p:
          if event.mod & pygame.KMOD_SHIFT:
            #from pylab import close
            #close('all')
            objs = trace.compute_object_map(a)
            fig = objs.plot_with_seeds(a)
            #trace.compute_object_map(a).draw(screen,(255,255,  0,255),scale[0],drawlines)
          else:
            from pylab import quiver, imshow, show, ion, axis, subplots_adjust, cm, clf
            h,m,s = trace.compute_seed_fields_windowed_on_objects( a,  maxr = 4, window = (0.4,0.4) );
            #h,m,s = trace.compute_seed_from_point_field_on_grid(a,spacing=8,maxr=4,window=(0.4,0.4) );
            ii,jj = where( s )
            ion()
            clf()
            imshow(a,cmap=cm.gray,interpolation='nearest');
            quiver( jj,ii,s[ii,jj]*cos(m[ii,jj]),-s[ii,jj]*sin(m[ii,jj]),
                    facecolors = ('w',),
                    edgecolors = ('k',),
                    linewidths = (0.5,) )
            axis('image')
            axis('off')
            subplots_adjust(0,0,1,1,0,0)
            show()
        
        elif event.key == pygame.K_ESCAPE:
          if DIRTY:
            save_state( whiskers_file_name, whiskers, trajectories, facehint ); # <-- autosave
            DIRTY = 0
          whiskerdata.close()
          pygame.quit();
          return whiskers, trajectories
        
        elif (event.key == pygame.K_s) and event.mod & pygame.KMOD_CTRL:
          print "Saving to " + str(whiskers_file_name)
          save_state( whiskers_file_name, whiskers, trajectories, facehint );
          DIRTY = 0

        elif event.key == pygame.K_f: # scroll face hint
          options = ['left', 'right', 'top', 'bottom']
          assert facehint in options, "The current face hint is not one of the valid options."
          for i,e in enumerate( options ):
            if e == facehint:
              break
          facehint = options[(i+1)%len(options)]    

        elif event.key == pygame.K_SPACE:
          mode["auto"] = not mode["auto"]
        
        elif event.key == pygame.K_LEFT:
          bg,a = render(screen, im, current_whisker, state, bg, scale[0], inc=-1*mult, mode=mode, facehint=facehint)
        
        elif event.key == pygame.K_RIGHT:
          bg,a = render(screen, im, current_whisker, state, bg, scale[0], inc=1*mult, mode=mode, facehint=facehint)
         
        elif event.key == pygame.K_UP  :
          current_whisker += mult
          bg,a = render(screen, im, current_whisker, state, bg, scale[0],  inc=0, mode=mode, facehint=facehint)
        
        elif event.key == pygame.K_DOWN :
          current_whisker -= mult
          bg,a = render(screen, im, current_whisker, state, bg, scale[0],  inc=0, mode=mode, facehint=facehint)
        
        elif event.key == pygame.K_MINUS:
          if cursor_size > 1:
            cursor_size -= 1
        
        elif event.key == pygame.K_EQUALS:
          cursor_size += 1
        
        elif event.key == pygame.K_COMMA: # jump to beginning of movie
          bg,a = render(screen, im, current_whisker, state, bg, scale[0], goto=0, mode=mode, facehint=facehint)
                                                                      
        elif event.key == pygame.K_PERIOD: # jump to the end of the movie
          bg,a = render(screen, im, current_whisker, state, bg, scale[0], goto=N-1, mode=mode, facehint=facehint)
                                                                      
        elif event.key in (pygame.K_BACKSPACE,  pygame.K_DELETE):
          try:
            DIRTY = 1
            if event.mod & pygame.KMOD_SHIFT:
              #erase all trajectory labels
              for key in trajectories.keys():
                del trajectories[key]
            else:
              #get rid of the selected whisker segment
              segid = trajectories[ current_whisker ][ im.tell() ]
              del whiskers[im.tell()][segid]
              del trajectories[ current_whisker ][ im.tell() ] 
          except KeyError: #deleted when no whisker on frame
            pass
          bg,a = render(screen, im, current_whisker, state, bg, scale[0],  inc=0, mode=mode, facehint=facehint)

        elif event.key == pygame.K_RIGHTBRACKET:
          if event.mod & pygame.KMOD_SHIFT:
            And = lambda a,b: a and b
            none_missing = lambda fid: reduce(And, 
                                              map(lambda t: trajectories[t].has_key(fid) and whiskers.get(fid,{}).has_key(trajectories[t][fid]),
                                                  filter(lambda t:t>=0, 
                                                         trajectories.keys() ) ))
          else:
            none_missing = lambda fid: trajectories[ current_whisker ].has_key(fid) and whiskers.get(fid,{}).has_key(trajectories[ current_whisker ][fid])

          fid = im.tell()
          while none_missing(fid):
            fid += 1
          bg,a = render(screen, im, current_whisker, state, bg, scale[0], goto=min(fid,N-1), mode=mode, facehint=facehint)

        elif event.key == pygame.K_LEFTBRACKET:
          if event.mod & pygame.KMOD_SHIFT:
            And = lambda a,b: a and b
            none_missing = lambda fid: reduce(And, 
                                    map(lambda t: trajectories[t].has_key(fid) and whiskers.get(fid,{}).has_key(trajectories[t][fid]),
                                        filter(lambda t:t>=0, 
                                               trajectories.keys() ) ))
          else:
            none_missing = lambda fid: trajectories.get( current_whisker,{} ).has_key(fid) and whiskers.get(fid,{}).has_key(trajectories.get( current_whisker,{}).get(fid,-1));

          fid = im.tell()
          while none_missing(fid):
            fid -= 1
          bg,a = render(screen, im, current_whisker, state, bg, scale[0], goto=max(fid,0), mode=mode, facehint=facehint)

    textsurf = font.render( "frame: %d/%d"%(im.tell(),N-1),
                            1, (255,255,255) )
    rect = textsurf.get_rect().move(10,10)
    textbgrect.union_ip( rect )
    textbgrect = textbgrect.clip( bg.get_clip() )
    
    sbg = bg.subsurface( textbgrect ) 
    screen.blit( sbg, (10,10) )
    del(sbg)
    screen.blit( textsurf, (10,10) )
    
    textsurf = font.render( "whisker: %d"%(current_whisker),
                            1, (255,255,255) )
    ori = textsurf.get_rect()
    ori.topleft = rect.bottomleft
    rect = ori
    textbgrect.union_ip(ori)
    textbgrect = textbgrect.clip( bg.get_clip() )
    screen.blit( textsurf, ori ) 
    
    textsurf = font.render( "Face: %s"%(facehint),
                            1, (255,255,255) )
    ori = textsurf.get_rect()
    ori.topleft = rect.bottomleft
    rect = ori
    textbgrect.union_ip(ori)
    textbgrect = textbgrect.clip( bg.get_clip() )
    screen.blit( textsurf, ori ) 

    if mode["showcursorpos"] and cursor_rect:
      textsurf = font.render( 
                    "cursor: (%4d,%4d)"%tuple(map( lambda x: int(x/scale[0]), 
                                                   cursor_rect.center )),
                              1, (255,255,255) )
      ori = textsurf.get_rect()
      ori.topleft = rect.bottomleft
      rect = ori
      textbgrect.union_ip(ori)
      textbgrect = textbgrect.clip( bg.get_clip() )
      screen.blit( textsurf, ori ) 

    if mode["showfps"] and cursor_rect:
      textsurf = font.render( 
                    "FPS: %4d"%fps,
                    1, (255,255,255) )
      ori = textsurf.get_rect()
      ori.topleft = rect.bottomleft
      rect = ori
      textbgrect.union_ip(ori)
      textbgrect = textbgrect.clip( bg.get_clip() )
      screen.blit( textsurf, ori ) 

    if mode["auto"]:
      textsurf = font.render( "Auto Mode", 1, (255,255,0) )
      ori = textsurf.get_rect()
      ori.topleft = rect.bottomleft
      textbgrect.union_ip(ori)
      textbgrect = textbgrect.clip( bg.get_clip() )
      screen.blit( textsurf, ori ) 
      rect = ori

    pygame.display.flip()
    
    clock.tick(60)
  save_state( whiskers_file_name, whiskers, trajectories, facehint ); # <-- autosave
  return whiskers, trajectories

def render(screen, im, current_whisker, state, bg, scale, **kwargs):
  whiskers, trajectories, bar_centers = state
  kwargs['scale'] = scale
  mode = kwargs['mode']
  facehint = kwargs['facehint']
  del kwargs['facehint']
  bg,a = render_plane(screen, im, **kwargs )
  try:
    marked_seg = trajectories[ current_whisker ][ im.tell() ]
  except KeyError:
    marked_seg = None
  NCOLORS = len(trajectories)
  cmap = cm.gist_rainbow
  coloralt = (255,155, 55,255)
  if im.tell() in whiskers:
    wv = whiskers[im.tell()]
    ts = [(k,v[im.tell()]) for k,v in trajectories.iteritems() if im.tell() in v]
    seg2traj = {}
    for itraj,iseg in ts:
      seg2traj[iseg] = itraj
    for (iseg, w) in wv.iteritems():
      try:
        color = cmap( seg2traj[iseg] / (NCOLORS+0.001), bytes = True )
      except KeyError:
        color = coloralt
      draw_whisker(bg,w,color=color,color2=color ,scale=scale, mode=mode,facehint=facehint)
    if marked_seg in wv.keys():
      try:
        color = cmap( current_whisker / (NCOLORS+0.001), bytes = True )
      except KeyError:
        color = coloralt
      draw_whisker(bg,wv[marked_seg],color=color,color2=color ,scale=scale, mode=mode,facehint=facehint)
      draw_whisker(bg,wv[marked_seg],color=color,color2=coloralt ,scale=scale, thick = 2.5, selected = True,mode={"draw":0},facehint=facehint)
      
  if im.tell() in bar_centers:
    x,y = bar_centers[im.tell()]
    draw_bar(bg,x,y,radius=18,color=(255,255,255,255),scale=scale);
  
  screen.blit(bg, (0,0))
  return bg,a
  

import sys
if __name__=='__main__':
  try:
    usage = "usage: %prog [options] moviefile [dataprefix]"
    description = \
"""
A GUI utility for viewing and operating on high-speed
videos of mouse whiskers.  `moviefile`  should be the path to a TIF or SEQ
file.  Whisker shapes, whisker trajectories, and bar tracking data are
loaded/saved from files of the form: dataprefix.extension, where extension
is one of .trajectory, .whisker, or .bar. If no dataprefix is specified,
the movie's filename is used so that data files have the same name, and are
stored along side, the movie.  Data files are continuously autosaved as one
proofreads the movie, so be sure to back up your old data files to prevent
unwanted changes.  """

    parser = optparse.OptionParser(usage=usage,
                                   description=description)
    parser.add_option("-t","-T","--transpose",
                          dest    = "transpose",
                          action  = "store_true",
                          default = False,
                          help    = "transpose images [default: %default]");
    parser.add_option("--frame",
                          dest    = "startframe",
                          metavar = "N",
                          action  = "store",
                          type    = "int",
                          default = 0,
                          help    = "Start browsing at the N'th frame [default: %default]");
    parser.add_option("-c","--cursorsize",
                          dest    = "cursor_size",
                          action  = "store",
                          type    = "int",
                          default = 10,
                          help    = "Initial cursor radius in pixels [default: %default px]");
    parser.add_option("--cursorpos",
                          dest    = "show_cursor_pos",
                          action  = "store_true",
                          default = False,
                          help    = "Show cursor's center coordinates [default: %default]");
    parser.add_option("--adjuststripes",
                          dest    = "noadjuststripes",
                          action  = "store_false",
                          default = False,
                          help    = "Adjust odd-scanline bias [default: True]");
    parser.add_option("--noadjuststripes",
                          dest    = "noadjuststripes",
                          action  = "store_true",
                          default = False,
                          help    = "Prevent adjust odd-scanline bias [default: %default]");
    parser.add_option("--fps",
                          dest    = "show_fps",
                          action  = "store_true",
                          default = False,
                          help    = "Display rendering speed [default: %default]");
    parser.add_option("--label",
                          dest    = "prefix_label",
                          action  = "store",
                          type    = "string",
                          default = "",
                          help    = "Label to append to prefix when guessing data file name.  Set to empty quotes for no label. [default: nothing]");
    parser.add_option("--facehint",
                          dest    = "facehint",
                          action  = "store",
                          type    = "string",
                          default = "top",
                          help    = "Face hint indicating the side of the image the face is on.  May be 'left', 'right', 'top', 'bottom' or 'x,y' where x and y are the face location. Used for saving to .measurements files. [default: %default]");
    options, args = parser.parse_args()

    # figure out which file is which
    files = dict( [(os.path.splitext(f)[-1],f) for f in args] )
    movie_file = files.get('.seq',files.get('.tif',None))
    if movie_file is None:
      parser.error("Path to movie file is required.");

    # The prefix_label arg will only be used if not "" and no specific prefix is supplied (len(args)==1)
    if len(args) in [0,1]:
      if options.prefix_label:
        whiskers_file_name = os.path.splitext( args[-1] )[0] + "[%s]"%options.prefix_label
        print str(whiskers_file_name)
        del options.__dict__['prefix_label'] #consume
      else:
        whiskers_file_name = os.path.splitext( args[-1] )[0]
    else:
      del files[ os.path.splitext(movie_file)[-1] ]
      whiskers_file_name = files.values()

    whiskers = main(movie_file,
                    whiskers_file_name, 
                    **options.__dict__)
  except SystemExit:
    whiskerdata.close()
    pygame.quit()
  except:
    traceback.print_exc(file=sys.stderr)
    whiskerdata.close()
    pygame.quit()

