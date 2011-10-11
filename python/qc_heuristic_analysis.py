import traj
from pylab import *

def heuristic_precision(table):
  length,estn = table.est_length_threshold();
  data = table.asarray()
  idx = find(data[:,3]>length);
  tp = sum(data[idx,0]>-1);          #relevent intersect retrieved/retrieved
  return tp/float(len(idx))

def heuristic_recall(table):
  length,estn = table.est_length_threshold();
  data = table.asarray()
  idx = find(data[:,0]>-1);
  tp = sum(data[idx,3]>length);      #relevant intersect retrieved/relevant
  return tp/float(len(idx))


def training_set_fraction(table):
  def frame_by_length(table):
    data = table.asarray()
    lengths = {}
    for r in data:
      f = lengths.get(r[1],[])
      f.append(r[3])
      lengths[r[1]] = f
    return lengths
  length,estn = table.est_length_threshold();
  count = lambda v: sum(map(lambda x: x>length,v))
  isok = lambda v: count(v)==estn
  lengths = frame_by_length(table)
  nok = sum(map(isok,lengths.values()))
  return nok/float(len(lengths))

def proc(root):
  import fnmatch
  import os

  def gen_names(root):
    for r,dirnames,filenames in os.walk(root):
      for filename in fnmatch.filter(filenames,'*.measurements'):
        yield os.path.join(r,filename)

  filecount = len(list(gen_names(root)))

  count  = 0.0;
  recall = 0.0;
  precision = 0.0;

  training_max  = 0.0;
  training_min  = 1.0;
  training_mean = 0.0;
  for i,n in enumerate(gen_names(root)):
    if 1: #i < 5:
      try:
        print "[%5d of %5d] %s"%(i,filecount,n)
        table = traj.MeasurementsTable(n)
        recall += heuristic_recall(table)
        precision += heuristic_precision(table)
        t = training_set_fraction(table)
        training_max = max(training_max,t)
        training_min = min(training_min,t)
        training_mean += t;
        count += 1.0
      except:
        pass
  recall /= count
  precision /= count
  training_mean /= count
  return locals()

"""
Ran this on Leo's curated data.  749 trials.
(note that only 344 were redone by karel)

  date: Sept. 
{'count': 508.0, #not all files could be read.
 'filecount': 749,
 'fnmatch': <module 'fnmatch' from 'C:\Python26\lib\fnmatch.pyc'>,
 'gen_names': <function gen_names at 0x03D57530>,
 'i': 748,
 'n': 'F:\\CuratedSessions\\LTPJF41967_083109\\LTPJF41967_083109_C3_D_0130.measurements',
 'os': <module 'os' from 'C:\Python26\lib\os.pyc'>,
 'precision': 1.0,
 'recall' : 0.18259270275364028,
 'root': 'F:\\CuratedSessions',
 't': 0.99919978660976261,
 'table': <traj.MeasurementsTable object at 0x03E65550>,
 'training_max': 1.0,
 'training_mean': 0.99866683665075406,
 'training_min': 0.90077353961056283}

"""

