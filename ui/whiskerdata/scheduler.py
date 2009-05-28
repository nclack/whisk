from threading import Thread, Event
import traceback, sys

class LastOnlyScheduler(Thread):
  def __init__(self):
    Thread.__init__(self)
    self.onjob = Event()
    self.onquit = Event()
    self._waiting_job = None
    self._executed = 0;
    self._requests = 0;

  def run(self):
    while not self.onquit.isSet():
      self.onjob.wait()
      self.onjob.clear()
      self._safecall( self._waiting_job )
      #print "scheduler (%d/%d)"%(self._executed, self._requests)
    if self.onjob.isSet():  #finish up the last one
      self._safecall( self._waiting_job )

  def _safecall(self, job):
    try:
      if job:
        job()
        self._executed += 1 
    except:
      title = '---'+str(self.__class__())+100*'-' 
      print
      print title[:80]
      traceback.print_exc(file=sys.stderr)
      print 80*'-'
      print

  def pushjob( self, job ):
    self._waiting_job = job
    self._requests += 1
    self.onjob.set()

  def quit(self):
    self.onquit.set()
    if not self.onjob.isSet():
      self._waiting_job = None
      self.onjob.set()

  def __del__(self):
    self.quit()      

import unittest
import sched, time

class TestLastOnlyScheduler(unittest.TestCase):
  def setUp(self):
    self.scheduler = LastOnlyScheduler()
    self.scheduler.start()

  def testLastOnlySchedulerAddOne(self):
    def f(name, x):
      for el in x:
        print '%s: %d'%(name, el)

    self.scheduler.pushjob( lambda: f('test add one job', range(10) ) )
  
  def testLastOnlySchedulerAddTen(self):
    def f(name, x):
      for el in x:
        print '%s: %d'%(name, el)
        time.sleep(0.1)

    for i in range(10):
      self.scheduler.pushjob( lambda: f('job (%d)'%i, range(10) ) )
      time.sleep(0.1)

  def testLastOnlySchedulerExceptionHandling(self):
    def f(name, x):
      for el in x:
        print '%s: %d'%(name, el)
        if (el+1)%5 == 0:
          raise Exception
        time.sleep(0.1)
    for i in range(10):
      self.scheduler.pushjob( 
        lambda: f('testLastOnlySchedulerExceptionHandling  (%d)'%i, range(10) ) 
        )
      time.sleep(0.1)

  def tearDown(self):
    self.scheduler.quit() 

if __name__ == '__main__':
  unittest.main()


