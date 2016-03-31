find_package (Boost REQUIRED)
include_directories (${Boost_INCLUDE_DIRS})

add_library (LELA SHARED term.cc setup.cc)

target_link_libraries (LELA ${LIBS})

# Make sure the compiler can find include files for our LELA library
# when other libraries or executables link to LELA
target_include_directories (LELA PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

