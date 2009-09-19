# portmidi
# Roger B. Dannenberg
# 20 Sep 2009

cmake_minimum_required(VERSION 2.6)

# this does not seem to work for xcode:
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING 
    "Semicolon-separate list of supported configuration types" FORCE)
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

include_directories(pm_common porttime)
add_subdirectory(pm_common)

add_subdirectory(pm_test)

# Cannot figure out how to make an xcode Java application with CMake
# add_subdirectory(pm_java)

# ---- Just for reference, here's what I had in pm_java/CMakeLists.txt ---

## pm_java
#
#PROJECT(pmdefaults Java)
#
#set(JAVA_SRC jportmidi/JPortMidiException.java
#             jportmidi/JPortMidi.java
#             jportmidi/JPortMidiApi.java
#             pmdefaults/PmDefaults.java
#             pmdefaults/PmDefaultsFrame.java)
#
#set(MACOSX_BUNDLE_ICON_FILE pmdefaults/pmdefaults.icns)
#set_source_files_properties(pmdefaults/pmdefaults.icns PROPERTIES
#           MACOSX_PACKAGE_LOCATION Resources)
#add_executable(PmDefaults MACOSX_BUNDLE ${JAVA_SRC})
#set_target_properties(PmDefaults PROPERTIES LINKER_LANGUAGE Java)
#
