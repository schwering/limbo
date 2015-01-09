# CMakeLists files in this project can
# refer to the root source directory of the project as ${LELA_SOURCE_DIR} and
# to the root binary directory of the project as ${LELA_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (LELA)

set (CMAKE_BUILD_TYPE Release)
#set (CMAKE_BUILD_TYPE Debug)
#set (CMAKE_C_FLAGS "-std=gnu99")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
endif()

if (CMAKE_BUILD_TYPE MATCHES Debug)
	if (CMAKE_COMPILER_IS_GNUCXX)
		# defining also
		#  -D_GLIBCXX_CONCEPT_CHECKS
		# leads to compiler errors when using things like
		#   std::vector<std::unique_ptr<int>> asdf;
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC")
	endif()
endif()

add_subdirectory (src)
add_subdirectory (bats)
add_subdirectory (examples)
add_subdirectory (tests)
add_subdirectory (eclipse-clp)

enable_testing ()

