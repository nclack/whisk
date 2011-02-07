# Locate libraries
# This module defines
# SPEEX_LIBRARY, the name of the library to link against
# SPEEX_FOUND, if false, do not try to link
# SPEEX_INCLUDE_DIR, where to find header
#

set( SPEEX_FOUND "NO" )

find_path( SPEEX_INCLUDE_DIR speex/speex.h
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

find_library( SPEEX_LIBRARY
  NAME speex
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

if(SPEEX_LIBRARY)
set( SPEEX_FOUND "YES" )
endif(SPEEX_LIBRARY)
