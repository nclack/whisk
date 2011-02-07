# Locate libva libraries
# This module defines
# VAAPI_LIBRARY, the name of the library to link against
# VAAPI_FOUND, if false, do not try to link
# VAAPI_INCLUDE_DIR, where to find header
#

set( VAAPI_FOUND "NO" )

find_path( VAAPI_INCLUDE_DIR va/va.h
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

find_library( VAAPI_LIBRARY
  NAME va
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

if(VAAPI_LIBRARY)
set( VAAPI_FOUND "YES" )
endif(VAAPI_LIBRARY)
