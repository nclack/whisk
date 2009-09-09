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

def pipeline_production(env, movie):
  def alter(j,subdir,ext):
    return env.File(j).Dir(subdir).File(  os.path.splitext(os.path.split(j.path)[-1])[0]+ext  ) 
  
  builders = [ 
    movie                                                       ,
    env.Whisk                                                   ,
	(env.Precious,),
#	(env.Bar),
    env.Measure                                                 ,
    env.Classify                                                ,
	env.HmmLRTimeSolver,
    env.GreyAreaSolver,       
    (env.MeasurementsAsMatlab,),
    (env.MeasurementsAsTrajectories,),
    env.Summary
  ]

  compose = lambda a,b: b(a)
  jobs = dfs_reduce( compose, builders )                         
  return jobs

def pipeline_standard(env, movie):
  def alter(j,subdir,ext):
    return env.File(j).Dir(subdir).File(  os.path.splitext(os.path.split(j.path)[-1])[0]+ext  ) 

  builders = [ 
    movie,
#    env.Whisk,
    ( env.Bar, ),
    lambda j: change_ext(j,'.whiskers'),
    env.Measure,
    env.Classify,
    ( env.GreyAreaSolver, 
      env.Summary
    ) ,                    
    ( env.HmmLRSolver,
      ( env.GreyAreaSolver, 
        ( env.MeasurementsAsTrajectories,),
        env.Summary
      ) ,
      ( env.MeasurementsAsTrajectories,),
      env.Summary 
    ),
    ( env.HmmLRTimeSolver,
      ( env.GreyAreaSolver, 
        ( env.MeasurementsAsTrajectories,),
        env.Summary
      ) ,                    
      ( env.MeasurementsAsTrajectories,),
      env.Summary 
    ),
#   ( env.HmmLRDelTimeSolver,
#     ( env.GreyAreaSolver, 
#       ( env.MeasurementsAsTrajectories,),
#       env.Summary
#     ) ,                    
#     ( env.MeasurementsAsTrajectories,),
#     env.Summary ),
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
    node = node[0]
    target  = change_ext( node, '[traj].measurements' )
    sources = map( lambda e: change_ext(node, e),['.measurements',
                                                  '.trajectories'] )
    out = env.Command( target, sources, "measure.py $SOURCES $TARGET" )
    return out

  builders = [
    source,  
    env.Measure,
    measure_and_label,
    ( env.MeasurementsAsMatlab, ),
    ( env.SummaryPDF ),
    env.Summary
  ]
  compose = lambda a,b: b(a)
  jobs = dfs_reduce( compose, builders )
  return jobs

def lit(s):
  return lambda env,sources: s

def whisk_generator( source, target, env, for_signature ):
	if not target[0].exists():
		return Action("whisk $SOURCE $TARGET --no-bar")
	else:
		return Action("")

def whisk_action( source, target, env ):
	#import pdb; pdb.set_trace()
	if not target[0].exists():
		env.Command(target,source,"whisk $SOURCE $TARGET --no-bar")
		
env  = Environment( 
  PX2MM = 0,
  BUILDERS = {
    'Thumbnail' : Builder(action = thumbnail),
    'LengthVScorePlot': Builder(action = length_v_score_plot),
    'Whisk' : Builder(generator = whisk_generator, #action = whisk_action, #"whisk $SOURCE $TARGET --no-bar",
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
    'Measure': Builder(action = "test_measure_1 $SOURCE $TARGET --face $FACEHINT",
                       suffix     = '.measurements',
                       src_suffix = '.whiskers'
                      ),
    'MeasureOld': Builder(action = "measure.py $SOURCE $TARGET --face=$FACEHINT",
                       suffix     = '.measurements',
                       src_suffix = '.whiskers'
                      ),
    'MeasurementsAsMatlab': Builder(action = "measure.py $SOURCE $TARGET",
                                    suffix = '.mat',
                                    src_suffix = '.measurements'
                                   ),
    'MeasurementsAsTrajectories': Builder(action = lambda source,target,env: 0 if MeasurementsTable(source[0].path).save_trajectories(target[0].path,excludes=[-1]) else 1,
                                          suffix = '.trajectories',
                                          src_suffix = '.measurements'),
    'Classify': Builder(action = "test_classify_1 $SOURCE $TARGET $FACEHINT -n $WHISKER_COUNT",
                        suffix = { '.measurements' : "[autotraj].measurements" },
                        src_suffix = ".measurements"
                       ),
    'Summary': Builder(action = "summary.py $SOURCE $TARGET --px2mm=$PX2MM",
                       src_suffix = ".measurements",
                       suffix = ".png"),
    'SummaryPDF': Builder(action = "summary.py $SOURCE $TARGET --px2mm=$PX2MM",
                       src_suffix = ".measurements",
                       suffix = ".pdf"),
    'GreyAreaSolver': Builder(action = "test_traj_solve_gray_areas $SOURCE $TARGET",
                              src_suffix = ".measurements",
                              suffix = lit( "[grey_v0].measurements") ),
    'HmmLRSolver': Builder(action = "test_hmm_reclassify_1 -n $WHISKER_COUNT $SOURCE $TARGET",
                              src_suffix = ".measurements",
                              suffix = lit( "[hmm-lr].measurements")),
    'HmmLRDelSolver': Builder(action = "test_hmm_reclassify_2 -n $WHISKER_COUNT $SOURCE $TARGET",
                              src_suffix = ".measurements",
                              suffix = lit( "[hmm-lrdel].measurements")),
    'HmmLRTimeSolver': Builder(action = "test_hmm_reclassify_3 -n $WHISKER_COUNT $SOURCE $TARGET",
                              src_suffix = ".measurements",
                              suffix = lit( "[hmm-lr-time].measurements")),
    'HmmLRDelTimeSolver': Builder(action = "test_hmm_reclassify_4 -n $WHISKER_COUNT $SOURCE $TARGET",
                              src_suffix = ".measurements",
                              suffix = lit( "[hmm-lrdel-time].measurements")),
  }
)

env.Decider('timestamp-newer')
env.AppendENVPath('PATH', os.getcwd())
env['WHISKER_COUNT'] = -1  # a count <1 tries to measure the count for each movie
                           # a count >= 1 will identify that many whiskers in each movie

env.AddMethod( pipeline_production, "Pipeline" )
env.AddMethod( pipeline_curated,  "CuratedPipeline" ) 

Export('env')

if __name__=='__main__':
  import doctest
  doctest.testmod()
