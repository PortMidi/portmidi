# portmidi
# Roger B. Dannenberg
# 20 Sep 2009

cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9 CACHE STRING 
    "make for this OS version or higher")

if(UNIX)
  # allow user to set Release or Debug
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Semicolon-separate list of supported configuration types")
  # set default directories but don't override cached values...
#  these cause problems for other projects that include this one:
#  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
#      CACHE STRING "libraries go here")
#  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
#      CACHE STRING "libraries go here")
#  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
#      ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
#      CACHE STRING "executables go here")
else(UNIX)
  # this does not seem to work for xcode:
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING
      "Semicolon-separate list of supported configuration types" OFF)
endif(UNIX)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_PMDEFAULTS
       "build the legacy Java application PmDefaults" OFF)
option(BUILD_JAVA_NATIVE_INTERFACE
       "build the Java PortMidi interface library" OFF)

if(BUILD_JAVA_NATIVE_INTERFACE)
  message(WARNING
    "Java API and PmDefaults program updated 2021, but support has "
    "been discontinued. If you need/use this, let developers know.")
else(BUILD_JAVA_NATIVE_INTERFACE)
  if(BUILD_PMDEFAULTS)
    message(FATAL_ERROR
      "Cannot build PM_Defaults program (BUILD_PM_DEFAULTS) without option "
      "BUILD_JAVA_NATIVE_INTERFACE")
  endif(BUILD_PMDEFAULTS)
endif(BUILD_JAVA_NATIVE_INTERFACE)

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

set(CMAKE_OSX_ARCHITECTURES x86_64 CACHE STRING
    "change to support other architectures" FORCE)

PROJECT(portmidi DESCRIPTION "Cross-Platform MIDI IO")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DPM_CHECK_ERRORS=1 ${LINUX_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${LINUX_FLAGS}")
# set(PORTMIDI_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE STRING
#    "CMake shows the wrong value for CMAKE_C_FLAGS_DEBUG. Here it is." FORCE)
# set(PORTMIDI_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING
#     "CMake shows the wrong value for CMAKE_C_FLAGS_RELEASE. Here it is." FORCE)

if(UNIX)
  # Problem: if there was an old Debug build and you change
  #  CMAKE_BUILD_TYPE to Release, then the OUTPUT_DIRECTORY's will
  #  still be Debug. Try to fix this by checking if the DIRECTORY's
  #  look wrong, and if so, force them to the defaults:
  if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(BAD_DIR "Release")
  else(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(BAD_DIR "Debug")
  endif(CMAKE_BUILD_TYPE MATCHES "Debug")
  # use library as reference -- if you give it a non-BAD_DIR location
  # then every other path is left alone
  if(CMAKE_LIBRARY_OUTPUT_DIRECTORY MATCHES ${BAD_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
        CACHE STRING "executables go here" FORCE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
        CACHE STRING "libraries go here" FORCE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
        CACHE STRING "libraries go here" FORCE)
  endif(CMAKE_LIBRARY_OUTPUT_DIRECTORY MATCHES ${BAD_DIR})
endif(UNIX)

include_directories(pm_common porttime)
add_subdirectory(pm_common)

option(BUILD_PORTMIDI_TESTS
       "Build test programs, including midi monitor (mm)" OFF)
if(BUILD_PORTMIDI_TESTS)
    add_subdirectory(pm_test)
endif(BUILD_PORTMIDI_TESTS)

# See note above about Java support (probably) discontinued
if(BUILD_JAVA_NATIVE_INTERFACE)
  add_subdirectory(pm_java)
endif(BUILD_JAVA_NATIVE_INTERFACE)

# generate pc file for pkg-config
# See https://cmake.org/cmake/help/latest/command/configure_file.html
configure_file(${CMAKE_PROJECT_NAME}.pc.in ${CMAKE_PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
