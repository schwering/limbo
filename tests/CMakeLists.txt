#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

find_package (Threads)
find_package (Check REQUIRED)
include_directories (${CHECK_INCLUDE_DIRS})

add_executable (check_vector check_vector.c)
add_executable (check_set check_set.c)
add_executable (check_map check_map.c)
add_executable (check_literal check_literal.c)
add_executable (check_setup check_setup.c)
add_executable (check_query check_query.c)
add_executable (check_belief check_belief.c)

target_link_libraries (check_vector LINK_PUBLIC ESBL ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (check_set LINK_PUBLIC ESBL ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (check_map LINK_PUBLIC ESBL ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (check_literal LINK_PUBLIC ESBL ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (check_setup LINK_PUBLIC ESBL ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (check_query LINK_PUBLIC ESBL ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (check_belief LINK_PUBLIC ESBL ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

