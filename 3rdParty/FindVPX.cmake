# Locate libvpx libraries
# This module defines
# VPX_LIBRARY, the name of the library to link against
# VPX_FOUND, if false, do not try to link
# VPX_INCLUDE_DIR, where to find header
#

set( VPX_FOUND "NO" )

find_path( VPX_INCLUDE_DIR vpx/vp8.h
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

find_library( VPX_LIBRARY
  NAME vpx
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

if(VPX_LIBRARY)
set( VPX_FOUND "YES" )
endif(VPX_LIBRARY)
