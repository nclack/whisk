# Locate libx264 library  
# This module defines
# X264_LIBRARY, the name of the library to link against
# X264_FOUND, if false, do not try to link
# X264_INCLUDE_DIR, where to find header
#

set( X264_FOUND "NO" )

find_path( X264_INCLUDE_DIR x264.h
  HINTS
  PATH_SUFFIXES include 
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include
  /usr/include
  /sw/include
  /opt/local/include
  /opt/csw/include 
  /opt/include
  /mingw
)

find_library( X264_LIBRARY
  NAMES x264
  HINTS
  PATH_SUFFIXES lib64 lib
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)

if(X264_LIBRARY)
  set( X264_FOUND "YES" )
endif(X264_LIBRARY)
