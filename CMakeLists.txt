cmake_minimum_required(VERSION 2.8)
project(Whisk)
set(WHISK_VERSION_MAJOR 1)
set(WHISK_VERSION_MINOR 0)


set(ROOT_3RDPARTY_DIR ${CMAKE_SOURCE_DIR}/3rdParty)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ROOT_3RDPARTY_DIR})

find_package(FFMPEG)
set(HAVE_FFMPEG 0)
if(FFMPEG_FOUND)
  set(HAVE_FFMPEG 1)
  include_directories(${FFMPEG_INCLUDE_DIRS})
endif()

# p-file transform
file(GLOB PFILES  src/*.p)
file(GLOB MANAGER src/manager.awk)
include(MylibPreproc)
mylib_preprocess(MYLIB_SRCS PFILES MANAGER)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/src")
set(MYLIB_HDRS
  include/contour_lib.h
  include/draw_lib.h
  include/image_filters.h
  include/image_lib.h
  include/level_set.h
  include/utilities.h
  include/water_shed.h
)
set(MYLIB
  src/tiff_image.c 
  src/tiff_io.c    
  src/tiff_adapt.c 
  src/image_adapt.c                         
  include/tiff_image.h 
  include/tiff_io.h    
  include/image_adapt.h                         
  ${MYLIB_HDRS}
  ${MYLIB_SRCS}
  ${PFILES}
)
source_group("Mylib\\Source Files" FILES ${PFILES})
source_group("Mylib\\Generated"    FILES ${MYLIB_SRCS})
source_group("Mylib\\Header Files" FILES ${MYLIB_HDRS})
source_group("Mylib\\Awk"          FILES ${MANAGER})

# parameter parsing
add_subdirectory(src/parameters)
set_source_files_properties(${PARAM_MODULE} PROPERTIES GENERATED 1)

#
# Configure
# ---------
#

configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

#
# Source Groups
# -------------
# This section defines these modules:
#
# COMMON
# WHISKER_IO
# MEASUREMENTS_IO
# BAR_IO
# VIDEO_IO
# MATH
# TRACE
# TRAJ
# HMM
include_directories(include)
include_directories("${PROJECT_BINARY_DIR}")

# Common
set(COMMON_SRCS
  src/error.c
  src/common.c
  src/compat.c
)
set(COMMON_HDRS
  include/error.h
  include/common.h
  include/compat.h
)
set(COMMON
  ${COMMON_SRCS}
  ${COMMON_HDRS}
  ${BISON_PARAM_OUTPUTS}
  )
source_group("Source Files\\Common" FILES ${COMMON_SRCS})
source_group("Header Files\\Common" FILES ${COMMON_HDRS})
source_group("Parameter File Parser" FILES ${BISON_PARAM_OUTPUTS})   

# Whisker IO
set(WHISKER_IO_SRCS
  src/whisker_io.c
  src/whisker_io_whisker1.c
  src/whisker_io_whiskpoly1.c
  src/whisker_io_whiskbin1.c
  src/whisker_io_whiskold.c
)
set(WHISKER_IO_HDRS
  include/whisker_io.h
  include/whisker_io_whisker1.h
  include/whisker_io_whiskpoly1.h
  include/whisker_io_whiskbin1.h           
  include/whisker_io_whiskold.h
)
set(WHISKER_IO
  ${WHISKER_IO_SRCS}
  ${WHISKER_IO_HDRS}
)
source_group("Source Files\\IO\\Whisker" FILES ${WHISKER_IO_SRCS})
source_group("Header Files\\IO\\Whisker" FILES ${WHISKER_IO_HDRS})

# Measurements IO
set(MEASUREMENTS_IO_SRCS
  src/measurements_io.c
  src/measurements_io_v0.c
  src/measurements_io_v1.c
)
set(MEASUREMENTS_IO_HDRS
  include/measurements_io.h
  include/measurements_io_v0.h
  include/measurements_io_v1.h
)
set(MEASUREMENTS_IO
  ${MEASUREMENTS_IO_SRCS}
  ${MEASUREMENTS_IO_HDRS}
)
source_group("Source Files\\IO\\Measurements" FILES ${MEASUREMENTS_IO_SRCS})
source_group("Header Files\\IO\\Measurements" FILES ${MEASUREMENTS_IO_HDRS})

# Bar IO
set(BAR_IO_SRCS
  src/bar_io.c
)
set(BAR_IO_HDRS
  include/bar_io.h
)
set(BAR_IO
  ${BAR_IO_SRCS}
  ${BAR_IO_HDRS}
)
source_group("Source Files\\IO\\Bar" FILES ${BAR_IO_SRCS})
source_group("Header Files\\IO\\Bar" FILES ${BAR_IO_HDRS})

# Video
set(VIDEO_IO_SRCS
  src/tiff_image.c 
  src/tiff_io.c    
  src/tiff_adapt.c 
  src/image_adapt.c                         
  src/seq.c
  src/ffmpeg_adapt.c
)
set(VIDEO_IO_HDRS
  include/tiff_image.h 
  include/tiff_io.h    
  include/image_adapt.h                         
  include/seq.h
  include/ffmpeg_adapt.h
)
set(VIDEO_IO
  ${VIDEO_IO_SRCS}
  ${VIDEO_IO_HDRS}
)
source_group("Source Files\\IO\\Video" FILES ${VIDEO_IO_SRCS})
source_group("Header Files\\IO\\Video" FILES ${VIDEO_IO_HDRS})

# Math
set(MATH_SRCS
  src/mat.c                                 
  src/poly.c
  src/svd.c
  src/aip.c
)
set(MATH_HDRS
  include/mat.h                                 
  include/poly.h
  include/svd.h
  include/aip.h
)
set(MATH
  ${MATH_SRCS}
  ${MATH_HDRS}
)
source_group("Source Files\\Math" FILES ${MATH_SRCS})
source_group("Header Files\\Math" FILES ${MATH_HDRS})
  

# Trace
set(TRACE_SRCS
  src/adjust_scan_bias.c                    
  src/bar.c
  src/seed.c
  src/merge.c
  src/eval.c                                
  src/trace.c
)
set(TRACE_HDRS
  include/adjust_scan_bias.h                    
  include/bar.h
  include/seed.h
  include/merge.h
  include/eval.h                                
  include/trace.h
)
set(TRACE
  ${TRACE_SRCS}
  ${TRACE_HDRS}
)
source_group("Source Files\\" FILES ${TRACE_SRCS})
source_group("Header Files\\" FILES ${TRACE_HDRS})

# TRAJ
set(TRAJ_SRCS
  src/measure.c
  src/report.c
  src/traj.c
)
set(TRAJ_HDRS
  include/traj.h
)
set(TRAJ
  ${TRAJ_SRCS}
  ${TRAJ_HDRS}
)
source_group("Source Files\\" FILES ${TRAJ_SRCS})
source_group("Header Files\\" FILES ${TRAJ_HDRS})


# HMM
set(HMM_SRCS
  src/hmm-reclassify-lrmodel-w-deletions.c
  src/hmm-reclassify-lrmodel.c            
  src/hmm-reclassify.c                    
  src/viterbi.c                           
)
set(HMM_HDRS
  include/hmm-reclassify-lrmodel-w-deletions.h
  include/hmm-reclassify-lrmodel.h            
  include/hmm-reclassify.h                    
  include/viterbi.h                           
)
set(HMM
  ${HMM_SRCS}
  ${HMM_HDRS}
)
source_group("Source Files\\" FILES ${HMM_SRCS})
source_group("Header Files\\" FILES ${HMM_HDRS})

###########
# TARGETS #
###########

#
# LIBS
#

add_library(whisk SHARED
  ${COMMON}
  ${MYLIB}
  ${WHISKER_IO}
  ${MEASUREMENTS_IO}
  ${VIDEO_IO}
  ${MATH}
  ${TRACE}
  ${BAR_IO}
  ${PARAM_MODULE}
  ${FFMPEG_INCLUDE_DIRS}
  ${TRAJ}
  ${HMM}
)
target_link_libraries(whisk ${FFMPEG_LIBRARIES})
add_dependencies(whisk ParameterParser)

add_library(traj SHARED
  ${COMMON}
  ${MYLIB}
  ${MEASUREMENTS_IO}
  ${MATH}
  ${PARAM_MODULE}
  ${TRAJ}
  ${HMM}
)
add_dependencies(traj ParameterParser)

add_library(whiskio STATIC
  ${COMMON}
  ${MYLIB}
  ${WHISKER_IO}
  ${MEASUREMENTS_IO}
  ${VIDEO_IO}
  ${MATH}
  ${TRACE}
  ${BAR_IO}
  ${FFMPEG_INCLUDE_DIRS}
)
target_link_libraries(whiskio ${FFMPEG_LIBRARIES})
add_dependencies(whiskio ParameterParser) #still need param.h
set_target_properties(whiskio PROPERTIES
  COMPILE_DEFINITIONS SKIP_PARAMS_FILE
)

#
# EXECUTABLES
#

# trace
source_group("Source Files" FILES src/whisk.c)
add_executable(trace
  src/whisk.c
  ${COMMON}
  ${MYLIB}
  ${WHISKER_IO}
  ${VIDEO_IO}
  ${MATH}
  ${TRACE}
  ${BAR_IO}
  ${PARAM_MODULE}
  ${FFMPEG_INCLUDE_DIRS}
)
target_link_libraries(trace ${FFMPEG_LIBRARIES})
add_dependencies(trace ParameterParser)
  
#measure
source_group("Source Files" FILES src/measure.c)
add_executable(measure
  src/measure.c
  ${COMMON}
  ${MYLIB}
  ${WHISKER_IO}
  ${MEASUREMENTS_IO}
  ${MATH}
  ${TRACE}
  ${TRAJ}
  ${BAR_IO}
  ${PARAM_MODULE}
)
set_target_properties(measure PROPERTIES
  COMPILE_DEFINITIONS TEST_MEASURE_1
)
add_dependencies(measure ParameterParser)

#classify
source_group("Source Files" FILES src/classify.c)
add_executable(classify
  src/classify.c
  ${COMMON}
  ${MYLIB}
  ${MEASUREMENTS_IO}
  ${MATH}
  ${TRAJ}
  ${PARAM_MODULE}
)
set_target_properties(classify
  PROPERTIES
    COMPILE_DEFINITIONS TEST_CLASSIFY_1
)
add_dependencies(classify ParameterParser)

#reclassify
source_group("Source Files" FILES src/hmm-reclassify.c)
add_executable(reclassify
  src/hmm-reclassify.c
  ${COMMON}
  ${MYLIB}
  ${MEASUREMENTS_IO}
  ${TRAJ}
  ${MATH}
  ${HMM}
  ${PARAM_MODULE}
)
set_target_properties(reclassify
  PROPERTIES
    COMPILE_DEFINITIONS TEST_HMM_RECLASSIFY_5
)
add_dependencies(reclassify ParameterParser)

#report
source_group("Source Files" FILES src/report.c)
add_executable(report
  src/report.c
  ${COMMON}
  ${MYLIB}
  ${MEASUREMENTS_IO}
  ${MATH}
  ${TRAJ}
  ${MATH}
  ${HMM}
  ${PARAM_MODULE}
)
set_target_properties(report
  PROPERTIES
    COMPILE_DEFINITIONS TEST_REPORT_1
)
add_dependencies(report ParameterParser)

#preprocess
source_group("Source Files" FILES src/adjust_lines.c)
add_executable(preprocess
  src/adjust_lines.c
  src/aip.c
  include/aip.h
  ${COMMON}
  ${MYLIB}
  ${TRACE}
  ${VIDEO_IO}
  ${PARAM_MODULE}
)
add_dependencies(preprocess ParameterParser)
target_link_libraries(preprocess ${FFMPEG_LIBRARIES})

#seedtest
source_group("Source Files" FILES src/seedtest.c)
add_executable(seedtest
  src/seedtest.c
  src/aip.c
  include/aip.h
  ${COMMON}
  ${MYLIB}
  ${TRACE}
  ${VIDEO_IO}
  ${PARAM_MODULE}
)
add_dependencies(seedtest ParameterParser)
target_link_libraries(seedtest ${FFMPEG_LIBRARIES})

#stripetest
source_group("Source Files" FILES src/stripetest.c)
add_executable(stripetest
  src/stripetest.c
  src/aip.c
  include/aip.h
  ${COMMON}
  ${MYLIB}
  ${TRACE}
  ${VIDEO_IO}
  ${PARAM_MODULE}
)
add_dependencies(stripetest ParameterParser)
target_link_libraries(stripetest ${FFMPEG_LIBRARIES})

#test_whisker_io
source_group("Source Files" FILES src/test_whisker_io.c)
add_executable(test_whisker_io
  src/test_whisker_io.c
  ${COMMON}
  ${MATH}
  ${TRACE}
  ${MYLIB}
  ${WHISKER_IO}
  ${PARAM_MODULE}
)
add_dependencies(test_whisker_io ParameterParser)

#evaltest
source_group("Source Files" FILES src/evaltest.c)
set(EVALTEST_SRCS 
  src/evaltest.c
  ${COMMON}
  ${MATH}
  ${TRACE}
  ${MYLIB}
  ${WHISKER_IO}
  ${PARAM_MODULE}
)
#add_executable(  test_eval_1 ${EVALTEST_SRCS})
#add_dependencies(test_eval_1 ParameterParser)
#set_target_properties(test_eval_1 PROPERTIES COMPILE_DEFINITIONS EVAL_TEST_1)
add_executable(  test_eval_2 ${EVALTEST_SRCS})
add_dependencies(test_eval_2 ParameterParser)
set_target_properties(test_eval_2 PROPERTIES COMPILE_DEFINITIONS EVAL_TEST_2)
add_executable(  test_eval_3 ${EVALTEST_SRCS})
add_dependencies(test_eval_3 ParameterParser)
set_target_properties(test_eval_3 PROPERTIES COMPILE_DEFINITIONS EVAL_TEST_3)
add_executable(  test_eval_4 ${EVALTEST_SRCS})
add_dependencies(test_eval_4 ParameterParser)
set_target_properties(test_eval_4 PROPERTIES COMPILE_DEFINITIONS EVAL_TEST_4)
add_executable(  test_eval_5 ${EVALTEST_SRCS})
add_dependencies(test_eval_5 ParameterParser)
set_target_properties(test_eval_5 PROPERTIES COMPILE_DEFINITIONS EVAL_TEST_5)
# APP
# ---
#
# LIB
# ---
#
# TESTS
# -----
#
# TESTED BUT UNUSED
# -----------------
# deque.c                               
# match.c - Hungarian algorithm
#
# EXCLUDE
# -------
# distance.c
# dag.c
