#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libxmlmm" for configuration "Release"
set_property(TARGET libxmlmm APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libxmlmm PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libxmlmm.a"
  )

list(APPEND _cmake_import_check_targets libxmlmm )
list(APPEND _cmake_import_check_files_for_libxmlmm "${_IMPORT_PREFIX}/lib/libxmlmm.a" )

# Import target "LibXml2" for configuration "Release"
set_property(TARGET LibXml2 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LibXml2 PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libxml2.a"
  )

list(APPEND _cmake_import_check_targets LibXml2 )
list(APPEND _cmake_import_check_files_for_LibXml2 "${_IMPORT_PREFIX}/lib/libxml2.a" )

# Import target "libdbcppp" for configuration "Release"
set_property(TARGET libdbcppp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libdbcppp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libdbcppp.a"
  )

list(APPEND _cmake_import_check_targets libdbcppp )
list(APPEND _cmake_import_check_files_for_libdbcppp "${_IMPORT_PREFIX}/lib/libdbcppp.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
