# pm_common

# set the build directory for libportmidi.a to be in portmidi, not in 
#    portmidi/pm_common
if(APPLE OR WIN32)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  # set the build directory for .dylib libraries
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

# Debugging code to figure out what CMake is doing
  # the first time CMake configures, save off CMake's built-in flags
#  if(NOT DEFAULT_DEBUG_FLAGS)
#    set(DEFAULT_DEBUG_FLAGS ${CMAKE_C_FLAGS_DEBUG} CACHE 
#        STRING "CMake's default debug flags" FORCE)
#    set(DEFAULT_RELEASE_FLAGS ${CMAKE_C_FLAGS_RELEASE} CACHE 
#        STRING "CMake's default release flags" FORCE)
#  else(NOT DEFAULT_DEBUG_FLAGS)
#    message(STATUS "DEFAULT_DEBUG_FLAGS not nil: " ${DEFAULT_DEBUG_FLAGS})
#  endif(NOT DEFAULT_DEBUG_FLAGS)
else(APPLE OR WIN32)
  set(LINUX_FLAGS "-DPMALSA" CACHE STRING "must define either PMALSA or PMNULL")
endif(APPLE OR WIN32)

if(APPLE)
  set(PM_OSX_VERSION "10.7" CACHE STRING
      "selects PM_OSX_SDK and macosx-version-min C flag")
  set(PM_OSX_SDK "/Developer/SDKs/MacOSX${PM_OSX_VERSION}.sdk")
  set(CMAKE_OSX_SYSROOT ${PM_OSC_SDK} CACHE 
      PATH "-isysroot parameter for compiler")
  set(CMAKE_C_FLAGS "-mmacosx-version-min=${PM_OSC_VERSION}" CACHE 
      STRING "needed in conjunction with CMAKE_OSX_SYSROOT" FORCE)
  # option(OSX_RPATH "control CMake policy for finding macOS dynamic libraries"
  #        ON)  # enabled here to match CMake default. Is this correct?
endif(APPLE)

macro(prepend_path RESULT PATH)
  set(${RESULT})
  foreach(FILE ${ARGN})
    list(APPEND ${RESULT} "${PATH}${FILE}")
  endforeach(FILE)
endmacro(prepend_path)

# first include the appropriate system-dependent file:
if(UNIX)
  if(APPLE)
    set(MACSRC pmmacosxcm.c pmmacosxcm.h pmmac.c pmmac.h 
        readbinaryplist.c readbinaryplist.h finddefault.c) 
    prepend_path(LIBSRC ${PROJECT_SOURCE_DIR}/pm_mac/ ${MACSRC})
    list(APPEND LIBSRC ${PROJECT_SOURCE_DIR}/porttime/ptmacosx_mach.c) 

    include_directories(${CMAKE_OSX_SYSROOT}/Developer/Headers/FlatCarbon)
    set(FRAMEWORK_PATH ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks)
    set(COREAUDIO_LIB "${FRAMEWORK_PATH}/CoreAudio.framework")
    set(COREFOUNDATION_LIB "${FRAMEWORK_PATH}/CoreFoundation.framework")
    set(COREMIDI_LIB "${FRAMEWORK_PATH}/CoreMIDI.framework")
    set(CORESERVICES_LIB "${FRAMEWORK_PATH}/CoreServices.framework")
    set(NEEDED_LIBS ${COREAUDIO_LIB} ${COREFOUNDATION_LIB}
                    ${COREMIDI_LIB} ${CORESERVICES_LIB})

    if(BUILD_JAVA_NATIVE_INTERFACE)
      set(JAVAVM_LIB "${FRAMEWORK_PATH}/JavaVM.framework")
      set(JAVA_INCLUDE_PATHS ${JAVAVM_LIB}/Headers)
    endif(BUILD_JAVA_NATIVE_INTERFACE)
    message(STATUS "SYSROOT: " ${CMAKE_OSX_SYSROOT})
  else(APPLE)
    # LINUX settings...
    if(BUILD_JAVA_NATIVE_INTERFACE)
      include(FindJNI)
      message(STATUS "JAVA_INCLUDE_PATH is " ${JAVA_INCLUDE_PATH})
      message(STATUS "JAVA_INCLUDE_PATH2 is " ${JAVA_INCLUDE_PATH2})
      message(STATUS "JAVA_JVM_LIBRARY is " ${JAVA_JVM_LIBRARY})
      set(JAVA_INCLUDE_PATHS ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
      # libjvm.so is found relative to JAVA_INCLUDE_PATH:
      set(JAVAVM_LIB ${JAVA_JVM_LIBRARY}/libjvm.so)
    endif(BUILD_JAVA_NATIVE_INTERFACE)

    set(LINUXSRC pmlinuxalsa.c pmlinuxalsa.h pmlinux.c pmlinux.h
                 finddefault.c) 
    prepend_path(LIBSRC ${PROJECT_SOURCE_DIR}/pm_linux/ ${LINUXSRC})
    list(APPEND LIBSRC ${PROJECT_SOURCE_DIR}/porttime/ptlinux.c) 
    set(NEEDED_LIBS pthread asound)
  endif(APPLE)
else(UNIX)
  if(WIN32)
    # /MDd is multithread debug DLL, /MTd is multithread debug
    # /MD is multithread DLL, /MT is multithread. Change to static:
    include(../pm_win/static.cmake)
    
    if(BUILD_JAVA_NATIVE_INTERFACE)
      include(FindJNI)

      set(JAVA_INCLUDE_PATHS ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
      # message(STATUS "JAVA_INCLUDE_PATHS: " ${JAVA_INCLUDE_PATHS})
    endif(BUILD_JAVA_NATIVE_INTERFACE)

    set(WINSRC pmwin.c pmwinmm.c pmwinmm.h) 
    prepend_path(LIBSRC ${PROJECT_SOURCE_DIR}/pm_win/ ${WINSRC})
    list(APPEND LIBSRC ${PROJECT_SOURCE_DIR}/porttime/ptwinmm.c)
    set(NEEDED_LIBS winmm.lib)
  endif(WIN32)
endif(UNIX)

# this completes the list of library sources by adding shared code
prepend_path(PMSRC "${CMAKE_CURRENT_SOURCE_DIR}/"
                   pmutil.c pmutil.h portmidi.c portmidi.h pminternal.h)
list(APPEND LIBSRC ${PMSRC})

set(PM_LIB_NAME "portmidi" CACHE STRING 
    "The PortMidi library name, e.g. portmidi_static. Default is portmidi")

# now add the shared files to make the complete list of library sources
add_library(${PM_LIB_NAME} ${LIBSRC})
set(PM_LIBSRC ${LIBSRC} CACHE INTERNAL
    "accumulate and share portmidi library sources here")
set(PM_NEEDED_LIBS ${NEEDED_LIBS} CACHE INTERNAL
    "accumulate and share portmidi dependencies here")

# set to CMake default; is this right?:
set_target_properties(${PM_LIB_NAME} PROPERTIES MACOSX_RPATH ON)
target_link_libraries(${PM_LIB_NAME} ${PM_NEEDED_LIBS})

# install the libraries (Linux and Mac OS X command line)
if(UNIX)
  INSTALL(TARGETS ${PM_LIB_NAME} ${PMJNI_LIBRARY}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
# .h files installed by pm_dylib/CMakeLists.txt, so don't need them here
#  INSTALL(FILES portmidi.h ../porttime/porttime.h
#    DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif(UNIX)
