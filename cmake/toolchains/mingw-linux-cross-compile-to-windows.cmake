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

# for classical mingw32
# see http://www.mingw.org/
#set(COMPILER_PREFIX "i586-mingw32msvc")

# for 32 or 64 bits mingw-w64
# see http://mingw-w64.sourceforge.net/
#set(COMPILER_PREFIX "i686-w64-mingw32") #32-bit
set(COMPILER_PREFIX "x86_64-w64-mingw32") #64-bit

# which compilers to use for C and C++
find_program(CMAKE_RC_COMPILER NAMES ${COMPILER_PREFIX}-windres)
find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)
#SET(CMAKE_RC_COMPILER ${COMPILER_PREFIX}-windres)
#SET(CMAKE_C_COMPILER ${COMPILER_PREFIX}-gcc)
#SET(CMAKE_CXX_COMPILER ${COMPILER_PREFIX}-g++)


#
# Set the target environment location
#
SET(USER_ROOT_PATH )
SET(CMAKE_FIND_ROOT_PATH  /usr/${COMPILER_PREFIX} ${USER_ROOT_PATH})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#The current version of ABC does some unsafe casting 
#(e.g. 64-bit pointer to 32-bit unsigned), since it assumes
#an LP64 OS, but Windows is LLP64.
#For now, just turn on permissive mode to let things compile
#TODO: these issues are fixed in the latest ABC, so this option should be removed when ABC is upgraded
add_compile_options(-fpermissive)

if (STATIC_LIBGCC_LIBSTDCPP)
    #Statically link to libgcc and libstdc++ to avoid DLL dependencies
    link_libraries(-static-libgcc -static-libstdc++)
endif()
