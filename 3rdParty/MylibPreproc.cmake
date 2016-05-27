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

find_program(AWK awk)
#set(AWK AWK-NOTFOUND)

macro(mylib_preprocess CFILES_OUT PFILES MANAGER)
    if(NOT AWK)
        message("awk not found.  Using previously generated mylib c files in ${CMAKE_CURRENT_LIST_DIR}/generated.")
        set(${CFILES_OUT}
            ${CMAKE_CURRENT_LIST_DIR}/src/generated/contour_lib.c
            ${CMAKE_CURRENT_LIST_DIR}/src/generated/draw_lib.c
            ${CMAKE_CURRENT_LIST_DIR}/src/generated/image_filters.c
            ${CMAKE_CURRENT_LIST_DIR}/src/generated/image_lib.c
            ${CMAKE_CURRENT_LIST_DIR}/src/generated/level_set.c
            ${CMAKE_CURRENT_LIST_DIR}/src/generated/utilities.c
            ${CMAKE_CURRENT_LIST_DIR}/src/generated/water_shed.c
        )
    else()
        set(${GENERATOR} "")
        #message(STATUS "** ARGC: ${ARGC}")
        #message(STATUS "** PFILES: ${${PFILES}}")
        #message(STATUS "** MANAGER: ${${MANAGER}}")
        if(${ARGC}>3) # use generator.awk
          set(${GENERATOR} " | ${AWK} -f ${ARGV4}")
        endif()
        #message(STATUS "** GENERATOR: ${GENERATOR}")
        foreach(f ${${PFILES}})
          string(REGEX REPLACE \\.p$ .c cfilet ${f})
          get_filename_component(cfile ${cfilet} NAME)
          #message(":: ${f} --> ${PROJECT_BINARY_DIR}/${cfile}")
          set(${CFILES_OUT} ${${CFILES_OUT}} ${PROJECT_BINARY_DIR}/${cfile})
          add_custom_command(
            OUTPUT ${PROJECT_BINARY_DIR}/${cfile}
            COMMAND ${AWK} -f ${${MANAGER}} ${f} ${GENERATOR} > ${PROJECT_BINARY_DIR}/${cfile}
            DEPENDS ${f}
            )
        endforeach(f)
        set_source_files_properties(${${CFILES_OUT}} PROPERTIES GENERATED 1)
        #message(":: ${CFILES_OUT} is ${${CFILES_OUT}}")
    endif()
endmacro(mylib_preprocess)
