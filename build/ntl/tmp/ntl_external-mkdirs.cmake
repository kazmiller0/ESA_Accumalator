# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/kazmiller/CLionProjects/untitled/third_party/ntl")
  file(MAKE_DIRECTORY "/Users/kazmiller/CLionProjects/untitled/third_party/ntl")
endif()
file(MAKE_DIRECTORY
  "/Users/kazmiller/CLionProjects/untitled/build/ntl/src/ntl_external-build"
  "/Users/kazmiller/CLionProjects/untitled/build/ntl"
  "/Users/kazmiller/CLionProjects/untitled/build/ntl/tmp"
  "/Users/kazmiller/CLionProjects/untitled/build/ntl/src/ntl_external-stamp"
  "/Users/kazmiller/CLionProjects/untitled/build/ntl/src"
  "/Users/kazmiller/CLionProjects/untitled/build/ntl/src/ntl_external-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/kazmiller/CLionProjects/untitled/build/ntl/src/ntl_external-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/kazmiller/CLionProjects/untitled/build/ntl/src/ntl_external-stamp${cfgdir}") # cfgdir has leading slash
endif()
