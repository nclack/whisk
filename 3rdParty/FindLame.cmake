# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Whispersoft s.r.l. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# Finds Lame library
#
#  Lame_INCLUDE_DIR - where to find lame.h lame/lame.h, etc.
#  Lame_LIBRARIES   - List of libraries when using Lame.
#  Lame_FOUND       - True if Lame found.
#

if (Lame_INCLUDE_DIR)
  # Already in cache, be silent
  set(Lame_FIND_QUIETLY TRUE)
endif (Lame_INCLUDE_DIR)

find_path(Lame_INCLUDE_DIR lame/lame.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Lame_NAMES mp3lame)
find_library(Lame_LIBRARY
  NAMES ${Lame_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Lame_INCLUDE_DIR AND Lame_LIBRARY)
   set(Lame_FOUND TRUE)
   set( Lame_LIBRARIES ${Lame_LIBRARY} )
else (Lame_INCLUDE_DIR AND Lame_LIBRARY)
   set(Lame_FOUND FALSE)
   set(Lame_LIBRARIES)
endif (Lame_INCLUDE_DIR AND Lame_LIBRARY)

if (Lame_FOUND)
   if (NOT Lame_FIND_QUIETLY)
      message(STATUS "Found Lame: ${Lame_LIBRARY}")
   endif (NOT Lame_FIND_QUIETLY)
else (Lame_FOUND)
   if (Lame_FIND_REQUIRED)
      message(STATUS "Looked for Lame libraries named ${Lame_NAMES}.")
      message(STATUS "Include file detected: [${Lame_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Lame_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Lame library")
   endif (Lame_FIND_REQUIRED)
endif (Lame_FOUND)

mark_as_advanced(
  Lame_LIBRARY
  Lame_INCLUDE_DIR
  )
