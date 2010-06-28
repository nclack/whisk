# vi:ft=python:et:ts=2
# 
from glob import glob
import os

Progress('Scanning:  $TARGET\r',overwrite=True)

# Currently, TARGET_ARCH only effects Windows builds.  It's set here to force
# an x86 build on x64 windows systems because there is no x64 FFMPEG support 
# there.

env = Environment(ENV = os.environ,
                  TOOLS = ['default','packaging'],
                  TARGET_ARCH="x86")

##
# PLATFORM DEPENDENT CONFIG
#
#
#Tool('mingw')(env)
if env['PLATFORM']=='win32':
  #env['CCFLAGS'] = ''
  #env.MergeFlags( env.ParseFlags('-g -lm') )
  #env.Append(CCFLAGS = r'/Od          /Ot     /D WIN32 /D _DEBUG /D _CONSOLE /D _UNICODE /D UNICODE /Gm /EHsc /RTC1 /MTd /W1 /ZI     /Gd /TC')   #Debug
  env.Append(CCFLAGS  = r'/Ox /Ob2 /Oi /Ot /GL /D WIN32 /D NDEBUG /D _CONSOLE /D _UNICODE /D UNICODE     /EHsc       /MT   /W1 /Zi /Gy     /TC')   #Release - optimized compilation - BROKEN! fread problem?
  env.Append(LINKFLAGS = r'/MACHINE:X86')
  static_libwhiskio_name = "whiskio.lib"
  env["LIBSUFFIX"]="" #have to do this to use static libaries with the wrong extension
else:
  #env.MergeFlags( env.ParseFlags( "-O3 -lm" ))
  static_libwhiskio_name = "whiskio"
  env.MergeFlags( env.ParseFlags( "-g -lm" ))
  if env['PLATFORM']=='darwin':
    pass
    #env.MergeFlags( env.ParseFlags( "-arch i386 -arch x86_64 -arch ppc"))
    
env['no_import_lib'] = 1

#
# Multiplatform configuration (autoconf-like)
#

conf = Configure(env)
# FFMPEG
if env['PLATFORM']=='win32':
  #binaries and includes are supplied, so we don't have to check.
  conf.env.Append(LIBPATH = r'dependencies\w32vs\ffmpeg\lib')
  conf.env.Append(CPPFLAGS = r'/I dependencies\w32vs\ffmpeg\include')
  conf.env.Append(CPPFLAGS=' /DHAVE_FFMPEG') #
  ffmpeg_libs = "libgcc.a libmingwex.a libavcodec.a libavformat.a libavutil.a libswscale.a liba52.a libz.a libfaac.a libfaad.a libgsm.a libmp3lame.a libogg.a libtheora.a libvorbis.a libvorbisenc.a libx264.a libxvidcore.a libpthreadGC2.a wsock32.lib vfw32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib".split()
  conf.env.Append(LIBS = ffmpeg_libs)
  print "(+) FFMPEG Found"
else:
  ffmpeg_includes = ["libavcodec/avcodec.h",
                     "libavformat/avformat.h",
                     "libswscale/swscale.h"]
  ffmpeg_libs = "avcodec avformat avutil swscale".split()
  if all(map(conf.CheckCHeader,ffmpeg_includes)) and \
     all(map(conf.CheckLib,ffmpeg_libs)):
       conf.env.Append(CPPFLAGS='-DHAVE_FFMPEG')
       conf.env.Append(LIBS = ffmpeg_libs)
       print "(+) FFMPEG Found"
  else:
       print "(-) FFMPEG NOT found. Building without FFMPEG support."
env = conf.Finish()

#
# BUILDERS
#

# Add builder for transforming .p --> .c
awk = Builder( action = "awk -f manager.awk $SOURCE > $TARGET",
               suffix = '.c',
               src_suffix = '.p' )
env.Append( BUILDERS = {'Awk' : awk} )

#
# COLORIZE 
#

try:
  from scons_colorizer import colorizer
  if env['PLATFORM']!='win32':
    col = colorizer()
    col.colorize(env)
    col.colorizeBuilder(env,'Awk',"Generating Myers Managed code",True,col.cGreen)
except ImportError,e:
  print e


##
# MAIN TARGETS
#

mains = Split( """ whisk
                   seedtest
                   test_whisker_io
                   stripetest
                   adjust_lines
               """ )
excludes = set(( """ collisiontable_link_list.c
                     distance.c
                     trajectory.c
                     whisker_io.mex.c
                     measurements_io.mex.c
                 """ ).split())
        

# Register files that have to be awked
# Get a list of the resultant c files.
pnodes  = [ env.Awk(name) for name in glob("*.p") ] #get nodes
for n in pnodes:                                    #rebuild these if manager.awk is changed
  Depends(n,'manager.awk')                          
pcfiles = [ str(n[0]) for n in pnodes ]             #get resulting c files names

# Aggregate other source files 
cfiles = set( glob("*.c")+pcfiles+["parameters/param.c"] ) - excludes        # a unique set of all *.c files: both existent and *.p dependant
main_cfiles = set([n+".c" for n in mains]) # Assume main() is in a c-file with the same name as the program
cfiles = cfiles.difference(main_cfiles)    # Seperate c-files with and without mains
cfiles.remove("evaltest.c")


#
# Builds
#
main_targets = {}
for name in mains:
  main_targets[name] = env.Program(name,[name+".c"] + list(cfiles) )

libwhisk = env.SharedLibrary('whisk',list(cfiles))

ioobjs = map(lambda n:env.Object(n),[
                          'svd.c', 'mat.c', 'poly.c',
                          'traj.c', 'bar_io.c',
                          'common.c',    'image_lib.c', 'contour_lib.c',
                          'error.c',     'eval.c',      'level_set.c',
                          'utilities.c', 'tiff_io.c',   'image_filters.c',
                          'trace.c',     'tiff_image.c','compat.c',
                          'aip.c',       'seed.c',      'draw_lib.c',
                          'whisker_io.c',          'whisker_io_whiskbin1.c',
                          'whisker_io_whisker1.c', 'whisker_io_whiskold.c',
                          'whisker_io_whiskpoly1.c',
                          'measurements_io.c',
                          'measurements_io_v0.c',
                          'measurements_io_v1.c'])
staticlibwhiskio = env.StaticLibrary(static_libwhiskio_name,ioobjs,LIBS="")

## whisker converter
obj = env.Object("whisker_io_main", "whisker_io.c", CPPDEFINES = "WHISKER_IO_CONVERTER");
whisker_convert = env.Program("whisker_convert",[obj]+list( cfiles - set(["whisker_io.c"]) ) )

## measurements converter
obj = env.Object("measurements_io_main", "measurements_io.c", CPPDEFINES = "MEASUREMENTS_IO_CONVERTER");
measurements_convert = env.Program("measurements_convert",[obj]+list( cfiles - set(["measurements_io.c"]) ) )

## traj 
libtraj = env.SharedLibrary( 'traj', ['traj.c','common.c','error.c',
                                      'utilities.c','viterbi.c','report.c',
                                      'measure.c',
                                      'poly.c','mat.c','svd.c',
                                      'measurements_io.c',
                                      'measurements_io_v0.c',
                                      'measurements_io_v1.c'] )

## install - copy things around
ui_install_targets = \
  env.Install( 'ui/whiskerdata', ['trace.py','traj.py',libwhisk] ) \
+ env.Install( 'ui/reader', [libwhisk] ) \
+ env.Install( 'ui',             ['trace.py', libwhisk] ) 
#env.Alias( 'python', ['ui/whiskerdata','ui'] )

#
# testing/temporary stuff
#

## eval tests
for i in range(2,6):
  obj = env.Object("evaltest%d"%i, "evaltest.c",  CPPDEFINES = "EVAL_TEST_%d"%i )
  env.Program('test_eval%d'%i,[obj] + list(cfiles))

## viterbi tests
viterbi_test = env.Object('viterbi_test', ['viterbi.c'], CPPDEFINES = "TEST_VITERBI")
env.Program('test_viterbi', [viterbi_test, 'common.c', 'utilities.c','error.c',])

## traj tests
tests = ["TEST_BUILD_DISTRIBUTIONS",
         "TEST_MEASUREMENT_TABLE_IO_1",
         "TEST_SOLVE_GRAY_AREAS" ]
totestobj = lambda t: env.Object( 'trajobj_'+t.lower(), ['traj.c'], CPPDEFINES = t)
for t in tests:
  env.Program( 'test_traj_'+t[5:].lower(), [ totestobj(t),'common.c','error.c',
                                            'utilities.c','viterbi.c',
                                            'measurements_io.c',
                                            'measurements_io_v0.c',
                                           'measurements_io_v1.c',] ) 

## merge tests
tests = ["TEST_COLLISIONTABLE_1",
         "TEST_COLLISIONTABLE_2",
         "TEST_COLLISIONTABLE_3"
         ] 
totestobj = lambda t: env.Object( 'merge_'+t.lower(), ['merge.c'], CPPDEFINES = t )
for t in tests:
  env.Program( 'test_merge_'+t[5:].lower(), [ totestobj(t),
                                              'common.c',    'image_lib.c', 'contour_lib.c',
                                              'error.c',     'eval.c',      'level_set.c',
                                              'utilities.c', 'tiff_io.c',   'image_filters.c',
                                              'trace.c',     'tiff_image.c','compat.c',
                                              'aip.c',       'seed.c',      'draw_lib.c',
                                              'whisker_io.c',          'whisker_io_whiskbin1.c',
                                              'whisker_io_whisker1.c', 'whisker_io_whiskold.c',
                                              'whisker_io_whiskpoly1.c',
                                              'svd.c', 'mat.c', 'poly.c',
                                              ] ) 
## classify tests
tests = ["TEST_CLASSIFY_1",
         "TEST_CLASSIFY_2",
         "TEST_CLASSIFY_3"
         ] 
totestobj = lambda t: env.Object( 'classify_'+t.lower(), ['classify.c'], CPPDEFINES = t )
test_classify = map( lambda t: env.Program( 'test_'+t[5:].lower(), [ totestobj(t),
                                                 'utilities.c', 'traj.c', 'common.c',
                                                 'error.c','viterbi.c',
                                                 'measurements_io.c',
                                                 'measurements_io_v0.c',
                                                 'measurements_io_v1.c',
                                               ] ),
                                               tests)

## hmm-reclassify tests
tests = [ "TEST_HMM_RECLASSIFY_1",
          "TEST_HMM_RECLASSIFY_3",
          "TEST_HMM_RECLASSIFY_5",
         ] 
totestobj = lambda t: env.Object( 'hmm-reclassify_'+t.lower(), ['hmm-reclassify.c'], CPPDEFINES = t )
test_reclassify = map( lambda t: env.Program( 'test_'+t[5:].lower(), [ totestobj(t),
                                                 'utilities.c', 'traj.c', 'common.c',
                                                 'error.c','viterbi.c',
                                                 'hmm-reclassify-lrmodel.c',
                                                 'hmm-reclassify-lrmodel-w-deletions.c',
                                                 'measurements_io.c',
                                                 'measurements_io_v0.c',
                                                 'measurements_io_v1.c'
                                               ] ),
                                               tests)
## Deque tests
tests = ["TEST_DEQUE_1",
         ] 
totestobj = lambda t: env.Object( 'deque_'+t.lower(), ['deque.c'], CPPDEFINES = t )
for t in tests:
  env.Program( 'test_'+t[5:].lower(), [ totestobj(t),
                                                 'utilities.c', 'common.c',
                                                 'error.c',
                                               ] ) 

## Running maxima filter tests
tests = ["TEST_MAX_FILT_1",
         "TEST_MAX_FILT_2", 
         "TEST_MAX_FILT_3", 
         ] 
totestobj = lambda t: env.Object( 'common_'+t.lower(), ['common.c'], CPPDEFINES = t )
for t in tests:
  env.Program( 'test_'+t[5:].lower(), [ totestobj(t),
                                                 'utilities.c',
                                                 'error.c',
                                               ] ) 

## report tests
tests = ["TEST_REPORT_COMPARE_TRAJECTORIES",
         "TEST_REPORT_1",
         ] 
totestobj = lambda t: env.Object( 'report_'+t.lower(), ['report.c'], CPPDEFINES = t )
reporter = {}
for t in tests:
  reporter[t] = env.Program( 'test_'+t[5:].lower(), [ totestobj(t),
                                                 'utilities.c', 'common.c',
                                                 'error.c', 'traj.c', 'viterbi.c',
                                                 'measurements_io.c',
                                                 'measurements_io_v0.c',
                                                 'measurements_io_v1.c'
                                               ] ) 

## svd tests
tests = ["TEST_SVD_1",
         "TEST_SVD_2",
         "TEST_SVD_3"
         ] 
totestobj = lambda t: env.Object( 'svd_'+t.lower(), ['svd.c'], CPPDEFINES = t )
for t in tests:
  env.Program( 'test_'+t[5:].lower(), [ totestobj(t),
                                                 'utilities.c', 'common.c',
                                                 'error.c', 'compat.c', 'mat.c'
                                               ] ) 

## polyfit tests
tests = ["TEST_POLYFIT_1",
         "TEST_POLYFIT_2", 
         ] 
totestobj = lambda t: env.Object( 'polyfit_'+t.lower(), ['poly.c'], CPPDEFINES = t )
for t in tests:
  env.Program( 'test_'+t[5:].lower(), [ totestobj(t),
                                        'utilities.c', 'common.c', 'svd.c',
                                        'error.c', 'compat.c', 'mat.c'
                                      ] ) 

## measure tests
tests = ["TEST_MEASURE_1",
         "TEST_MEASURE_2"
         ] 
totestobj = lambda t: env.Object( 'measure_'+t.lower(), ['measure.c'], CPPDEFINES = t )
test_measure = map( lambda t: env.Program( 'test_'+t[5:].lower(), [ totestobj(t),
                                              'svd.c', 'mat.c', 'poly.c',
                                              'traj.c', 'bar_io.c',
                                              'common.c',    'image_lib.c', 'contour_lib.c',
                                              'error.c',     'eval.c',      'level_set.c',
                                              'utilities.c', 'tiff_io.c',   'image_filters.c',
                                              'trace.c',     'tiff_image.c','compat.c',
                                              'aip.c',       'seed.c',      'draw_lib.c',
                                              'whisker_io.c',          'whisker_io_whiskbin1.c',
                                              'whisker_io_whisker1.c', 'whisker_io_whiskold.c',
                                              'whisker_io_whiskpoly1.c',
                                              'measurements_io.c',
                                              'measurements_io_v0.c',
                                              'measurements_io_v1.c'
                                                ] ),
                                                tests)
## bar_io tests 
tests = [ "TEST_BAR_IO_1",
          "TEST_BAR_IO_2",
        ]
totestobj = lambda t: env.Object( 'bar_io_'+t.lower(), ['bar_io.c'], CPPDEFINES = t )
for t in tests:
  env.Program( 'test_'+t[5:].lower(), [ totestobj(t),
                                                 'utilities.c', 'common.c',
                                                 'error.c', 'compat.c'
                                               ] ) 

##
# PACKAGING
#
distname='WhiskerTracking'
dist = []
dist += env.InstallAs(target=distname+'/bin/trace$PROGSUFFIX'                     , source=main_targets['whisk'])
dist += env.InstallAs(target=distname+'/bin/measure$PROGSUFFIX'                   , source=test_measure[0])
dist += env.InstallAs(target=distname+'/bin/classify$PROGSUFFIX'                  , source=test_classify[0])
dist += env.InstallAs(target=distname+'/bin/reclassify$PROGSUFFIX'                , source=test_reclassify[-1])
dist += env.InstallAs(target=distname+'/bin/report/trajectory_mismatch_histogram$PROGSUFFIX' , source=reporter['TEST_REPORT_COMPARE_TRAJECTORIES'])
dist += env.InstallAs(target=distname+'/bin/report/trajectory_mismatch_frames$PROGSUFFIX'    , source=reporter['TEST_REPORT_1'])
dist += env.InstallAs(target=distname+'/bin/whisker_convert$PROGSUFFIX'           , source=whisker_convert)
dist += env.InstallAs(target=distname+'/bin/measurements_convert$PROGSUFFIX'      , source=measurements_convert)
dist += env.InstallAs(target=distname+'/bin/default.parameters'                   , source='parameters/default.parameters')
dist += env.Install(target=distname+'/python'                                     , source=['traj.py','trace.py',libwhisk])
dist += env.Install(target=distname+'/matlab'                                     , source=env.Glob('*.m')+['whisker_io.mex.c','measurements_io.mex.c']+staticlibwhiskio)
dist += env.Install(target=distname+'/matlab/include'                             , source=['measurements_io.h','traj.h','whisker_io.h','trace.h','common.h','image_lib.h','contour_lib.h','seed.h','eval.h','parameters/param.h','compat.h','tiff_io.h','level_set.h','water_shed.h','aip.h','utilities.h'])
dist += env.Install(target=distname+'/matlab/include/parameters'                  , source=['parameters/param.h'])
dist += env.Install(target=distname+'/ui/'                                        , source=['ui/README','ui/icon.png']+env.Glob('ui/*.py')+[libwhisk[0].path])
dist += env.Install(target=distname+'/ui/reader'                                  , source=env.Glob('ui/reader/*.py')+['README']+[libwhisk[0].path])
dist += env.Install(target=distname+'/ui/whiskerdata'                             , source=env.Glob('ui/whiskerdata/*.py')+[libwhisk[0].path])
## 
## Packager does not seem to work correctly.
##
# env.Package( source = [e.path for e in dist],
#             NAME = 'whisk',
#             VERSION = '0.1',
#             PACKAGEVERSION = 0,
#             LICENSE = 'The Janelia Farm Research Campus Software Copyright 1.1 (http://license.janelia.org/license/jfrc_copyright_1_1.html)',
#             SUMMARY = 'A software suite for tracing and tracking whiskers',
#             DESCRIPTION = 'FIXME: This is the description.')
#
