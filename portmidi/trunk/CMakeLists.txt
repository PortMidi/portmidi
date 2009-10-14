# portmidi
# Roger B. Dannenberg
# 20 Sep 2009

cmake_minimum_required(VERSION 2.6)

if(UNIX)
  # allow user to set Release or Debug
  set(CMAKE_BUILD_TYPE Release CACHE STRING 
      "Semicolon-separate list of supported configuration types")
  # for Makefile (more stuff that doesn't work)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY pm_test/${CMAKE_BUILD_TYPE}
      CACHE STRING "executables go here")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BUILD_TYPE}
      CACHE STRING "libraries go here")
else(UNIX)
  # this does not seem to work for xcode:
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING 
      "Semicolon-separate list of supported configuration types")
endif(UNIX)

#set(CMAKE_RELATIVE_PATHS ON CACHE STRING "avoid absolute paths" FORCE)

# Clear out the built-in C++ compiler and link flags for each of the 
# unused configurations.
set(CMAKE_CXX_FLAGS_MINSIZEREL "" CACHE INTERNAL "Unused")
set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "" CACHE INTERNAL "Unused")
set(CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL "" CACHE INTERNAL "Unused")
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "" CACHE INTERNAL "Unused")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "" CACHE INTERNAL "Unused")
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "" CACHE INTERNAL "Unused")
set(CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO "" CACHE INTERNAL "Unused")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "" CACHE INTERNAL "Unused")

set(CMAKE_OSX_ARCHITECTURES i386 ppc CACHE STRING "do not build for 64-bit" FORCE)

PROJECT(portmidi)
if(UNIX)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY 
      ${PROJECT_BINARY_DIR}/pm_test/${CMAKE_BUILD_TYPE}
      CACHE STRING "executables go here")
endif(UNIX)

include_directories(pm_common porttime)
add_subdirectory(pm_common)

add_subdirectory(pm_test)

# Cannot figure out how to make an xcode Java application with CMake
add_subdirectory(pm_java)

