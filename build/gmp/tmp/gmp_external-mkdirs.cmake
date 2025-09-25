# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/kazmiller/CLionProjects/untitled/third_party/gmp/GMP-6.2.1")
  file(MAKE_DIRECTORY "/Users/kazmiller/CLionProjects/untitled/third_party/gmp/GMP-6.2.1")
endif()
file(MAKE_DIRECTORY
  "/Users/kazmiller/CLionProjects/untitled/build/gmp/src/gmp_external-build"
  "/Users/kazmiller/CLionProjects/untitled/build/gmp"
  "/Users/kazmiller/CLionProjects/untitled/build/gmp/tmp"
  "/Users/kazmiller/CLionProjects/untitled/build/gmp/src/gmp_external-stamp"
  "/Users/kazmiller/CLionProjects/untitled/build/gmp/src"
  "/Users/kazmiller/CLionProjects/untitled/build/gmp/src/gmp_external-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/kazmiller/CLionProjects/untitled/build/gmp/src/gmp_external-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/kazmiller/CLionProjects/untitled/build/gmp/src/gmp_external-stamp${cfgdir}") # cfgdir has leading slash
endif()
