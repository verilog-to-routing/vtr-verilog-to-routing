# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

option(STATIC_LIBGCC_LIBSTDCPP "Link to libgcc and libstdc++ statically. If not set, requires libgcc and libstdc++ DLLs to be installed on the target machines. Note that static linking libgcc means exceptions will not propagate accross shared library boundaries." OFF) 

#Note that if libgcc and libstdcpp are NOT statically linked the appropriate
#DLL's should be installed on the target Windows machine, or distributed with
#the generated executables (e.g. in the same directory as the exectuable). 
#
#They are usually installed under:
#   /usr/lib/gcc/${COMPILER_PEFIX}/*-win32/libgcc*.dll
#   /usr/lib/gcc/${COMPILER_PEFIX}/*-win32/libstdc++*.dll
#
#For example targetting x86_64 with MingW based on gcc 5.3
#   /usr/lib/gcc/x86_64-w64-mingw32/5.3-win32/libstdc++*.dll
#   /usr/lib/gcc/x86_64-w64-mingw32/5.3-win32/libgcc*.dll

#
# Choose an appropriate compiler prefix
#
# for classical mingw32 (http://www.mingw.org/) set:
#       "i586-mingw32msvc"
#
# for 32 or 64 bit mingw-w64 (http://mingw-w64.sourceforge.net/) set:
#       "i686-w64-mingw32" or "x86_64-w64-mingw32"
set(COMPILER_PREFIX "x86_64-w64-mingw32" CACHE STRING "Default prefix for MinGW gcc/g++/windres")

set(MINGW_ROOT_PATH /usr/${COMPILER_PREFIX} CACHE STRING "MinGW path to target environment directory (containing include and lib)")
set(MINGW_RC ${COMPILER_PREFIX}-windres CACHE STRING "MinGW RC Compiler")
set(MINGW_CC ${COMPILER_PREFIX}-gcc CACHE STRING "MinGW C Compiler")
set(MINGW_CXX ${COMPILER_PREFIX}-g++ CACHE STRING "MinGW C++ Compiler")

# which compilers to use for C and C++
find_program(CMAKE_RC_COMPILER NAMES ${MINGW_RC})
find_program(CMAKE_C_COMPILER NAMES ${MINGW_CC})
find_program(CMAKE_CXX_COMPILER NAMES ${MINGW_CXX})

#
# Set the target environment location
#
SET(CMAKE_FIND_ROOT_PATH ${MINGW_ROOT_PATH})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#The mingw-w64 headers and ABC's pthread.h have conflicting definitions of struct timespec.
#Adding this define suppresses the duplicate definition in ABC's pthread.h
add_definitions(-DHAVE_STRUCT_TIMESPEC=1)

#This forces ABC to use the stdint types (e.g. ptrdiff_t) instead of its platform
#dependant type look-up. This avoids 'unkown platform' compilation errors.
add_definitions(-DABC_USE_STDINT_H=1)

if (STATIC_LIBGCC_LIBSTDCPP)
    #Statically link to libgcc and libstdc++ to avoid DLL dependencies
    link_libraries(-static-libgcc -static-libstdc++)
endif()
