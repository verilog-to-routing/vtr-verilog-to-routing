cmake_minimum_required(VERSION 2.8.12)

if (${CMAKE_VERSION} VERSION_GREATER "3.8")
    #For cmake >= 3.9 INTERPROCEDURAL_OPTIMIZATION behaviour we need to explicitly
    #set the cmake policy version number
    cmake_policy(VERSION 3.9) 

    # If we are using verison < 3.9 then setting INTERPROCEDURAL_OPTIMIZATION
    # has no effect unless an Intel compiler is used
endif()

project("vpr")

option(VPR_USE_SIGNAL_HANDLER "Should VPR use a signal handler to intercept signals (e.g. SIGINT)?" OFF)
option(VPR_IPO_BUILD "Should VPR be compiled with an interprocedural optimizations?" ON)

include(CheckCXXSymbolExists)

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES src/main.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Create the library
add_library(libvpr STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libvpr PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libvpr PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libvpr
                        libvtrutil
                        libarchfpga
                        libsdcparse
                        libblifparse
                        libeasygl
                        libtatum
                        libargparse
                        libpugixml)

#Create the executable
add_executable(vpr ${EXEC_SOURCES})
target_link_libraries(vpr 
                        libvpr)

if (VPR_USE_IPO_BUILD)
    message(STATUS "Using IPO build for vpr")
    set_property(TARGET libvpr PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET vpr PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (VPR_USE_SIGNAL_HANDLER)
    #Check wheter VPR can use sigaction to handle signals (only supported by POSIX)
    CHECK_CXX_SYMBOL_EXISTS(sigaction csignal HAVE_SIGACTION)
    if(HAVE_SIGACTION)
        target_compile_definitions(libvpr PRIVATE VPR_USE_SIGACTION)    
    endif()
endif()

install(TARGETS vpr libvpr DESTINATION bin)
