# Locate libva libraries
# This module defines
# SCHROEDINGER_LIBRARY, the name of the library to link against
# SCHROEDINGER_FOUND, if false, do not try to link
# SCHROEDINGER_INCLUDE_DIR, where to find header
#

set( SCHROEDINGER_FOUND "NO" )

find_path( SCHROEDINGER_INCLUDE_DIR schroedinger-1.0/schroedinger/schro.h
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

find_library( SCHROEDINGER_LIBRARY
  NAME schroedinger-1.0
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

if(SCHROEDINGER_LIBRARY)
set( SCHROEDINGER_FOUND "YES" )
endif(SCHROEDINGER_LIBRARY)
