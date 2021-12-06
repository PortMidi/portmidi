# CMake file to build tests in this directory: pm_test

# set the build directory to be in portmidi, not in portmidi/pm_test
# this is required for Xcode:
if(APPLE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif(APPLE)

if(WIN32)
include(../pm_win/static.cmake)
endif(WIN32)

macro(make_a_test name)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name} ${PM_LIB_NAME} ${PM_NEEDED_LIBS})
  add_dependencies(${name} ${PM_LIB_NAME})
endmacro(make_a_test)

make_a_test(testio)
make_a_test(midithread)
make_a_test(midithru)
make_a_test(sysex)
make_a_test(latency)
make_a_test(mm)
make_a_test(midiclock)
make_a_test(qtest)
make_a_test(fast) 
make_a_test(fastrcv)
if(WIN32)
# windows does not implement Pm_CreateVirtualInput or Pm_CreateVirtualOutput
else(WIN32)
make_a_test(recvvirtual) 
make_a_test(sendvirtual) 
make_a_test(multivirtual)  
make_a_test(virttest)  
endif(WIN32)
