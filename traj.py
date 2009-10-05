#!/usr/bin/env python
import os
from ctypes import *
from ctypes.util import find_library
import numpy
from numpy import zeros, double, fabs, ndarray, array

import warnings

import pdb

os.environ['PATH'] += r';.\\'
ctraj = cdll.LoadLibrary( find_library('traj') )
if not ctraj:
  ctraj = cdll.LoadLibrary( find_library('whisk') )
  if not ctraj:
    raise ImportError("Can not load whisk or traj shared library");


class cMeasurements(Structure):
  """ Proxy for Measurements struct. 
  >>> from numpy.random import rand
  >>> data = rand(20,10)
  >>> table = ctraj.Measurements_Table_From_Doubles( data.ctypes.data_as(POINTER(c_double)), 20, 10 )
  >>> table[0].n
  7
  >>> table   # doctest:+ELLIPSIS
  <ctypes.LP_cMeasurements object at ...>
  """
  _fields_ = [("row",            c_int               ),
              ("fid",            c_int               ),                                                           
              ("wid",            c_int               ),                                                           
              ("state",          c_int               ),                                                           
              ("face_x",         c_int               ),             #// used in ordering whiskers on the face...roughly, the center of the face                                              
              ("face_y",         c_int               ),             #//                                      ...does not need to be in image                                                 
              ("col_follicle_x", c_int               ),             #// index of the column corresponding to the folicle x position                                                          
              ("col_follicle_y", c_int               ),             #// index of the column corresponding to the folicle y position                                                          
              ("valid_velocity", c_int               ),                                                           
              ("n",              c_int               ),                                                           
              ("data",           POINTER( c_double ) ),             # // array of n elements                      
              ("velocity",       POINTER( c_double ) )]             # // array of n elements - change in data/time

class cDistributions(Structure):
  """
  >>> this = ctraj.Alloc_Distributions( 32, 8, 4 )
  >>> this                                          # doctest:+ELLIPSIS 
  <ctypes.LP_cDistributions object at ...>
  >>> ctraj.Free_Distributions( this )
  """
  _fields_ = [("n_measures",   c_int               ),
              ("n_states",     c_int               ),
              ("n_bins",       c_int               ),
              ("bin_min",      POINTER( c_double ) ),   # // array of n_measures elements                                                          
              ("bin_delta",    POINTER( c_double ) ),   # // array of n_measures elements                                                          
              ("data",         POINTER( c_double ) )]   # // array of holding histogram information with dimensions (n_bins,n_measures,n_states)
  def asarray(self):
    d = zeros( (self.n_states, self.n_measures, self.n_bins) )
    ctraj.Copy_Distribution_To_Doubles( byref(self), d.ctypes.data_as( POINTER(c_double) ) )
    return d
  
  def bins_as_array(self):
    b = zeros( (self.n_measures, self.n_bins) )
    ctraj.Distributions_Bins_To_Doubles( byref(self), b.ctypes.data_as( POINTER(c_double) ) )
    return b

class MeasurementsTable(object):
  """
  >>> data = numpy.load('data/testing/seq140[autotraj].npy')
  >>> table = MeasurementsTable(data)
  >>> table._measurements[0].n
  8
  """
  def __init__(self, datasource):
    """
    Load table from numpy array or from a file.

    >>> table = MeasurementsTable( zeros((500,5)) )

    >>> table = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    """
    object.__init__(self)
    self._measurements = None
    self._nrows = 0
    self._sort_state = None
    self._free_measurements = ctraj.Free_Measurements_Table
    if isinstance(datasource,str):
      self._load(datasource)
    else:
      self._measurements = ctraj.Measurements_Table_From_Doubles( 
                              datasource.ctypes.data_as( POINTER(c_double) ),   # data buffer       
                              datasource.shape[0],                              # number of rows    
                              datasource.shape[1] )                             # number of columns 
      self._nrows = datasource.shape[0]

  def __del__(self):
    """
    >>> table = MeasurementsTable( zeros((500,5)) )
    >>> del table
    """
    self._free_measurements(self._measurements)
    #ctraj.Free_Measurements_Table(self._measurements)

  def asarray(self):
    """  
    >>> from numpy.random import rand
    >>> data = rand(200,10)
    >>> table = MeasurementsTable(data)
    >>> shape = table.asarray()
    >>> print shape.shape
    (200, 10)
    >>> print (shape[:,3:]==data[:,3:]).all()
    True
    """
    data = zeros( (self._nrows, self._measurements[0].n+3), dtype=double )
    ctraj.Measurements_Table_Data_To_Doubles(self._measurements, 
                                             self._nrows, 
                                             data.ctypes.data_as( POINTER( c_double ))
                                            );
    return data

  def get_trajectories(self):
    """
    >>> table = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    >>> traj  = table.get_trajectories()
    >>> max(traj.keys())
    3
    >>> traj.has_key(-1)
    False
    """
    data = self.asarray()
    t = {}
    for row in data:
      r = map(int,row[:3])
      t.setdefault( r[0],{} ).setdefault( r[1], r[2] ) 
    if -1 in t.keys():
      del t[-1]
    return t

  def save_trajectories(self, filename, excludes=[]):
    """  Saves to a trajectories file.

    >>> table = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    >>> table.save_trajectories( "data/testing/trash.trajectories" ) # doctest:+ELLIPSIS 
    <...MeasurementsTable object at ...>
    """
    trajectories = self.get_trajectories()
    f = open( filename, 'w' )
    for k,v in trajectories.iteritems():
      if not k in excludes:
        for s,t in v.iteritems():
          print >> f, '%d,%d,%d'%(k,s,t)
    return self

  def load_trajectories(self,filename ):
    """  Loads trajectories and saves them to the table.
    Trajectory id's correspond to the `state` label.

    >>> table = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    >>> table.save_trajectories( "data/testing/trash.trajectories" )  # doctest:+ELLIPSIS 
    <...MeasurementsTable object at ...>
    >>> table.load_trajectories( "data/testing/trash.trajectories" )  # doctest:+ELLIPSIS 
    <...MeasurementsTable object at ...>
    """
    trajectories = {}
    f = open( filename, 'r' )

    cur = 0;
    for line in f:
      t = [int(x) for x in line.split(',')[:3]]
      if not t[0] in trajectories:
        trajectories[t[0]] = {}
      trajectories[ t[0] ][ t[1] ] = t[2];

    self.commit_trajectories( trajectories )
    return self

  def commit_trajectories(self,traj):
    """
    >>> traj = {0: {0:0,1:0}, 1: {0:1,1:1} }
    >>> table = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    >>> mn,mx = table.commit_trajectories(traj).get_state_range()
    >>> mn
    0
    >>> mx
    1
    >>> lentraj = lambda x: len(table.get_shape_data(x))
    >>> lentraj(0)
    2
    >>> add = lambda a,b:a+b
    >>> table._nrows == reduce(add, map(lentraj,xrange(mn-1,mx+1)))
    True
    """
    inv = {}
    for tid,t in traj.iteritems():
      for k in t.iteritems():
        inv[k] = tid  

    for i in xrange(self._nrows):  #update new
      row = self._measurements[i]
      s = inv.get( (row.fid,row.wid) )
      row.state = s if (not s is None) else -1 

    return self

  def get_state_range(self):
    """
    >>> data = numpy.load('data/testing/seq140[autotraj].npy')
    >>> mn,mx = MeasurementsTable(data).update_velocities().get_state_range()
    >>> mn
    0
    >>> mx
    3
    """ 
    mn,mx = c_int(),c_int()
    sorted = (not self._sort_state is None ) and \
             ("state" in self._sort_state  )
    n = ctraj._count_n_states(self._measurements,
                              self._nrows,
                              sorted,
                              byref(mn),
                              byref(mx))
    f = lambda x: x.value if x.value >=0 else 0
    return map(f,[mn,mx])

  def iter_state(self):
    """
    >>> table = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    >>> for i in table.update_velocities().iter_state():
    ...     print i
    ...     
    0
    1
    2
    3
    """
    mn,mx = self.get_state_range()
    return xrange(mn,mx+1)

  def get_shape_table(self):
    """  
    >>> from numpy.random import rand
    >>> data = rand(200,10)
    >>> table = MeasurementsTable(data)
    >>> shape = table.get_shape_table()
    """
    shape = zeros( (self._nrows, self._measurements[0].n), dtype=double )
    ctraj.Measurements_Table_Copy_Shape_Data( self._measurements, 
                                              self._nrows, 
                                              shape.ctypes.data_as( POINTER(c_double) ) )
    return shape

  def get_time_and_mask(self, state, rows = None):
    """
    Returns `time` and `valid velocity` mask for selected state.  
    Order of results is determined by the table's sort order.

    >>> data = numpy.load('data/testing/seq140[autotraj].npy')
    >>> table = MeasurementsTable(data).update_velocities()
    >>> time,mask = table.get_time_and_mask(1)
    """
    if rows is None:
      rows = ctraj.Measurements_Table_Size_Select_State( self._measurements, self._nrows, int(state) )
    time = zeros( rows, dtype = double )
    mask = zeros( rows, dtype = int    )
    ctraj.Measurements_Table_Select_Time_And_Mask_By_State( self._measurements, 
                                                            self._nrows,
                                                            int(state),
                                                            time.ctypes.data_as (POINTER( c_double )), 
                                                            mask.ctypes.data_as (POINTER( c_int    )) )
    return time,mask

  def get_velocities(self, state, rows = None):
    """
    Returns velocity for selected state.  
    Order of results is determined by the table's sort order.

    >>> data = numpy.load('data/testing/seq140[autotraj].npy')
    >>> table = MeasurementsTable(data)
    >>> table.update_velocities() # doctest:+ELLIPSIS 
    <...MeasurementsTable object at ...>
    >>> velocities = table.get_velocities(1)
    """
    if rows is None:
      rows = ctraj.Measurements_Table_Size_Select_State( self._measurements, self._nrows, int(state) )
    vel  = zeros( (rows, self._measurements[0].n ), dtype = double )
    ctraj.Measurements_Table_Select_Velocities_By_State( self._measurements, 
                                                         self._nrows,
                                                         int(state),
                                                         vel.ctypes.data_as (POINTER( c_double )) )
    return vel
  
  def get_shape_data(self, state, rows = None):
    """
    Returns shape data for selected state.  
    Order of results is determined by the table's sort order.

    >>> data = numpy.load('data/testing/seq140[autotraj].npy')
    >>> table = MeasurementsTable(data).update_velocities()
    >>> shape = table.get_shape_data(1)
    
    >>> table = MeasurementsTable('data/testing/seq140[autotraj].measurements').update_velocities()
    >>> shape = table.get_shape_data(1)
    """
    if rows is None:
      rows = ctraj.Measurements_Table_Size_Select_State( self._measurements, self._nrows, int(state) )
    data  = zeros( (rows, self._measurements[0].n ), dtype = double )
    ctraj.Measurements_Table_Select_Shape_By_State( self._measurements, 
                                                    self._nrows,
                                                    int(state),
                                                    data.ctypes.data_as (POINTER( c_double )) )
    return data

  def get_data(self, state, rows = None ):
    """
    Returns time, shape, velocity and velocity_valid  data for selected state.  
    Order of results is determined by the table's sort order.

    >>> data = numpy.load('data/testing/seq140[autotraj].npy')
    >>> table = MeasurementsTable(data).update_velocities()
    >>> time,shp,vel,mask = table.get_data(1)
    """
    if rows is None:
      time,mask = self.get_time_and_mask(state) 
    else:
      time,mask = self.get_time_and_mask(state, rows = rows) 
    vel = self.get_velocities(state, rows = time.shape[0] )
    shp = self.get_shape_data(state, rows = time.shape[0] )
    return time, shp, vel, mask

  def get_velocities_table(self):
    """
    >>> data = numpy.load('data/testing/seq140[autotraj].npy')
    >>> table = MeasurementsTable(data).update_velocities()
    >>> vel = table.get_velocities_table()
    """
    vel = zeros( (self._nrows, self._measurements[0].n), dtype=double )
    ctraj.Measurements_Table_Copy_Velocities( self._measurements, 
                                              self._nrows, 
                                              vel.ctypes.data_as( POINTER(c_double) ) )
    return vel

  def set_constant_face_position(self, x, y):
    """
    >>> table = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    >>> table = table.set_constant_face_position( -100, 100 )
    >>> table._measurements[0].face_x
    -100
    >>> table._measurements[0].face_y
    100
    """
    ctraj.Measurements_Table_Set_Constant_Face_Position( self._measurements, self._nrows, x, y )
    return self

  def set_follicle_position_column(self, ix, iy):
    """
    >>> table = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    >>> table = table.set_follicle_position_column( 7, 8 ) 
    >>> table._measurements[0].col_follicle_x
    7
    >>> table._measurements[0].col_follicle_y
    8
    """
    ctraj.Measurements_Table_Set_Follicle_Position_Indices( self._measurements, self._nrows, ix, iy )
    return self

  def sort_by_state_time(self):
    """
    >>> data = numpy.load('data/testing/seq140[autotraj].npy')
    >>> table = MeasurementsTable(data).sort_by_state_time() 
    >>> table._measurements[0].state
    -1
    >>> table._measurements[0].fid
    0
    >>> table._measurements[table._nrows].state
    0
    >>> table._measurements[table._nrows-1].state
    3
    >>> table._measurements[table._nrows-1].fid
    4598
    >>> table._sort_state
    'state,time'
    """
    sortstate = "state,time"
    if self._sort_state != sortstate:
      ctraj.Sort_Measurements_Table_State_Time( self._measurements, self._nrows )
      self._sort_state = sortstate
    return self

  def sort_by_time(self):
    """
    >>> data = numpy.load('data/testing/seq140[autotraj].npy')
    >>> table = MeasurementsTable(data).sort_by_time() 
    >>> table._measurements[0].fid
    0
    >>> table._measurements[table._nrows-1].fid
    4598
    >>> table._sort_state
    'time'
    """
    sortstate = "time"
    if self._sort_state != sortstate:
      ctraj.Sort_Measurements_Table_Time( self._measurements, self._nrows )
      self._sort_state = sortstate
    return self

  def sort_by_time_face(self):
    """
    >>> table = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    >>> table = table.set_constant_face_position( -100, 100 ).set_follicle_position_column( 7, 8 )
    >>> table = table.sort_by_time_face()
    >>> table._measurements[0].fid
    0
    >>> table._measurements[table._nrows-1].fid
    4598
    >>> table._sort_state
    'time,face'
    """
    sortstate = "time,face"
    if(self._sort_state != sortstate):
      ctraj.Sort_Measurements_Table_Time_Face( self._measurements, self._nrows )
      self._sort_state = sortstate
    return self

  def update_velocities(self):
    """
    >>> data = numpy.load('data/testing/seq140[autotraj].npy')
    >>> table = MeasurementsTable(data).update_velocities() 
    >>> vel = table.get_velocities_table()
    """
    self.sort_by_state_time()
    ctraj.Measurements_Table_Compute_Velocities( self._measurements, self._nrows )
    return self

  def save(self, filename):
    """
    >>> data = numpy.load('data/testing/seq140[autotraj].npy')
    >>> table = MeasurementsTable(data).update_velocities()
    >>> table.save( "data/testing/trash.measurements" )    # doctest:+ELLIPSIS 
    <...MeasurementsTable object at ...>
    """
    ctraj.Measurements_Table_To_Filename( filename, self._measurements, self._nrows )
    return self

  def save_to_matlab_file(self, filename, format = '5'):
    """
    Saves shape measurements to Matlab's .mat format.

    This uses the `scipy.io.matlab.savemat` function.  See that functions documentation for
    details on input options.

    >>> table = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    >>> table.save_to_matlab_file( "data/testing/trash.mat" )    # doctest:+ELLIPSIS 
    <...MeasurementsTable object at ...>
    """
    from scipy.io.matlab import savemat
    kwargs = locals().copy()
    for k in [ 'self', 'savemat', 'filename' ]:
      del kwargs[k]
    savemat( filename, 
            { 'measurements': self.asarray() }, 
            **kwargs)
    return self

  def _load(self, filename):
    """
    Loads table from a saved file.

    >>> table = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    """
    if not os.path.exists(filename):
      raise IOError("Could not find file %s"%filename)
    nrows = c_int()
    if self._measurements:
      ctraj.Free_Measurements_Table( self._measurements )
    self._measurements = ctraj.Measurements_Table_From_Filename( filename, byref(nrows) )
    self._nrows = nrows.value
    self._sort_state = None #unknown
    return self
  
  def diff_identity(self, table):
    """
    Searches two tables for different identity assignments and returns
    a list of frames where a difference was found.  Ideally, the two
    tables would have been derived from the same movie.

    If the tables are identical, an empty list is returned:

    >>> A = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    >>> B = MeasurementsTable( "data/testing/seq140[autotraj].measurements" )
    >>> A.diff_identity(B)
    []

    The differences are not symmetric.  This is because "junk" states in the
    table on the left-hand side (`self`) are ignored.

    >>> B = MeasurementsTable( "data/testing/seq140[solve].measurements" )
    >>> len(B.diff_identity(A))
    69
    >>> len(A.diff_identity(B))
    25
    """
    nframes = c_int(0)
    frames = ctraj.Measurements_Tables_Get_Diff_Frames( self._measurements, self._nrows, 
                                                        table._measurements, table._nrows, 
                                                        byref(nframes) )
    return [frames[i] for i in xrange(nframes.value)]

class Distributions(object):
  def __init__(self, table = None, nbins = 32):
    """
    Create an empty Distributions object:
    
    >>> dists = Distributions()
    
    Initialize Distributions using a MeasurementTable:
    
    >>> import numpy
    >>> data = numpy.load( "data/testing/seq140[autotraj].npy" )
    >>> table = MeasurementsTable(data)

    >>> table = MeasurementsTable('data/testing/seq140[autotraj].measurements')
    >>> dists = Distributions(table.update_velocities())         # doctest:+ELLIPSIS  
    ...
    """
    object.__init__(self)
    self._free_distributions = ctraj.Free_Distributions
    self._shp = None
    self._vel = None
    if not table is None:
      self.build(table, nbins)  
  
  def __del__(self):
    if self._shp:
      self._free_distributions( self._shp )
    if self._vel:
      self._free_distributions( self._vel )
  
  def build(self, table, nbins = 32):
    """
    >>> dists = Distributions()
    >>> table = MeasurementsTable('data/testing/seq140[autotraj].measurements')
    >>> dists.build(table)                   # doctest:+ELLIPSIS  
    <...Distributions object at ...>
    """
    assert isinstance(table,MeasurementsTable), "Wrong type for table."
    table.sort_by_state_time()
    self._shp = ctraj.Build_Distributions         ( table._measurements, table._nrows, nbins )
    ctraj.Distributions_Normalize( self._shp )
    ctraj.Distributions_Apply_Log2( self._shp )
    table.update_velocities()
    self._vel = ctraj.Build_Velocity_Distributions( table._measurements, table._nrows, nbins ) #changes table's sort order
    ctraj.Distributions_Normalize( self._vel )
    ctraj.Distributions_Apply_Log2( self._vel )
    table._sort_state = 'time'
    return self
  
  def velocities(self):
    """
    >>> dists = Distributions( MeasurementsTable('data/testing/seq140[autotraj].measurements') )
    >>> vbins, v = dists.velocities()
    """
    return self._vel[0].bins_as_array(), self._vel[0].asarray()  
  
  def shapes(self):
    """
    >>> dists = Distributions( MeasurementsTable('data/testing/seq140[autotraj].measurements') )
    >>> sbins, s = dists.shapes()
    """
    return self._shp[0].bins_as_array(), self._shp[0].asarray()  

def solve( table ):
  ctraj.Solve( table._measurements, table._nrows, 32, 8096 )
  table._sort_state = "time"
  return table

def batch_make_measurements(sourcepath, ext = '*.seq', label = 'curated'):
  """
  To update/remake a measurements table, delete the *.npy and *.measurements
  files in the `sourcepath`.
  """
  warnings.simplefilter("ignore")
  from glob import glob
  from ui.whiskerdata import load_trajectories
  from trace import Load_Whiskers
  import summary
  warnings.simplefilter("default")

  def get_summary_data( filename, whiskers, trajectories ):
    if os.path.exists(filename):
      data = numpy.load(filename)
    else:
      data = array(list( summary.features(whiskers) ))
      numpy.save( filename, data )
      return data
    return summary.commit_traj_to_data_table( trajectories, data )

  for name in glob( os.path.join( sourcepath, ext ) ):
    root,ext = os.path.splitext( name )
    prefix = root + '[%s]'%label
    if not os.path.exists( prefix + '.measurements' ):
      t,tid = load_trajectories( prefix + '.trajectories' )
      print prefix
      print t.keys()
      w = Load_Whiskers( prefix + '.whiskers' ) 
      data = get_summary_data( prefix + '.npy', w, t )
      MeasurementsTable( data ).update_velocities().save( prefix + '.measurements' )

#
# Testing
#

import unittest
import doctest

class Tests_MeasurementsTable(unittest.TestCase):
  """
  Tests for Measurements table functions.
  There are different ways of setting up a MeasurementsTable.
  This test case is subclassed to handle different setups.
  """
  def test_LoadedDataValid(self):
    self.failUnless( self.data[:,0].min() == -1 )
    self.failUnless( self.data[:,0].max() > 1   )

  def test_TableInstanced(self):
    self.failUnlessEqual( self.table._nrows, self.data.shape[0] ) 
    self.failUnlessEqual( self.table._measurements[0].n, self.data.shape[1]-3 )

  def test_GetShapeMeasures(self):
    shape = self.table.get_shape_table()
    self.failUnlessEqual( shape.shape[0], self.data.shape[0] )
    self.failUnlessEqual( shape.shape[1], self.data.shape[1]-3 )
    #self.failUnlessAlmostEqual( ((self.data[:,3:] - shape)**2).sum(), 0.0, 7 ) #can't fix right now...not important...

  def test_SortByStateAndTime(self):
    self.table.sort_by_state_time()
    for i in xrange(1, self.table._nrows):
        self.failIf( (self.table._measurements[i-1].state > self.table._measurements[i].state) and 
                     (self.table._measurements[i-1].fid   > self.table._measurements[i].fid) )
    for i in xrange(1, self.table._nrows):
        self.failIf( self.table._measurements[i-1].state  > self.table._measurements[i].state )
  
  def test_SortByTime(self):
    self.table.sort_by_time()
    for i in xrange(1, self.table._nrows):
        self.failIf( self.table._measurements[i-1].fid  > self.table._measurements[i].fid )

  def test_ComputeVelocities_SomeVelocitiesAreValid(self):
    self.table.update_velocities()    
    any = lambda x,y: x or y  
    is_row_valid = lambda i: self.table._measurements[i].valid_velocity                                           
    self.failUnless( reduce( any, map( is_row_valid, xrange(self.table._nrows) )) )

  def test_SizeSelectVelocities_StatesPartitionTable(self):
    self.table.update_velocities()
    cnt = 0
    for state in map(int, set(self.data[:,0])):
      cnt += ctraj.Measurements_Table_Size_Select_State( self.table._measurements, self.table._nrows, c_int(state) )
    self.failUnlessEqual( cnt, self.data.shape[0] )

  def test_SelectDataByState(self):
    """
    Scan through all states and perform selects.
    No checks, just demonstates functionality is present.
    """
    self.table.update_velocities()
    states = set( self.data[:,0] )
    states.discard(-1)
    time,mask = self.table.get_time_and_mask(0) #given the input data, this should be the same for all states
    for s in states:
      vel = self.table.get_velocities(s, rows = time.shape[0] )
      shp = self.table.get_shape_data(s, rows = time.shape[0] )
  
  def test_LoadNonexistentFile(self):
    filename = 'nonexistent.measurement'
    self.failIf( os.path.exists(filename) )
    self.failUnlessRaises(IOError, MeasurementsTable, filename)

class Tests_MeasurementsTable_FromDoubles( Tests_MeasurementsTable ):
  def setUp(self):
    self.data = numpy.load('data/testing/seq140[autotraj].npy')
    self.table = MeasurementsTable(self.data)
  
  def test_VelocitiesInitiallyZero(self):
    vel = self.table.get_velocities_table()
    self.failUnlessAlmostEqual( vel.sum(), 0.0)


class Tests_MeasurementsTable_FromFile( Tests_MeasurementsTable ):
  def setUp(self):
    self.data = numpy.load('data/testing/seq140[autotraj].npy')
    self.table = MeasurementsTable('data/testing/seq140[autotraj].measurements')

class Tests_Distributions(unittest.TestCase):
  def setUp(self):
    self.data = numpy.load('data/testing/seq140[autotraj].npy')
    self.table = MeasurementsTable('data/testing/seq140[autotraj].measurements')
    self.dists = Distributions(self.table)

  def test_PostBuildSortStateIsTime(self):
    self.failUnlessEqual( self.table._sort_state, 'time' )

  def test_InitializationTypeCheck(self):
    self.failUnlessRaises( AssertionError, Distributions, zeros(10) )

  def test_ShapeDistributionsAsArray(self):
    d = self.dists._shp[0].asarray()
    self.failUnless( isinstance( d, numpy.ndarray) )
    nstates, nmeasures, nbins = d.shape
    self.failUnlessEqual( nstates,   self.dists._shp[0].n_states )
    self.failUnlessEqual( nmeasures, self.dists._shp[0].n_measures )
    self.failUnlessEqual( nbins,     self.dists._shp[0].n_bins )

  def test_ShapeDistributionBinsAsArray(self):
    bins = self.dists._shp[0].bins_as_array()
    self.failUnless( isinstance( bins, numpy.ndarray) )
    nmeasures, nbins = bins.shape
    self.failUnlessEqual( nmeasures, self.dists._shp[0].n_measures )
    self.failUnlessEqual( nbins,     self.dists._shp[0].n_bins )
  
  def test_VelocityDistributionsAsArray(self):
    d = self.dists._vel[0].asarray()
    self.failUnless( isinstance( d, numpy.ndarray) )
    nstates, nmeasures, nbins = d.shape
    self.failUnlessEqual( nstates,   self.dists._vel[0].n_states )
    self.failUnlessEqual( nmeasures, self.dists._vel[0].n_measures )
    self.failUnlessEqual( nbins,     self.dists._vel[0].n_bins )
  
  def test_VelocityDistributionBinsAsArray(self):
    bins = self.dists._vel[0].bins_as_array()
    self.failUnless( isinstance( bins, numpy.ndarray) )
    nmeasures, nbins = bins.shape
    self.failUnlessEqual( nmeasures, self.dists._vel[0].n_measures )
    self.failUnlessEqual( nbins,     self.dists._vel[0].n_bins )

#
# Declarations 
#
ctraj.Measurements_Table_From_Doubles.restype = POINTER(cMeasurements)
ctraj.Measurements_Table_From_Doubles.argtypes = [
  POINTER( c_double ), # data buffer
  c_int,               # number of rows
  c_int ]              # number of columns

ctraj.Measurements_Table_Copy_Shape_Data.restype = None
ctraj.Measurements_Table_Copy_Shape_Data.argtypes = [
  POINTER( cMeasurements ), # the table (the source)
  c_int,                    # number of rows
  POINTER( c_double ) ]     # destination

ctraj.Measurements_Table_Copy_Velocities.restype = None
ctraj.Measurements_Table_Copy_Velocities.argtypes = [
  POINTER( cMeasurements ), # the table (the source)
  c_int,                    # number of rows
  POINTER( c_double ) ]     # destination

ctraj.Measurements_Table_From_Filename.restype = POINTER(cMeasurements)
ctraj.Measurements_Table_From_Filename.argtypes = [
  POINTER( c_char ),
  POINTER( c_int  ) ]

ctraj.Alloc_Distributions.restype = POINTER(cDistributions)
ctraj.Alloc_Distributions.argtypes = [
  c_int,  # n_bins
  c_int,  # n_measures
  c_int ] # n_states

ctraj.Free_Distributions.restype = None
ctraj.Free_Distributions.argtypes = [ POINTER(cDistributions) ]

ctraj.Build_Distributions.restype = POINTER( cDistributions )
ctraj.Build_Distributions.argtype = [
  POINTER( cMeasurements ), # measurements table
  c_int,                    # number of rows
  c_int ]                   # number of bins

ctraj.Build_Velocity_Distributions.restype = POINTER( cDistributions )
ctraj.Build_Velocity_Distributions.argtype = [
  POINTER( cMeasurements ), # measurements table
  c_int,                    # number of rows
  c_int ]                   # number of bins

ctraj.Solve.restype = None
ctraj.argtypes = [
  POINTER( cMeasurements ), # table
  c_int,                    # number of rows
  c_int ]                   # number of bins

ctraj.Measurements_Tables_Get_Diff_Frames.restype = POINTER( c_int )
ctraj.Measurements_Tables_Get_Diff_Frames.argtypes = [
  POINTER( cMeasurements ), #table A
  c_int,                    #number of rows for table A
  POINTER( cMeasurements ), #table B                   
  c_int,                    #number of rows for table B
  POINTER( c_int ) ]        #size of returned static array

if __name__=='__main__':
  testcases = [ 
                Tests_MeasurementsTable_FromDoubles,
                Tests_MeasurementsTable_FromFile ,
                Tests_Distributions 
                ]
  suite = reduce( lambda a,b: a if a.addTest(b) else a, 
                  map( unittest.defaultTestLoader.loadTestsFromTestCase, testcases ) 
                )
  suite.addTest( doctest.DocTestSuite() )
  runner = unittest.TextTestRunner(verbosity=2,descriptions=1).run(suite)
