function(files_to_dirs file_list directory_list)
    #Extract the directories for each header file
    foreach(file ${${file_list}})
        get_filename_component(dir ${file} DIRECTORY)
        list(APPEND dir_list ${dir})
    endforeach()

    #Remove any duplicates
    list(LENGTH "${dir_list}" length)
    if(${length} GREATER 1) #Avoid error with zero-length lists
        list(REMOVE_DUPLICATES ${dir_list})
    endif()

    #Set the second argument in the caller's scope
    set(${directory_list} ${dir_list} PARENT_SCOPE)
endfunction(files_to_dirs)
