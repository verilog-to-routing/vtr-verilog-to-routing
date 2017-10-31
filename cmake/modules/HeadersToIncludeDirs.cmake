function(headers_to_include_dirs header_file_list include_directory_list)
    #Extract the directories for each header file
    foreach(header ${${header_file_list}})
        get_filename_component(incl_dir ${header} DIRECTORY)
        list(APPEND dir_list ${incl_dir})
    endforeach()

    #Remove any duplicates
    list(LENGTH "${dir_list}" length)
    if(${length} GREATER 1) #Avoid error with zero-length lists
        list(REMOVE_DUPLICATES ${dir_list})
    endif()

    #Set the second argument in the caller's scope
    set(${include_directory_list} ${dir_list} PARENT_SCOPE)
endfunction(headers_to_include_dirs)
