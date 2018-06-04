cmake_minimum_required (VERSION 2.8.11)
project (limbo)

include (CTest)

#set (CMAKE_BUILD_TYPE Release)
set (CMAKE_BUILD_TYPE Debug)

set (CMAKE_VERBOSE_MAKEFILE ON)

# Good for YCM-Generator for the Vim YouCompleteMe plugin
set (CMAKE_EXPORT_COMPILE_COMMANDS 1)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBLOOM")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -march=native")
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    #set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "(Clang|GNU)")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
endif ()

# Debug configuration
if (CMAKE_BUILD_TYPE MATCHES Debug)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_LIBCPP_DEBUG")
    # defining also
    #  -D_GLIBCXX_CONCEPT_CHECKS
    # leads to compiler errors when using things like
    #   std::vector<std::unique_ptr<int>> asdf;
    # because of missing copy assignment operators
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC")
endif ()

# Release configuration
if (CMAKE_BUILD_TYPE MATCHES Release)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif ()

add_subdirectory (src)
add_subdirectory (tests)
add_subdirectory (examples)

