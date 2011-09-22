import traj
from numpy import *
from pylab import *
import pdb

def backup_measurements(root,backupname):

  import fnmatch
  import os
  import shutil

  def gen_names(root):
    for r,dirnames,filenames in os.walk(root):
      for filename in fnmatch.filter(filenames,'*.measurements'):
        yield os.path.join(r,filename)

  def subroot(root,name):
    s = os.path.split  #alias
    j = os.path.join
    head,post = s(name)
    while head != root:
      head,tail = s(head)
      post = j(tail,post)
    return post

  def ensure(name):
    s = os.path.split  #alias
    isfname = lambda x: os.path.splitext(x)[-1] != '' 
    def gen():
      h,t = s(name)
      while t:
        yield h
        h,t = s(h)
    for n in reversed(list(gen())):
      if not os.path.exists(n):
        os.mkdir(n)


      
  broot = os.path.join(root,backupname)

  N = len(list(gen_names(root)))
  for i,e in enumerate(gen_names(root)):
    d = os.path.join(broot,subroot(root,e))
    print "[%6d of %6d] %s"%(i,N,d)
    ensure(d)
    shutil.copy( e,d )
