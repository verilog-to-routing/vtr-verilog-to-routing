#Externally developed libraries
add_subdirectory(EXTERNAL)

# VTR developed libraries
#  Only add warn flags for VPR internal libraries.
add_compile_options(${WARN_FLAGS})
add_subdirectory(libarchfpga)
add_subdirectory(libvtrutil)
add_subdirectory(liblog)
add_subdirectory(libpugiutil)
add_subdirectory(libeasygl)
add_subdirectory(librtlnumber)
if(${VTR_ENABLE_CAPNPROTO})
    add_subdirectory(libvtrcapnproto)
endif()
