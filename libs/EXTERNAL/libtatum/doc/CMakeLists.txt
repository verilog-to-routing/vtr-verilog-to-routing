#find_package(Doxygen)

#if(DOXYGEN_FOUND)

    #set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in)
    #set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/doxyfile)

    #configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    #add_custom_target(doc
        #COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        #WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        #COMMENT "Generating API Documentation with Doxygen"
        #VERBATIM)

#else()
    #message(INFO "Doxygen not found. Documentation will not be built")
#endif()
