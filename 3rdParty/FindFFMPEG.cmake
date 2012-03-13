# Locate ffmpeg
#
# Output
# ------
# FFMPEG_LIBRARIES
# FFMPEG_FOUND
# FFMPEG_INCLUDE_DIR
# FFMPEG_SHARED_LIBS
#       dynamically loaded libraries that need to be
#       distributed with any binaries
#
# Input
# -----
# ROOT_3RDPARTY_DIR
#    Location of 3rd party libraries for a project
#    It is searched last, so system libraries will be have priority.
# FFMPEG_DIR       
#    an environment variable that would
#    correspond to the ./configure --prefix=$FFMPEG_DIR
#
# Created by Robert Osfield.
# Modified by Nathan Clack.


# Notes
# -----
# In ffmpeg code, old version use "#include <header.h>" and newer use "#include <libname/header.h>"
# Use "#include <header.h>" for compatibility with old version of ffmpeg
# We have to search the path which contain the header.h (usefull for old version)
# and search the path which contain the libname/header.h (usefull for new version)
# Then we need to include ${FFMPEG_libname_INCLUDE_DIRS} (in old version case, use by ffmpeg header and osg plugin code)
#                                                       (in new version case, use by ffmpeg header) 
# and ${FFMPEG_libname_INCLUDE_DIRS/libname}             (in new version case, use by osg plugin code)


if(WIN32)
  set(HASH a4c22e3)
  if(CMAKE_CL_64)
    set(SYS   ffmpeg-git-${HASH}-win64-dev)
    set(DLLS  ffmpeg-git-${HASH}-win64-shared)
  else()
    set(SYS ffmpeg-git-${HASH}-win32-dev)
    set(DLLS ffmpeg-git-${HASH}-win32-shared)
  endif()

  set(FFMPEG_INCLUDE_PATH_SUFFIXES
            ffmpeg/${SYS}/include
            ffmpeg/${SYS}/include/lib${shortname} 
  )
  set(FFMPEG_LIB_PATH_SUFFIXES
            ffmpeg/${SYS}/lib
            ffmpeg/${SYS}/lib/lib${shortname}
  )
  file(GLOB _FFMPEG_SHARED_LIBS ${ROOT_3RDPARTY_DIR}/ffmpeg/${DLLS}/bin/*.dll)

  #get path to inttypes
  FIND_PATH(INTTYPES_INCLUDE_DIR inttypes.h
      HINTS ${ROOT_3RDPARTY_DIR}
      PATH_SUFFIXES inttypes
      )
else()
  set(FFMPEG_INCLUDE_PATH_SUFFIXES
            ffmpeg
            ffmpeg/lib${shortname}
  )
  set(FFMPEG_LIB_PATH_SUFFIXES
            lib
            lib64
  )
endif()

if(APPLE) #FFMPEG may rely on these Frameworks
  find_library(CF_LIBS  CoreFoundation)
  find_library(VDA_LIBS VideoDecodeAcceleration)
  find_library(CV_LIBS CoreVideo)
endif()

set(FFMPEG_SHARED_LIBS ${_FFMPEG_SHARED_LIBS})

# Macro to find header and lib directories
# example: FFMPEG_FIND(AVFORMAT avformat avformat.h)
MACRO(FFMPEG_FIND varname shortname headername)
#message("++ ROOT: ${ROOT_3RDPARTY_DIR}")
#message("++     : ${headername}")
#message("++     : ${shortname}")
    FIND_PATH(FFMPEG_${varname}_INCLUDE_DIRS lib${shortname}/${headername}
        HINTS
          ${ROOT_3RDPARTY_DIR}
        PATHS
          ${FFMPEG_ROOT}/include
          $ENV{FFMPEG_DIR}/include
          ~/Library/Frameworks
          /Library/Frameworks
          /usr/local/include
          /usr/include
          /sw/include # Fink
          /opt/local/include # DarwinPorts
          /opt/csw/include # Blastwave
          /opt/include
          /usr/freeware/include
        PATH_SUFFIXES 
          ${FFMPEG_INCLUDE_PATH_SUFFIXES}
        DOC "Location of FFMPEG Headers"
        NO_DEFAULT_PATH
    )

    FIND_FILE(FFMPEG_${varname}_LIBRARIES
        NAMES
          "${shortname}.lib"
          "lib${shortname}.a"
        HINTS
          ${ROOT_3RDPARTY_DIR}
        PATHS
          ${FFMPEG_ROOT}
          $ENV{FFMPEG_DIR}
          ~/Library/Frameworks
          /Library/Frameworks
          /usr/local
          /usr
          /sw
          /opt/local
          /opt/csw
          /opt
          /usr/freeware
        PATH_SUFFIXES 
          ${FFMPEG_LIB_PATH_SUFFIXES}
        DOC "Location of FFMPEG Libraries"
    )

  #message("++ ${FFMPEG_${varname}_LIBRARIES}")
  #message("++ ${FFMPEG_${varname}_INCLUDE_DIRS}")
    IF (FFMPEG_${varname}_LIBRARIES AND FFMPEG_${varname}_INCLUDE_DIRS)
        SET(FFMPEG_${varname}_FOUND 1)
    ENDIF(FFMPEG_${varname}_LIBRARIES AND FFMPEG_${varname}_INCLUDE_DIRS)

ENDMACRO(FFMPEG_FIND)

SET(FFMPEG_ROOT "$ENV{FFMPEG_DIR}" CACHE PATH "Location of FFMPEG")

FFMPEG_FIND(LIBAVFORMAT avformat avformat.h)
FFMPEG_FIND(LIBAVDEVICE avdevice avdevice.h)
FFMPEG_FIND(LIBAVCODEC  avcodec  avcodec.h)
FFMPEG_FIND(LIBAVUTIL   avutil   avutil.h)
FFMPEG_FIND(LIBSWSCALE  swscale  swscale.h)

SET(FFMPEG_FOUND "NO")
IF   (FFMPEG_LIBAVFORMAT_FOUND AND FFMPEG_LIBAVDEVICE_FOUND AND FFMPEG_LIBAVCODEC_FOUND AND FFMPEG_LIBAVUTIL_FOUND AND FFMPEG_LIBSWSCALE_FOUND)

    SET(FFMPEG_FOUND "YES")

    SET(FFMPEG_INCLUDE_DIRS 
        ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS}
        ${INTTYPES_INCLUDE_DIR})

    SET(FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBAVFORMAT_LIBRARY_DIRS})

    if(WIN32) 
      get_filename_component(
          FFMPEG_KITCHEN_SINK_PATH
          ${FFMPEG_LIBAVCODEC_LIBRARIES}
          PATH)
      FILE(GLOB FFMPEG_LIBRARIES ${FFMPEG_KITCHEN_SINK_PATH}/*.lib)
#message("FFMPEG_KITCHEN_SINK_PATH is ${FFMPEG_KITCHEN_SINK_PATH}")        
#message("FFMPEG_LIBRARIES is ${FFMPEG_LIBRARIES}")
    else()
      find_package(ZLIB)
      find_package(BZip2)
      find_package(Vorbis)
      find_package(Lame)
      find_package(x264)
      find_package(xvid)
      find_package(theora)
      find_package(va)
      find_package(VPX)
      find_package(Schroedinger)
      find_package(Speex)
      find_package(GSM)

      set(CMAKE_THREAD_PREFER_PTHREAD)
      find_package(Threads)
      set(THREAD_LIBRARY ${CMAKE_THREAD_LIBS_INIT})

macro(ADDLIB VAR NAME)
  if(${NAME}_FOUND)
    set(${VAR} ${${VAR}} ${${NAME}_LIBRARY})
  endif()
endmacro()
macro(ADDLIBS VAR NAME)
  if(${NAME}_FOUND)
    set(${VAR} ${${VAR}} ${${NAME}_LIBRARIES})
  endif()
endmacro()

      SET(FFMPEG_LIBRARIES
          ${FFMPEG_DLLS}
          ${FFMPEG_LIBAVFORMAT_LIBRARIES}
          ${FFMPEG_LIBAVDEVICE_LIBRARIES}
          ${FFMPEG_LIBAVCODEC_LIBRARIES}
          ${FFMPEG_LIBSWSCALE_LIBRARIES}
          ${FFMPEG_LIBAVUTIL_LIBRARIES}
          ${ZLIB_LIBRARY}
          ${BZIP2_LIBRARIES}
          ${OGG_LIBRARY}
          ${VORBIS_LIBRARIES}
          ${Lame_LIBRARY}
          ${X264_LIBRARY}
          ${XVID_LIBRARY}
          ${THREAD_LIBRARY}
          ${CF_LIBS}
          ${VDA_LIBS}
          ${CV_LIBS}
          )
      ADDLIBS(FFMPEG_LIBRARIES THEORA)
      ADDLIB (FFMPEG_LIBRARIES VAAPI)
      ADDLIB (FFMPEG_LIBRARIES VPX)
      ADDLIB (FFMPEG_LIBRARIES SCHROEDINGER)
      ADDLIB (FFMPEG_LIBRARIES SPEEX)
      ADDLIB (FFMPEG_LIBRARIES GSM)
    endif()
    #message("FFMPEG_LIBRARIES are ${FFMPEG_LIBRARIES}")        

ELSE ()

    MESSAGE(STATUS "Could not find FFMPEG")

ENDIF()
