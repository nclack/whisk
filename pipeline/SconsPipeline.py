from SCons.Script import *
from ui.genetiff import Reader
from traj import MeasurementsTable
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

def thumbnail(target, source, env):
  import Image
  Image.fromarray( Reader(source[0].path)[0] ).save( target[0].path )

def length_v_score_plot(target,source,env):
  from pylab import plot, savefig, figure, close
  data = MeasurementsTable( source[0].path ).get_shape_table()
  f = figure()
  plot(data[:,0],data[:,1],'k.',markersize=1, hold=0)
  savefig( target[0].path )
  close(f)

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
  isbranch = lambda y: any( map( lambda x: isinstance(y,x), 
                                [tuple,
                                 list,
                                 SCons.Node.NodeList] 
                               ))
  if not isbranch(tree):
    yield tree
  else:
    for node in tree:
      if isbranch(node):
        for e in flatten(node):
          yield e
      else:
        yield node

def dfs_reduce(f,tree):
  """
  >>> a = (0,1,2,(3,4,(5,),(6,)),(7,8),9,0,1,2,(4,(5,),6),7)
  >>> f = lambda x,y: str(x)+str(y)
  >>> list(dfs_reduce(f,a))
  ['012345', '012346', '01278', '012901245', '012901246', '01290127']
  """
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
  res =  flatten(_dfs_reduce(f,tree))  
  #for e in  map(str,res):
  #  print e
  return res

def pipeline_standard(env, movie):
  def alter(j,subdir,ext):
    return env.File(j).Dir(subdir).File(  os.path.splitext(os.path.split(j.path)[-1])[0]+ext  ) 

  builders = [ 
    movie                                                       ,
    ( lambda j: env.Thumbnail(target = alter(j,'firstframe','[frm0].png'),
                              source = j) ,)                    ,
    env.Whisk                                                   ,
    env.Measure                                                 ,
    #( lambda j: env.LengthVScorePlot(target = alter(j[0],'LengthVScore','.png'),
    #                                 source = j[0]) ,)          ,
    env.Classify                                                ,
    #lambda j: env.CommitToMeasurements( j, label = "autotraj" ) ,
    #( env.IdentitySolver                                        , 
    #  env.MeasurementsAsMatlab                                  ,
    #  env.Summary
    #)                                                           ,
    ( env.MeasurementsAsMatlab, ),
    env.Summary                             
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
    ( env.MeasurementsAsMatlab, ),
    env.Summary                           
  ]
  compose = lambda a,b: b(a)
  jobs = dfs_reduce( compose, builders )
  return jobs

env  = Environment( 
  PX2MM = 0,
  BUILDERS = {
    'Thumbnail' : Builder(action = thumbnail),
    'LengthVScorePlot': Builder(action = length_v_score_plot),
    'Whisk' : Builder(action = "whisk $SOURCE $TARGET --no-bar",
                      suffix  = '.whiskers',
                      src_suffix = '.seq'
                     ),
    'Bar'   : Builder(action = "whisk $SOURCE $TARGET --no-whisk",
                      suffix  = '.bar',
                      src_suffix = '.seq'
                     ),
    'Heal'  : Builder(action = "test_merge_collisiontable_3 $SOURCE $TARGET",
                      suffix = '.whiskers',
                      src_suffix = '.whiskers'
                     ),
    'Measure': Builder(action = "measure.py $SOURCE $TARGET --face=$FACEHINT",
                       suffix     = '.measurements',
                       src_suffix = '.whiskers'
                      ),
    'MeasurementsAsMatlab': Builder(action = "measure.py $SOURCE $TARGET",
                                    suffix = '.mat',
                                    src_suffix = '.measurements'
                                   ),
    'Classify': Builder(action = "test_classify_1 $SOURCE $TARGET $FACEHINT",
                        suffix = { '.measurements' : "[autotraj].measurements" },
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

if __name__=='__main__':
  import doctest
  doctest.testmod()
