#
# Compiler flags come from parent
#
option(TATUM_STA_ENABLE_VTUNE_PROFILE "Enable selective sampling in VTUNE" OFF)
option(TATUM_STA_ENABLE_CALLGRIND_PROFILE "Enable selective sampling with callgrind" OFF)

#
#
# Build files configuration
#
#

#Demo executable
#set(EXEC_NAME tatum_sta)

#Source files for the demo executable (not generated)
file(GLOB_RECURSE TATUM_STA_SOURCES *.cpp)
file(GLOB_RECURSE TATUM_STA_HEADERS *.hpp)

set(LEXER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/parsers/vpr_timing_graph.l)
set(PARSER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/parsers/vpr_timing_graph.y)

#Include directories
foreach(header ${TATUM_STA_HEADERS})
    get_filename_component(incl_dir ${header} DIRECTORY)
    list(APPEND TATUM_STA_INCLUDE_DIRS ${incl_dir})
endforeach()
#Remove duplicate include directories
list(REMOVE_DUPLICATES TATUM_STA_INCLUDE_DIRS)


#
# Configure intermediate files
#

#
#
# Define the actual build targets
#
#

#Define Executable
add_executable(tatum_sta
               ${TATUM_STA_SOURCES}
               ${TATUM_STA_HEADERS})

#Exectuable Includes
target_include_directories(tatum_sta PRIVATE
                           ${TATUM_STA_INCLUDE_DIRS})

#Executable links to the library
target_link_libraries(tatum_sta libtatum libtatumparse)

if(TATUM_STA_ENABLE_VTUNE_PROFILE)
    target_include_directories(tatum_sta PRIVATE /opt/intel/vtune_amplifier_xe/include)
    target_link_libraries(tatum_sta /opt/intel/vtune_amplifier_xe/lib64/libittnotify.a ${CMAKE_DL_LIBS})
    target_compile_definitions(tatum_sta PRIVATE TATUM_STA_PROFILE_VTUNE=1)
    target_compile_options(tatum_sta PUBLIC -g)
endif()

if(TATUM_STA_ENABLE_CALLGRIND_PROFILE)
    
    #To selectively profile using callgrind:
    #  valgrind --tool=callgrind --collect-atstart=no --instr-atstart=no --cache-sim=yes --cacheuse=yes ./command

    target_compile_definitions(tatum_sta PRIVATE TATUM_STA_PROFILE_CALLGRIND=1)
    target_compile_options(tatum_sta PUBLIC -g)
endif()
