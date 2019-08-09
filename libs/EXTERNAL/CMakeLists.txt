#Manually synchronized external libraries
add_subdirectory(libpugixml)
add_subdirectory(libcatch)

#External libraries synchronized with 'git subtree'
add_subdirectory(libargparse)
add_subdirectory(libsdcparse)
add_subdirectory(libblifparse)
add_subdirectory(libtatum)
if(VPR_USE_EZGL STREQUAL "on")
message(STATUS "including libezgl!!!!!!!!")
add_subdirectory(libezgl)
else()
message(STATUS "did not include libezgl!!!")
endif()
