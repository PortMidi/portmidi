# pm_common

# set the build directory for libportmidi.a to be in portmidi, not in 
#    portmidi/pm_common
if(APPLE OR WIN32)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  # set the build directory for .dylib libraries
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
else(APPLE OR WIN32)
  set(LINUX_FLAGS "-DPMALSA")
endif(APPLE OR WIN32)

macro(prepend_path RESULT PATH)
  set(${RESULT})
  foreach(FILE ${ARGN})
    list(APPEND ${RESULT} "${PATH}${FILE}")
  endforeach(FILE)
endmacro(prepend_path)

message(STATUS "Flags before setting, debug: " ${CMAKE_C_FLAGS_DEBUG}
	       " release " ${CMAKE_C_FLAGS_RELEASE} " general "
	       ${CMAKE_C_FLAGS})

set(CMAKE_C_FLAGS_DEBUG "-DPM_CHECK_ERRORS=1 -DDEBUG ${LINUX_FLAGS}"
    CACHE STRING "enable extra checks for debugging" FORCE)

set(CMAKE_C_FLAGS_RELEASE ${LINUX_FLAGS}
    CACHE STRING "flags for release version" FORCE)

# first include the appropriate system-dependent file:
if(UNIX)
  # add the -g switch for Linux and Mac OS X (not used in Win32)
  set (CMAKE_C_FLAGS_DEBUG "-g ${CMAKE_C_FLAGS_DEBUG}" 
       CACHE STRING "enable extra checks for debugging" FORCE)
  if(APPLE)
    set(MACSRC pmmacosxcm pmmac readbinaryplist finddefault)
    prepend_path(LIBSRC ../pm_mac/ ${MACSRC})
    list(APPEND LIBSRC ../porttime/ptmacosx_mach)

    include_directories(${CMAKE_OSX_SYSROOT}/Developer/Headers/FlatCarbon)
    set(FRAMEWORK_PATH ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks)
    set(COREAUDIO_LIB "${FRAMEWORK_PATH}/CoreAudio.framework")
    set(COREFOUNDATION_LIB "${FRAMEWORK_PATH}/CoreFoundation.framework")
    set(COREMIDI_LIB "${FRAMEWORK_PATH}/CoreMIDI.framework")
    set(CORESERVICES_LIB "${FRAMEWORK_PATH}/CoreServices.framework")
    set(PM_NEEDED_LIBS ${COREAUDIO_LIB} ${COREFOUNDATION_LIB}
                             ${COREMIDI_LIB} ${CORESERVICES_LIB}
        CACHE INTERNAL "")

    set(JAVAVM_LIB "${FRAMEWORK_PATH}/JavaVM.framework")
    set(JAVA_INCLUDE_PATHS ${JAVAVM_LIB}/Headers)
    message(STATUS "SYSROOT: " ${CMAKE_OSX_SYSROOT})
  else(APPLE)
    # LINUX settings...
    include(FindJNI)
    # message(STATUS "JAVA_JVM_LIB_PATH is " ${JAVA_JVM_LIB_PATH})
    # message(STATUS "JAVA_INCLUDE_PATH is " ${JAVA_INCLUDE_PATH})
    # note: should use JAVA_JVM_LIB_PATH, but it is not set properly
    # note: user might need to set JAVA_INCLUDE_PATH manually
    #
    # this will probably break on BSD and other Unix systems; the fix
    # depends on whether FindJNI can find Java or not. If yes, then
    # we should try to rely on automatically set JAVA_INCLUDE_PATH and
    # JAVA_INCLUDE_PATH2; if no, then we need to make both JAVA_INCLUDE_PATH
    # and JAVA_INCLUDE_PATH2 set by user (will need clear documentation
    # because JAVA_INCLUDE_PATH2 is pretty obscure)
    set(JAVA_INCLUDE_PATH  ${JAVA_INCLUDE_PATH-UNKNOWN}
        CACHE STRING "where to find Java SDK include directory")
    set(JAVA_INCLUDE_PATHS ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH}/linux)
    # libjvm.so is found relative to JAVA_INCLUDE_PATH:
    set(JAVAVM_LIB ${JAVA_INCLUDE_PATH}/../jre/lib/i386/client/libjvm.so)

    set(LINUXSRC pmlinuxalsa pmlinux finddefault)
    prepend_path(LIBSRC ../pm_linux/ ${LINUXSRC})
    list(APPEND LIBSRC ../porttime/ptlinux)

    set(PM_NEEDED_LIBS pthread asound)
  endif(APPLE)
else(UNIX)
  if(WIN32)
    include(FindJNI)
    # note: should use JAVA_JVM_LIB_PATH, but it is not set properly
    set(JAVAVM_LIB ${JAVA_INCLUDE_PATH}/../lib/jvm.lib)

    set(JAVA_INCLUDE_PATHS ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
    message(STATUS "JAVA_INCLUDE_PATHS: " ${JAVA_INCLUDE_PATHS})

    set(WINSRC pmwin pmwinmm)
    prepend_path(LIBSRC ../pm_win/ ${WINSRC})
    list(APPEND LIBSRC ../porttime/ptwinmm)
    set(PM_NEEDED_LIBS winmm.lib)
    message(STATUS "JAVAVM_LIB: " ${JAVAVM_LIB})
  endif(WIN32)
endif(UNIX)
set(JNI_EXTRA_LIBS ${PM_NEEDED_LIBS} ${JAVAVM_LIB})

# this completes the list of library sources by adding shared code
list(APPEND LIBSRC pmutil portmidi)

# now add the shared files to make the complete list of library sources
add_library(portmidi-static ${LIBSRC})
set_target_properties(portmidi-static PROPERTIES OUTPUT_NAME "portmidi_s")
target_link_libraries(portmidi-static ${PM_NEEDED_LIBS})

add_library(portmidi-dynamic SHARED ${LIBSRC})
set_target_properties(portmidi-dynamic PROPERTIES OUTPUT_NAME "portmidi")
target_link_libraries(portmidi-dynamic ${PM_NEEDED_LIBS})


# define the jni library
include_directories(${JAVA_INCLUDE_PATHS})

set(JNISRC ${LIBSRC} ../pm_java/pmjni/pmjni.c)
add_library(pmjni SHARED ${JNISRC})
target_link_libraries(pmjni ${JNI_EXTRA_LIBS})
set_target_properties(pmjni PROPERTIES EXECUTABLE_EXTENSION "jnilib")

# install the libraries (Linux only)
if(UNIX AND NOT APPLE)
  INSTALL(TARGETS portmidi-static portmidi-dynamic pmjni
    LIBRARY DESTINATION /usr/local/lib
    ARCHIVE DESTINATION /usr/local/lib)
  INSTALL(FILES portmidi.h ../porttime/porttime.h
    DESTINATION /usr/local/include)
endif(UNIX AND NOT APPLE)
