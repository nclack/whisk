## Author: Nathan Clack <clackn@janelia.hhmi.org>
## Date  : 2010 
##
## Copyright 2010 Howard Hughes Medical Institute.
## All rights reserved.
## Use is subject to Janelia Farm Research Campus Software Copyright 1.1
## license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
## 
cmake_minimum_required(VERSION 2.8)
project(whisk)
set(WHISK_VERSION_MAJOR 1)
set(WHISK_VERSION_MINOR 0)
set(WHISK_VERSION_PATCH 2)

macro(show v)
  message("${v} is ${${v}}")
endmacro()

#
# Setup output directories for visual studio
#
if(MSVC OR XCODE)
  set(USE_GENERATOR_SPECIFIC_OUTPUT_DIR FALSE CACHE BOOL "Put built files into a directory named for the generator.  Helps support side-by-side 32-bit and 64-bit builds.")
  #message("!! ${CMAKE_CURRENT_SOURCE_DIR}")
  if(${USE_GENERATOR_SPECIFIC_OUTPUT_DIR})
    set(dir ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_GENERATOR})
    set(EXECUTABLE_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)
    set(LIBRARY_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)
  endif()
endif()
set(CMAKE_VERBOSE_MAKEFILE 0)

#
# 3rdParty support
#
set(ROOT_3RDPARTY_DIR ${CMAKE_SOURCE_DIR}/3rdParty)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ROOT_3RDPARTY_DIR})
find_package(FFMPEG)
set(HAVE_FFMPEG 0)
if(FFMPEG_FOUND)
  set(HAVE_FFMPEG 1)
  include_directories(${FFMPEG_INCLUDE_DIRS})
  if(WIN32)
    file(COPY ${FFMPEG_SHARED_LIBS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}) #convenient for debugging
  endif()
endif()

# Mylib: p-file transform
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
source_group("Source Files\\Parameters" FILES ${PARAM_MODULE})

#
# Configure
# ---------
#

configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

if(WIN32)
set(LIBM )
else()
set(LIBM m)
endif()

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
# QTUI

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
  src/measurements_io_v2.c
)
set(MEASUREMENTS_IO_HDRS
  include/measurements_io.h
  include/measurements_io_v0.h
  include/measurements_io_v1.h
  include/measurements_io_v2.h
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
  src/video.c
  src/ffmpeg_adapt.c
)
set(VIDEO_IO_HDRS
  include/tiff_image.h 
  include/tiff_io.h    
  include/image_adapt.h                         
  include/seq.h
  include/video.h
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
  src/classify.c
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

# QTUI
SET(QTLIBLIST
  QtMain
  QtGui
  QtCore
  QtOpenGL
  QtSvg
)
if(WIN32) #this is a bit specific to me...should use imported targets if you build qt yourself
set(QT_USE_IMPORTED_TARGETS TRUE)
endif()
find_package(Qt4 4.7 COMPONENTS ${QTLIBLIST})
set(QTUI_HDRS)
set(QTUI_SRCS)
set(QTUI_SUBDIR qtui)
if(QT_FOUND)
  include(${QT_USE_FILE})
  include_directories(${QT_INCLUDE_DIRS})
  file(GLOB QTUI_HDRS ${CMAKE_SOURCE_DIR}/${QTUI_SUBDIR}/*.h)
  file(GLOB QTUI_SRCS ${CMAKE_SOURCE_DIR}/${QTUI_SUBDIR}/*.cpp)
  source_group("Source Files\\ui" FILES ${QTUI_SRCS})
  source_group("Header Files\\ui" FILES ${QTUI_HDRS})

  qt4_wrap_cpp(MOC_SRCS
    qtui/MainWindow.h
    qtui/Editor.h
    qtui/Data.h
    qtui/Curve.h
    qtui/FaceIndicator.h
  )
  qt4_add_resources(RCC_SRCS qtui/qtui.qrc)
  source_group("Source Files\\Qt Generated" FILES
    ${MOC_SRCS}
    ${RCC_SRCS}
  )
endif()

###########
# TARGETS #
###########

#
# LIBS
#

add_library(whisk MODULE #SHARED
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
target_link_libraries(whisk ${FFMPEG_LIBRARIES} ${LIBM})
add_dependencies(whisk ParameterParser)

add_library(traj MODULE #SHARED
  ${COMMON}
  ${MYLIB}
  ${MEASUREMENTS_IO}
  ${MATH}
  ${PARAM_MODULE}
  ${TRAJ}
  ${HMM}
)
add_dependencies(traj ParameterParser)
target_link_libraries(traj ${LIBM})

add_library(whiskio MODULE #STATIC
  ${COMMON}
  ${PARAM_MODULE}
  ${MYLIB}
  ${WHISKER_IO}
  ${MEASUREMENTS_IO}
  ${VIDEO_IO}
  ${MATH}
  ${TRACE}
  ${TRAJ}
  ${BAR_IO}
  ${FFMPEG_INCLUDE_DIRS}
)
target_link_libraries(whiskio ${FFMPEG_LIBRARIES} ${LIBM})
add_dependencies(whiskio ParameterParser) #still need param.h
set_target_properties(whiskio PROPERTIES
  COMPILE_DEFINITIONS SKIP_PARAMS_FILE
)
add_dependencies(whiskio ParameterParser)

add_library(whiskio_static STATIC
  ${COMMON}
  ${MYLIB}
  ${WHISKER_IO}
  ${MEASUREMENTS_IO}
  ${MATH}
  ${TRACE}
  ${TRAJ}
  ${BAR_IO}
)
target_link_libraries(whiskio_static ${LIBM})
add_dependencies(whiskio_static ParameterParser) #still need param.h
set_target_properties(whiskio_static PROPERTIES
  COMPILE_DEFINITIONS SKIP_PARAMS_FILE
)
add_dependencies(whiskio_static ParameterParser)

#
# MATLAB
#

  find_package(Matlab-rwp) #optional
  include(MatlabMexMacro)

  if(MATLAB_FOUND)
    add_mex_file(mexLoadMeasurements matlab/src/measurements_io.mex.c)
    target_link_libraries(mexLoadMeasurements whiskio_static)
    set_target_properties(mexLoadMeasurements
        PROPERTIES
        COMPILE_DEFINITIONS LOAD_MEASUREMENTS
        )

    add_mex_file(mexSaveMeasurements matlab/src/measurements_io.mex.c)
    target_link_libraries(mexSaveMeasurements whiskio_static)
    set_target_properties(mexSaveMeasurements
        PROPERTIES
        COMPILE_DEFINITIONS SAVE_MEASUREMENTS
        )

    add_mex_file(mexLoadWhiskers matlab/src/whisker_io.mex.c)
    target_link_libraries(mexLoadWhiskers whiskio_static)
  endif()

#
# EXECUTABLES
#

# whiski - Qt GUI
# see: http://developer.qt.nokia.com/doc/qt-4.8/appicon.html
if(QT_FOUND)
  if(WIN32)
    file(GLOB RC_SRCS qtui/*.rc)
  endif()
  set(ICONS qtui/resources/icons/whiski.icns)
  set_property(SOURCE ${ICONS} PROPERTY MACOSX_PACKAGE_LOCATION Resources)
  add_executable(whiski MACOSX_BUNDLE #WIN32 
    ${RC_SRCS}
    ${QTUI_HDRS}
    ${QTUI_SRCS}
    ${MOC_SRCS}
    ${RCC_SRCS}
    ${ICONS}
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
  target_link_libraries(whiski ${QT_LIBRARIES})
  target_link_libraries(whiski ${FFMPEG_LIBRARIES} ${LIBM})
  add_dependencies(whiski ParameterParser)
  set_property(TARGET whiski PROPERTY 
      MACOSX_BUNDLE_ICON_FILE whiski.icns)
endif()


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
target_link_libraries(trace ${FFMPEG_LIBRARIES} ${LIBM})
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
target_link_libraries(measure ${LIBM})

#classify-single-whisker
source_group("Source Files" FILES src/classify-single-whisker.c)
add_executable(classify-single-whisker
  src/classify-single-whisker.c
  ${COMMON}
  ${MYLIB}
  ${MEASUREMENTS_IO}
  ${MATH}
  ${TRAJ}
  ${PARAM_MODULE}
)
add_dependencies(classify-single-whisker ParameterParser)
target_link_libraries(classify-single-whisker ${LIBM})

#classify
source_group("Source Files" FILES src/classify.c)
add_executable(classify
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
target_link_libraries(classify ${LIBM})

#classify_radial
source_group("Source Files" FILES src/classify.c)
add_executable(classify_radial
  src/classify.c
  ${COMMON}
  ${MYLIB}
  ${MEASUREMENTS_IO}
  ${MATH}
  ${TRAJ}
  ${PARAM_MODULE}
)
set_target_properties(classify_radial
  PROPERTIES
    COMPILE_DEFINITIONS TEST_CLASSIFY_4
)
add_dependencies(classify_radial ParameterParser)
target_link_libraries(classify_radial ${LIBM})

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
target_link_libraries(reclassify ${LIBM})

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
target_link_libraries(report ${LIBM})

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
target_link_libraries(preprocess ${FFMPEG_LIBRARIES} ${LIBM})

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
target_link_libraries(seedtest ${FFMPEG_LIBRARIES} ${LIBM})

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
target_link_libraries(stripetest ${FFMPEG_LIBRARIES} ${LIBM})

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
target_link_libraries(test_whisker_io ${LIBM})

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
target_link_libraries(test_eval_2 ${LIBM})
add_executable(  test_eval_3 ${EVALTEST_SRCS})
add_dependencies(test_eval_3 ParameterParser)
set_target_properties(test_eval_3 PROPERTIES COMPILE_DEFINITIONS EVAL_TEST_3)
target_link_libraries(test_eval_3 ${LIBM})
add_executable(  test_eval_4 ${EVALTEST_SRCS})
add_dependencies(test_eval_4 ParameterParser)
set_target_properties(test_eval_4 PROPERTIES COMPILE_DEFINITIONS EVAL_TEST_4)
target_link_libraries(test_eval_4 ${LIBM})
add_executable(  test_eval_5 ${EVALTEST_SRCS})
add_dependencies(test_eval_5 ParameterParser)
set_target_properties(test_eval_5 PROPERTIES COMPILE_DEFINITIONS EVAL_TEST_5)
target_link_libraries(test_eval_5 ${LIBM})

#whisker_convert
add_executable(whisker_convert
  ${COMMON}
  ${MYLIB}
  ${WHISKER_IO}
  ${TRACE}
  ${MATH}
  ${PARAM_MODULE}
)
set_target_properties(whisker_convert
  PROPERTIES
    COMPILE_DEFINITIONS WHISKER_IO_CONVERTER
)
add_dependencies(whisker_convert ParameterParser)
target_link_libraries(whisker_convert ${LIBM})

#measurements_convert
add_executable(measurements_convert
  ${COMMON}
  ${MYLIB}
  ${MEASUREMENTS_IO}
  ${TRAJ}
  ${MATH}
  ${PARAM_MODULE}
)
set_target_properties(measurements_convert
  PROPERTIES
    COMPILE_DEFINITIONS MEASUREMENTS_IO_CONVERTER
)
target_link_libraries(measurements_convert ${LIBM})
add_dependencies(measurements_convert ParameterParser)

#test_measurementsio_repeated_reads
add_executable(test_measurementsio_repeated_reads
  ${COMMON}
  ${MYLIB}
  ${MEASUREMENTS_IO}
  ${TRAJ}
  ${MATH}
  ${PARAM_MODULE}
)
set_target_properties(test_measurementsio_repeated_reads
  PROPERTIES
    COMPILE_DEFINITIONS MEASUREMENTS_IO_TEST_REPEATED_READS
)
target_link_libraries(test_measurementsio_repeated_reads ${LIBM})
add_dependencies(test_measurementsio_repeated_reads ParameterParser)

#test_measurementsio_repeated_read_writes
add_executable(test_measurementsio_repeated_read_writes
  ${COMMON}
  ${MYLIB}
  ${MEASUREMENTS_IO}
  ${TRAJ}
  ${MATH}
  ${PARAM_MODULE}
)
set_target_properties(test_measurementsio_repeated_read_writes
  PROPERTIES
    COMPILE_DEFINITIONS MEASUREMENTS_IO_TEST_REPEATED_READ_WRITES
)
target_link_libraries(test_measurementsio_repeated_read_writes ${LIBM})
add_dependencies(test_measurementsio_repeated_read_writes ParameterParser)

#
# INSTALL
#

if(WIN32)
  set(DEST_BIN    "bin")
  set(DEST_LIB    "lib")
  set(DEST_UI     ".")
  set(DEST_MATLAB ".")
  set(DEST_PYTHON ".")
else()
  set(DEST_BIN "bin/whisk")
  set(DEST_LIB "lib/whisk")
  set(DEST_UI  "bin/whisk")
  set(DEST_MATLAB "share/whisk")
  set(DEST_PYTHON "share/whisk")
endif()

set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
include(InstallRequiredSystemLibraries)
# show(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
# show(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP)
# show(CMAKE_INSTALL_DEBUG_LIBRARIES)
# show(CMAKE_INSTALL_MFC_LIBRARIES)
# show(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION)
# show(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS)

install(
  TARGETS 
    trace
    measure
    classify
    classify_radial
    reclassify
  RUNTIME DESTINATION ${DEST_BIN}
  COMPONENT commandline
)
install(
    FILES ${FFMPEG_SHARED_LIBS}
    DESTINATION "${DEST_BIN}"
    COMPONENT commandline)
install(
  TARGETS
    report
    whisker_convert
    measurements_convert
  RUNTIME DESTINATION ${DEST_BIN}
  COMPONENT tools
)
install(
  TARGETS
    whisk
    traj
    whiskio
  LIBRARY DESTINATION ${DEST_LIB}
  ARCHIVE DESTINATION ${DEST_LIB}
  COMPONENT libraries
)
install(
    FILES ${FFMPEG_SHARED_LIBS}
    DESTINATION ${DEST_LIB}
    COMPONENT libraries)

install(DIRECTORY matlab
    DESTINATION ${DEST_MATLAB}
    COMPONENT matlab
)
if(MATLAB_FOUND)
  install(
      TARGETS
        mexLoadMeasurements
        mexSaveMeasurements
        mexLoadWhiskers
      DESTINATION ${DEST_MATLAB}/matlab
      COMPONENT matlab
      )
  install(PROGRAMS
    ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
    DESTINATION ${DEST_MATLAB}/matlab
    COMPONENT matlab
    )
endif()

install(DIRECTORY python
    DESTINATION ${DEST_PYTHON}
    COMPONENT python
)
install(TARGETS
    whisk
    RUNTIME DESTINATION "${DEST_PYTHON}/python"
    LIBRARY DESTINATION "${DEST_PYTHON}/python"
    ARCHIVE DESTINATION "${DEST_PYTHON}/python"
    COMPONENT python)
install(
    FILES ${FFMPEG_SHARED_LIBS}
    DESTINATION "${DEST_PYTHON}/python"
    COMPONENT python)
install(PROGRAMS
    ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
    DESTINATION "${DEST_PYTHON}/python"
    COMPONENT python)


install(DIRECTORY ui
    DESTINATION ${DEST_UI}
    COMPONENT GUI)
install(
    TARGETS
      whisk
    RUNTIME DESTINATION "${DEST_UI}/ui"
    LIBRARY DESTINATION "${DEST_UI}/ui"
    ARCHIVE DESTINATION "${DEST_UI}/ui"
    COMPONENT GUI)
install(
    FILES
      python/trace.py
      python/traj.py
      ${FFMPEG_SHARED_LIBS}
    DESTINATION "${DEST_UI}/ui"
    COMPONENT GUI)
install(PROGRAMS
    ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
    DESTINATION "${DEST_UI}/ui"
    COMPONENT GUI)

#
# PACKAGING
#


SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Whisker tracking")
SET(CPACK_PACKAGE_VENDOR "Howard Hughes Medical Institute")

SET(CPACK_OSX_PACKAGE_VERSION 10.5)
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/PACKAGE.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
SET(CPACK_RESOURCE_FILE_README  "${CMAKE_CURRENT_SOURCE_DIR}/PACKAGE.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR ${WHISK_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${WHISK_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${WHISK_VERSION_PATCH})
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "WhiskerTracking")
#SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/ui/icon.png")
IF(WIN32 AND NOT UNIX)
#SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\ui\\icon.png")
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\trace.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY}")
  SET(CPACK_NSIS_HELP_LINK "http://wiki.int.janelia.org/wiki/display/MyersLab/Whisker+Tracking")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http://wiki.int.janelia.org/wiki/display/MyersLab/Whisker+Tracking")
  SET(CPACK_NSIS_CONTACT "clackn@janelia.hhmi.org")
  SET(CPACK_NSIS_MODIFY_PATH ON)
ELSE(WIN32 AND NOT UNIX)
ENDIF(WIN32 AND NOT UNIX)

include(CPack)
cpack_add_install_type(minimal_install
  DISPLAY_NAME "Minimal"
)

cpack_add_install_type(everything_install
  DISPLAY_NAME "Everything"
)

cpack_add_component_group(Core
  DISPLAY_NAME Core
  EXPANDED
  BOLD_TITLE
)

cpack_add_component_group(Extras
  DISPLAY_NAME Extras
  EXPANDED
  BOLD_TITLE
)

cpack_add_component(
  commandline
  DISPLAY_NAME "Command line tools"
  DESCRIPTION  "These are the core tools for automating whisker tracking."
  REQUIRED
  INSTALL_TYPES minimal_install everything_install
  GROUP Core
)

cpack_add_component(
  GUI
  DISPLAY_NAME "Graphical interface."
  DESCRIPTION  "Python based GUI for proofreading."
  REQUIRED
  INSTALL_TYPES minimal_install everything_install
  GROUP Core
)

cpack_add_component(
  tools
  DISPLAY_NAME "Tools"
  DESCRIPTION  "Utilities for manipulating files"  
  DISABLED
  INSTALL_TYPES everything_install
  GROUP Extras
)

cpack_add_component(
  matlab
  DISPLAY_NAME "Matlab Interface"
  DESCRIPTION  "Matlab scripts and mex-files for reading and writing tracking files."
  DISABLED
  INSTALL_TYPES everything_install
  GROUP Extras
)

cpack_add_component(
  python
  DISPLAY_NAME "Python Interface"
  DESCRIPTION  "Python interface to the tracing and tracking libraries."
  DISABLED
  INSTALL_TYPES everything_install
  GROUP Extras
)

cpack_add_component(
  libraries
  DISPLAY_NAME "Libraries"
  DESCRIPTION  "Currently, these libraries are used for interfacing with other languages like Matlab and Python."
  DISABLED
  INSTALL_TYPES everything_install
  GROUP Extras
)


#SET(CPACK_PACKAGE_EXECUTABLES "MyExecutable" "My Executable") # for Start Menu links



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
