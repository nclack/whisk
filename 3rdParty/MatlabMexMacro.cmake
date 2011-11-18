# Macro to build mex files from cmake with the standard c/c++ compiler
MACRO(ADD_MEX_FILE Target)
  INCLUDE_DIRECTORIES(${MATLAB_INCLUDE_DIR})
  ADD_LIBRARY(${Target} SHARED ${ARGN})
  TARGET_LINK_LIBRARIES(${Target} 
    ${MATLAB_MX_LIBRARY} 
    ${MATLAB_MEX_LIBRARY} 
    ${MATLAB_MAT_LIBRARY} 
#    m - libm not necessary on windows? [ngc]
    )
  
  SET_TARGET_PROPERTIES(${Target} PROPERTIES PREFIX "")
  
  # Determine mex suffix (Should this be in FindMatlab.cmake?)
  IF(UNIX)
    IF(APPLE)
      IF(CMAKE_OSX_ARCHITECTURES MATCHES i386)
        SET(MATLAB_MEX_SUFFIX ".mexmaci")
      ELSE(CMAKE_OSX_ARCHITECTURES MATCHES i386)
        SET(MATLAB_MEX_SUFFIX ".mexmaci64")
      ENDIF(CMAKE_OSX_ARCHITECTURES MATCHES i386)
    ELSE(APPLE)
      IF(CMAKE_SIZEOF_VOID_P MATCHES "4")
        SET(MATLAB_MEX_SUFFIX ".mexglx")
      ELSEIF(CMAKE_SIZEOF_VOID_P MATCHES "8")
        SET(MATLAB_MEX_SUFFIX ".mexa64")
      ELSE(CMAKE_SIZEOF_VOID_P MATCHES "4")
        MESSAGE(FATAL_ERROR 
          "CMAKE_SIZEOF_VOID_P (${CMAKE_SIZEOF_VOID_P}) doesn't indicate a valid platform")
      ENDIF(CMAKE_SIZEOF_VOID_P MATCHES "4")
    ENDIF(APPLE)
  ELSEIF(WIN32)
    IF(CMAKE_SIZEOF_VOID_P MATCHES "4")
      SET(MATLAB_MEX_SUFFIX ".mexw32")
    ELSEIF(CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET(MATLAB_MEX_SUFFIX ".mexw64")
    ELSE(CMAKE_SIZEOF_VOID_P MATCHES "4")
      MESSAGE(FATAL_ERROR 
        "CMAKE_SIZEOF_VOID_P (${CMAKE_SIZEOF_VOID_P}) doesn't indicate a valid platform")
    ENDIF(CMAKE_SIZEOF_VOID_P MATCHES "4")
  ENDIF(UNIX)

  # Use mex suffix
  SET_TARGET_PROPERTIES(${Target} PROPERTIES SUFFIX ${MATLAB_MEX_SUFFIX})
  
  # Determine useful mex compile and link flags
  IF(MSVC)
    SET(MEX_COMPILE_FLAGS "-DMATLAB_MEX_FILE")
    SET(MEX_LINK_FLAGS "/export:mexFunction")
  ELSE(MSVC)
    SET(MEX_COMPILE_FLAGS 
      "-fPIC" "-D_GNU_SOURCE" "-pthread" "-D_FILE_OFFSET_BITS=64" "-DMX_COMPAT_32")
    
    IF(APPLE)
      SET(MEX_LINK_FLAGS 
          "-L${MATLAB_SYS} -Wl,-flat_namespace -undefined suppress")
    ELSE(APPLE)
      SET(MEX_LINK_FLAGS "-Wl,-E -Wl,--no-undefined")
    ENDIF(APPLE)
  ENDIF(MSVC)

  # Append mex compile flags to existing ones
  SD_APPEND_TARGET_PROPERTIES(${Target} COMPILE_FLAGS ${MEX_COMPILE_FLAGS})

  # Use mex link flags (Should be append them?)
  SET_TARGET_PROPERTIES(${Target} PROPERTIES LINK_FLAGS ${MEX_LINK_FLAGS})

ENDMACRO(ADD_MEX_FILE)

# Macro append a property to an existing target property
MACRO(SD_APPEND_TARGET_PROPERTIES TARGET_TO_CHANGE PROP_TO_CHANGE)
  FOREACH(_newProp ${ARGN})
    GET_TARGET_PROPERTY(_oldProps ${TARGET_TO_CHANGE} ${PROP_TO_CHANGE})
    IF(_oldProps)
      IF(NOT "${_oldProps}" MATCHES "^.*${_newProp}.*$")
        SET_TARGET_PROPERTIES(${TARGET_TO_CHANGE} PROPERTIES ${PROP_TO_CHANGE} "${_newProp} ${_oldProps}")
      ENDIF(NOT "${_oldProps}" MATCHES "^.*${_newProp}.*$")
    ELSE(_oldProps)
      SET_TARGET_PROPERTIES(${TARGET_TO_CHANGE} PROPERTIES ${PROP_TO_CHANGE} ${_newProp})
    ENDIF(_oldProps)
  ENDFOREACH(_newProp ${ARGN})
ENDMACRO(SD_APPEND_TARGET_PROPERTIES TARGET_TO_CHANGE PROP_TO_CHANGE)
