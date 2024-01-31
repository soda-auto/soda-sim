include("${CMAKE_CURRENT_LIST_DIR}/soda-sim-proto-v1-targets.cmake")


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was SodaSimProtoV1Config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

set(SodaSimProtoV1_VERSION "")
set(SodaSimProtoV1_INSTALL_PATH "${PACKAGE_PREFIX_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SodaSimProtoV1 REQUIRED_VARS SodaSimProtoV1_INSTALL_PATH
                                  VERSION_VAR SodaSimProtoV1_VERSION)

check_required_components(SodaSimProtoV1)
