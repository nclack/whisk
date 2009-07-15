from SCons.Script import *
import os
import re

def change_label( name, newlabel ):
  if name.rfind(']')==-1:
    pfx,ext = os.path.splitext(name)
    return'%s[%s]%s'%(pfx,newlabel,ext)
  else:
    return re.sub( '(?<=\[).*(?=\]($|\.))',newlabel, name ) #substitutes the last label before the dot

def change_ext( node, newext ):
  prefix,ext = os.path.splitext( node.path )
  target = env.File( os.path.split(prefix)[1] + newext )
  return target
                                      
def emit_for_heal(target,source,env):
  name = str(target[0]) 
  prefix,ext = os.path.splitext( name )
  name = change_label( name , 'heal' )
  source = [ prefix + ".seq", prefix + ".whiskers" ]
  target = name + '.whiskers'
  return target,source

def labelled_commit_to_measurements(env, target, label):
  target = str(target[0])
  source = map( lambda ext: os.path.splitext(target)[0] + ext, [ ".trajectories", ".measurements" ] )
  target = change_label( target , label )
  target = os.path.splitext(target)[0] + ".measurements"
  return env.Command(target,source, "measure.py $SOURCES $TARGET")

def make_solver(command, label):
  def solver(env, target, suffix = ".measurements", src_suffix = ".measurements"):
    target = str(target[0])
    source = os.path.splitext(target)[0] + src_suffix
    target = change_label( target , label )
    target = os.path.splitext(target)[0] + suffix
    return env.Command(target,source, "%s $SOURCE $TARGET"%command)
  return solver

def dfs_consume_tuples(g,cur):
  while isinstance(cur,tuple):
    yield cur
    cur = g.next()
  def rest():
    yield cur
    for e in g:
      yield e
  yield rest()

def flatten(tree):
  if not isinstance(tree,tuple):
    yield tree
  for node in tree:
    if isinstance(node,tuple):
      for e in flatten(node):
        yield e
    else:
      yield node

def dfs_reduce(f,tree):
  def _dfs_reduce(f,tree, a = None):
    tree = iter(tree)
    for node in tree:
      if isinstance(node,tuple):
        return tuple ( map(lambda e: _dfs_reduce( f, e, a ),   #branch recursively
                          dfs_consume_tuples(tree,node) )
                    )
      else:
        a = node if a is None else f(a,node)                   #process node
    return a
  return flatten( _dfs_reduce(f,tree) )

def pipeline_standard(env, movie):
  builders = [ 
    movie                                                       ,
    env.Whisk                                                   ,
    lambda j: env.Heal( j )                                     ,
    lambda j: env.Measure( j )                                  ,
    lambda j: env.Classify( j )                                 ,
    lambda j: env.CommitToMeasurements( j, label = "autotraj" ) ,
    ( lambda j: env.IdentitySolver( j )                         , 
      lambda j: env.Summary( j )
    )                                                           ,
    lambda j: env.Summary( j )                             
  ]


  compose = lambda a,b: b(a)
  jobs = dfs_reduce( compose, builders )                         
  return jobs

def pipeline_curated(env, source):
  def measure_and_label(node):
    """ expects source to be a curated whiskers file 
        generated target is a measurements file
        returns target node
    """
    target  = change_ext( node, '.measurements' )
    sources = map( lambda e: change_ext(node, e),['.whiskers',
                                                  '.trajectories'] )
    out = env.Command( target, sources, "measure.py $SOURCES $TARGET" )
    return out

  builders = [
    source,
    measure_and_label,
    lambda j: env.Summary( j )                           
  ]
  compose = lambda a,b: b(a)
  jobs = reduce( compose, builders )
  return jobs

env  = Environment( 
  PX2MM = 0,
  BUILDERS = {
    'Whisk' : Builder(action = "whisk $SOURCE $TARGET --no-bar",
                      suffix  = '.whiskers',
                      src_suffix = '.seq'
                     ),
    'Bar'   : Builder(action = "whisk $SOURCE $TARGET --no-whisk",
                      suffix  = '.bar',
                      src_suffix = '.seq'
                     ),
    'Heal'  : Builder(action = "heal.py $SOURCES $TARGET",
                      emitter = emit_for_heal 
                     ),
    'Measure': Builder(action = "measure.py $SOURCE $TARGET",
                       suffix     = '.measurements',
                       src_suffix = '.whiskers'
                      ),
    'Classify': Builder(action = "classify.py $SOURCE $TARGET",
                        suffix = ".trajectories",
                        src_suffix = ".measurements"
                       ),
    'Summary': Builder(action = "summary.py $SOURCE $TARGET --px2mm=$PX2MM",
                       src_suffix = ".measurements",
                       suffix = ".png")
  }
)

env.AppendENVPath('PATH', os.getcwd())

env.AddMethod( labelled_commit_to_measurements, "CommitToMeasurements" )
env.AddMethod( make_solver("test_traj_solve_gray_areas", "grey_v0"), "IdentitySolver" )
env.AddMethod( pipeline_standard, "Pipeline" )
env.AddMethod( pipeline_curated,  "CuratedPipeline" ) 

Export('env')
