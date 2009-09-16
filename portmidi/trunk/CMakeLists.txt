# portmidi

cmake_minimum_required(VERSION 2.6)

# this does not seem to work for xcode:
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING 
    "Semicolon-separate list of supported configuration types" FORCE)

set(CMAKE_OSX_ARCHITECTURES i386 ppc CACHE STRING "do not build for 64-bit" FORCE)
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

PROJECT(portmidi)

include_directories(pm_common porttime)
add_subdirectory(pm_common)

add_subdirectory(pm_test)

# Cannot figure out how to make an xcode Java application with CMake
# add_subdirectory(pm_java)
