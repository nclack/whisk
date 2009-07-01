from glob import glob
import os

env = Environment(ENV = os.environ )
#env.MergeFlags( env.ParseFlags( "-g -O3 -lm" ))
env.MergeFlags( env.ParseFlags( "-g -lm" ))

mains = Split( """ whisk
                   seedtest
                   test_whisker_io
                   stripetest
               """ )
        
# Add builder for transforming .p --> .c
awk = Builder( action = "awk -f manager.awk $SOURCE > $TARGET",
               suffix = '.c',
               src_suffix = '.p' )
env.Append( BUILDERS = {'Awk' : awk} )

# Register files that have to be awked
# Get a list of the resultant c files.
pnodes  = [ env.Awk(name) for name in glob("*.p") ] #get nodes
for n in pnodes:                                    #rebuild these if manager.awk is changed
  Depends(n,'manager.awk')                          
pcfiles = [ str(n[0]) for n in pnodes ]             #get resulting c files names

# Aggregate other source files 
cfiles = set( glob("*.c")+pcfiles )        # a unique set of all *.c files: both existent and *.p dependant
main_cfiles = set([n+".c" for n in mains]) # Assume main() is in a c-file with the same name as the program
cfiles = cfiles.difference(main_cfiles)    # Seperate c-files with and without mains
cfiles.remove("evaltest.c")

#
# Builds
#
for name in mains:
  env.Program(name,[name+".c"] + list(cfiles) )

libwhisk = env.SharedLibrary('whisk',list(cfiles))

## whisker converter
obj = env.Object("whisker_io_main", "whisker_io.c", CPPDEFINES = "WHISKER_IO_CONVERTER");
env.Program("whisker_convert",[obj]+list( cfiles - set(["whisker_io.c"]) ) )

## traj 
libtraj = env.SharedLibrary( 'traj', ['traj.c','common.c','error.c','utilities.c'] )

## install - copy things around
env.Install( 'ui/whiskerdata', ['trace.py', libwhisk] ) 
env.Install( 'ui',             ['trace.py', libwhisk] ) 
env.Alias( 'python', ['ui/whiskerdata','ui'] )

#
# testing/temporary stuff
#

## eval tests
for i in range(2,6):
  obj = env.Object("evaltest%d"%i, "evaltest.c",  CPPDEFINES = "EVAL_TEST_%d"%i )
  env.Program('test_eval%d'%i,[obj] + list(cfiles))

## viterbi tests
viterbi_test = env.Object('viterbi_test', ['viterbi.c'], CPPDEFINES = "TEST_VITERBI")
env.Program('test_viterbi', [viterbi_test, 'common.c', 'utilities.c'])

## traj tests
tests = ["TEST_BUILD_DISTRIBUTIONS",
         "TEST_MEASUREMENT_TABLE_IO_1",
         "TEST_SOLVE_GRAY_AREAS" ]
totestobj = lambda t: env.Object( 'trajobj_'+t.lower(), ['traj.c'], CPPDEFINES = t)
for t in tests:
  env.Program( 'test_traj_'+t[5:].lower(), [ totestobj(t),'common.c','error.c','utilities.c'] ) 
