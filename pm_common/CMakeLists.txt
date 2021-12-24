# pm_common/CMakeLists.txt -- how to build portmidi library

# Support alternative name for static libraries to avoid confusion.
# (In particular, Xcode has automatically converted portmidi.a to
# portmidi.dylib without warning, so using portmidi-static.a eliminates
# this possibility, but default for all libs is "portmidi"):
set(PM_STATIC_LIB_NAME "portmidi" CACHE STRING 
    "For static builds, the PortMidi library name, e.g. portmidi-static.
     Default is portmidi")
set(PM_ACTUAL_LIB_NAME "portmidi" PARENT_SCOPE) # shared with pm_tests
if(NOT BUILD_SHARED_LIBS)
  set(PM_ACTUAL_LIB_NAME ${PM_STATIC_LIB_NAME})
endif()

# we need full paths to sources because they are shared with other targets
# (in particular pmjni). Set PMDIR to the top-level portmidi directory:
get_filename_component(PMDIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(PM_LIB_PUBLIC_SRC ${PMDIR}/pm_common/portmidi.c
                      ${PMDIR}/pm_common/pmutil.c
                      ${PMDIR}/porttime/porttime.c)
add_library(portmidi ${PM_LIB_PUBLIC_SRC})
set_target_properties(portmidi PROPERTIES
                      OUTPUT_NAME "${PM_ACTUAL_LIB_NAME}"
                      WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
target_include_directories(portmidi PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pm_common>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)


# set the build directory for libportmidi.a to be in portmidi, not in 
#    portmidi/pm_common
if(APPLE OR WIN32)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  # set the build directory for .dylib libraries
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif(APPLE OR WIN32)

option(PM_CHECK_ERRORS
"Insert a check for error return values at the end of each PortMidi function.
If an error is encountered, a text message is printed using printf(), the user
is asked to type ENTER, and then exit(-1) is called to clean up and terminate
the program.

You should not use PM_CHECK_ERRORS if printf() does not work (e.g. this is not
a console application under Windows, or there is no visible console on some
other OS), and you should not use PM_CHECK_ERRORS if you intend to recover
from errors rather than abruptly terminate the program." OFF)
if(PM_CHECK_ERRORS)
  target_compile_definitions(portmidi PRIVATE PM_CHECK_ERRORS)
endif(PM_CHECK_ERRORS)


if(APPLE)
#  set(PM_OSX_VERSION "10.7" CACHE STRING
#      "selects PM_OSX_SDK and macosx-version-min C flag")
  set(PM_OSX_SDK "/Developer/SDKs/MacOSX${PM_OSX_VERSION}.sdk")
  set(CMAKE_OSX_SYSROOT ${PM_OSC_SDK} CACHE 
      PATH "-isysroot parameter for compiler")
  set(CMAKE_C_FLAGS "-mmacosx-version-min=${PM_OSC_VERSION}" CACHE 
      STRING "needed in conjunction with CMAKE_OSX_SYSROOT" FORCE)
  # option(OSX_RPATH "control CMake policy for finding macOS dynamic libraries"
  #        ON)  # enabled here to match CMake default. Is this correct?
endif()

macro(prepend_path RESULT PATH)
  set(${RESULT})
  foreach(FILE ${ARGN})
    list(APPEND ${RESULT} "${PATH}${FILE}")
  endforeach(FILE)
endmacro(prepend_path)

# first include the appropriate system-dependent file:
if(UNIX AND APPLE)
  find_library(COREAUDIO_LIBRARY CoreAudio REQUIRED)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
  find_library(COREMIDI_LIBRARY CoreMidi REQUIRED)
  find_library(CORESERVICES_LIBRARY CoreServices REQUIRED)
  set(PM_LIB_PRIVATE_SRC 
      ${PMDIR}/porttime/ptmacosx_mach.c
      ${PMDIR}/pm_mac/pmmac.c
      ${PMDIR}/pm_mac/pmmacosxcm.c
      ${PMDIR}/pm_mac/finddefault.c
      ${PMDIR}/pm_mac/readbinaryplist.c)
  target_link_libraries(portmidi PRIVATE pthread ${COREAUDIO_LIBRARY}
      ${COREFOUNDATION_LIBRARY} ${COREMIDI_LIBRARY} ${CORESERVICES_LIBRARY})
  # set to CMake default; is this right?:
  set_target_properties(portmidi PROPERTIES MACOSX_RPATH ON)
elseif(UNIX)
  target_compile_definitions(portmidi PRIVATE ${LINUX_FLAGS})
  set(PM_LIB_PRIVATE_SRC
      ${PMDIR}/porttime/ptlinux.c
      ${PMDIR}/pm_linux/pmlinux.c
      ${PMDIR}/pm_linux/pmlinuxnull.c
      ${PMDIR}/pm_linux/finddefault.c)
  if(${LINUX_DEFINES} MATCHES ".*PMALSA.*")
    # Note that ALSA is not required if PMNULL is defined -- PortMidi will then
    # compile without ALSA and report no MIDI devices. Later, PMSNDIO or PMJACK
    # might be additional options.
    find_package(ALSA REQUIRED)
    list(APPEND PM_LIB_PRIVATE_SRC ${PMDIR}/pm_linux/pmlinuxalsa.c)
    target_link_libraries(portmidi PRIVATE pthread ALSA::ALSA)
  else()
    message(WARNING "No PMALSA, so PortMidi will not use ALSA, "
                    "and will not find or open MIDI devices.")
  endif()
elseif(WIN32)
  set(PM_LIB_PRIVATE_SRC
      ${PMDIR}/porttime/ptwinmm.c
      ${PMDIR}/pm_windows/pmwin.c
      ${PMDIR}/pm_windows/pmwinmm.c)
  target_link_libraries(portmidi PRIVATE winmm)
  if(NOT BUILD_SHARED_LIBS)
    # /MDd is multithread debug DLL, /MTd is multithread debug
    # /MD is multithread DLL, /MT is multithread. Change to static:
    include(../pm_win/static.cmake)
  endif(BUILD_SHARED_LIBS)
else()
  message(FATAL_ERROR "Operating system not supported.")
endif()

set(PM_LIB_PUBLIC_SRC ${PM_LIB_PUBLIC_SRC} PARENT_SCOPE) # export to parent
set(PM_LIB_PRIVATE_SRC ${PM_LIB_PRIVATE_SRC} PARENT_SCOPE) # export to parent

target_sources(portmidi PRIVATE ${PM_LIB_PRIVATE_SRC})
