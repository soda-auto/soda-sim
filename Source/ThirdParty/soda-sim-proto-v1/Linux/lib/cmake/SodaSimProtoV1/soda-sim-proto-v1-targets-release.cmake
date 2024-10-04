#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SodaSimProtoV1::generic-msgs" for configuration "Release"
set_property(TARGET SodaSimProtoV1::generic-msgs APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SodaSimProtoV1::generic-msgs PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libsoda-sim-proto-v1.a"
  )

list(APPEND _cmake_import_check_targets SodaSimProtoV1::generic-msgs )
list(APPEND _cmake_import_check_files_for_SodaSimProtoV1::generic-msgs "${_IMPORT_PREFIX}/lib/libsoda-sim-proto-v1.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
