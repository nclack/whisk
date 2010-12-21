# Usage
# mylib_preprocess(CFILES PFILES MANAGER [GENERATOR])
#
# Input:
# PFILES    is a list of .p files to transform.
# MANAGER   is the path to the manager.awk file
# GENERATOR is the path to the generator.awk file
#
# Output:
# CFILES    is a list of the generated files.
macro(mylib_preprocess CFILES_OUT PFILES MANAGER)
  set(${GENERATOR} "")
  #message(STATUS "** ARGC: ${ARGC}")
  #message(STATUS "** PFILES: ${${PFILES}}")
  #message(STATUS "** MANAGER: ${${MANAGER}}")
  if(${ARGC}>3) # use generator.awk
    set(${GENERATOR} " | awk -f ${ARGV4}")
  endif()
  #message(STATUS "** GENERATOR: ${GENERATOR}")
  foreach(f ${${PFILES}})
    string(REGEX REPLACE \\.p$ .c cfilet ${f})
    get_filename_component(cfile ${cfilet} NAME)
    #message(":: ${f} --> ${PROJECT_BINARY_DIR}/${cfile}")
    set(${CFILES_OUT} ${${CFILES_OUT}} ${PROJECT_BINARY_DIR}/${cfile})
    add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/${cfile}
      COMMAND awk -f ${${MANAGER}} ${f} ${GENERATOR} > ${PROJECT_BINARY_DIR}/${cfile} 
      DEPENDS ${f}
      )
  endforeach(f)
  set_source_files_properties(${${CFILES_OUT}} PROPERTIES GENERATED 1)
  #message(":: ${CFILES_OUT} is ${${CFILES_OUT}}")
endmacro(mylib_preprocess)

