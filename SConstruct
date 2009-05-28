from glob import glob
import os

env = Environment(ENV = os.environ )
env.MergeFlags( env.ParseFlags( "-g -O3 -lm" ))

mains = Split( """ whisk
                   seedtest
                   test_whisker_io
                   stripetest
               """ )
        
#
# Add builder for transforming .p --> .c
#
awk = Builder( action = "awk -f manager.awk $SOURCE > $TARGET",
        suffix = '.c',
        src_suffix = '.p' )
env.Append( BUILDERS = {'Awk' : awk} )

#
# Register files that have to be awked and get 
#   a list of the resultant c files.
#
pnodes  = [ env.Awk(name) for name in glob("*.p") ] #get nodes
for n in pnodes:                                    #rebuild these if manager.awk is changed
  Depends(n,'manager.awk')                          
pcfiles = [ str(n[0]) for n in pnodes ]             #get resulting c files names

#
# Get the other source files and organize
#
cfiles = set( glob("*.c")+pcfiles )        # a unique set of all *.c files: both existent and *.p dependant
main_cfiles = set([n+".c" for n in mains]) # Assume main() is in a c-file with the same name as the program
cfiles = cfiles.difference(main_cfiles)    # Seperate c-files with and without mains
cfiles.remove("evaltest.c")

#
# Build some things!
#
for name in mains:
  env.Program(name,[name+".c"] + list(cfiles) )
env.SharedLibrary('whisk',list(cfiles))

## whisker converter
obj = env.Object("whisker_io_main", "whisker_io.c", CPPDEFINES = "WHISKER_IO_CONVERTER");
env.Program("whisker_convert",[obj]+list( cfiles - set(["whisker_io.c"]) ) )


## eval tests
#for i in range(2,6):
#  obj = env.Object("evaltest%d"%i, "evaltest.c",  CPPDEFINES = "EVAL_TEST_%d"%i )
#  env.Program('evaltest%d'%i,[obj] + list(cfiles))
