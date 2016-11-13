# CMakeLists files in this project can
# refer to the root source directory of the project as ${LELA_SOURCE_DIR} and
# to the root binary directory of the project as ${LELA_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (LELA)


#set (CMAKE_BUILD_TYPE Release)
set (CMAKE_BUILD_TYPE Debug)

#set (CMAKE_VERBOSE_MAKEFILE ON)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "(Clang|GNU)")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
endif()

# Debug configuration
if (CMAKE_BUILD_TYPE MATCHES Debug)
        #if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        #endif()

        #if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
                set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_LIBCPP_DEBUG")
        #endif()

        #if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_FLAGS MATCHES "-stdlib=libstdc\\+\\+")
                # defining also
                #  -D_GLIBCXX_CONCEPT_CHECKS
                # leads to compiler errors when using things like
                #   std::vector<std::unique_ptr<int>> asdf;
                set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC")
        #endif()
endif()

# Release configuration
if (CMAKE_BUILD_TYPE MATCHES Release)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
endif()

add_subdirectory (src)
#add_subdirectory (bats)
#add_subdirectory (examples)
add_subdirectory (tests)
#add_subdirectory (eclipse-clp)

enable_testing ()

