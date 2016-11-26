find_package (Boost) 

if (Boost_FOUND)
    include_directories (${Boost_INCLUDE_DIRS}) 
    add_executable (ti ti.cc) 
    target_link_libraries (ti ${Boost_LIBRARIES})
    target_link_libraries (ti LINK_PUBLIC lela)
endif ()

