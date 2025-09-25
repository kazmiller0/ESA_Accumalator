#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "mcl::mcl" for configuration "Debug"
set_property(TARGET mcl::mcl APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(mcl::mcl PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libmcl.3.3.dylib"
  IMPORTED_SONAME_DEBUG "@rpath/libmcl.3.dylib"
  )

list(APPEND _cmake_import_check_targets mcl::mcl )
list(APPEND _cmake_import_check_files_for_mcl::mcl "${_IMPORT_PREFIX}/lib/libmcl.3.3.dylib" )

# Import target "mcl::mcl_st" for configuration "Debug"
set_property(TARGET mcl::mcl_st APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(mcl::mcl_st PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libmcl.a"
  )

list(APPEND _cmake_import_check_targets mcl::mcl_st )
list(APPEND _cmake_import_check_files_for_mcl::mcl_st "${_IMPORT_PREFIX}/lib/libmcl.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
