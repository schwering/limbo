#set (Boost_USE_STATIC_LIBS OFF) 
#set (Boost_USE_MULTITHREADED OFF)  
#set (Boost_USE_STATIC_RUNTIME OFF) 
#find_package (Boost COMPONENTS spirit) 
find_package (Boost) 

if (Boost_FOUND)
    include_directories (${Boost_INCLUDE_DIRS}) 
    add_executable (cli cli.cc) 
    target_link_libraries (cli ${Boost_LIBRARIES})
    target_link_libraries (cli LINK_PUBLIC lela)
endif ()

