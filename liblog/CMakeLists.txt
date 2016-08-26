project("liblog")

file(GLOB_RECURSE EXEC_SOURCES src/main.c)
file(GLOB_RECURSE LIB_SOURCES src/*.c)
file(GLOB_RECURSE LIB_HEADERS src/log.h)
get_include_directories(LIB_HEADERS LIB_INCLUDE_DIRS)

#Treat .c as CXX
set_source_files_properties(${LIB_SOURCES} PROPERTIES LANGUAGE CXX)

#Remove test executable from library
list(REMOVE_ITEM LIB_SOURCES ${EXEC_SOURCES})

#Create the library
add_library(liblog STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(liblog PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(liblog PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Create the test executable
add_executable(test_log ${EXEC_SOURCES})
target_link_libraries(test_log liblog)
