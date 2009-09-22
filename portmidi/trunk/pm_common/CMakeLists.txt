# pm_common

# set the build directory for libportmidi.a to be in portmidi, not in 
#    portmidi/pm_common
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
# set the build directory for .dylib libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

macro(prepend_path RESULT PATH)
  set(${RESULT})
  foreach(FILE ${ARGN})
    list(APPEND ${RESULT} "${PATH}${FILE}")
  endforeach(FILE)
endmacro(prepend_path)

set(CMAKE_C_FLAGS_DEBUG "-g -DPM_CHECK_ERRORS=1"
    CACHE STRING "enable extra checks for debugging" FORCE)

# first include the appropriate system-dependent file:
if(UNIX)
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
    set(PM_NEEDED_FRAMEWORKS ${COREAUDIO_LIB} ${COREFOUNDATION_LIB}
                             ${COREMIDI_LIB} ${CORESERVICES_LIB}
        CACHE INTERNAL "")

    set(JAVAVM_LIB "${FRAMEWORK_PATH}/JavaVM.framework")
    message(STATUS "SYSROOT: " ${CMAKE_OSX_SYSROOT})
    set(JNI_EXTRA_LIBS ${PM_NEEDED_FRAMEWORKS} ${JAVAVM_LIB})
    #set(JNI_EXTRA_LIBS CoreAudio.framework CoreFoundation.framework CoreMIDI.framework CoreServices.framework JavaVM.framework)
  endif(APPLE)
endif(UNIX)

# this completes the list of library sources by adding shared code
list(APPEND LIBSRC pmutil portmidi)

# now add the shared files to make the complete list of library sources
add_library(portmidi-static ${LIBSRC})
set_target_properties(portmidi-static PROPERTIES OUTPUT_NAME "portmidi")

add_library(portmidi SHARED ${LIBSRC})
target_link_libraries(portmidi ${PM_NEEDED_FRAMEWORKS})

# define the jni library
if(APPLE)
  include_directories("${JAVAVM_LIB}/Headers")
endif(APPLE)
set(JNISRC ${LIBSRC} ../pm_java/pmjni/pmjni.c)
add_library(pmjni SHARED ${JNISRC})
target_link_libraries(pmjni ${JNI_EXTRA_LIBS})
set_target_properties(pmjni PROPERTIES EXECUTABLE_EXTENSION "jnilib")


