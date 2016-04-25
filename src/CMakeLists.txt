# find_package (Boost REQUIRED)
# include_directories (${Boost_INCLUDE_DIRS})

add_library (LELA SHARED bloom.h  clause.h  compar.h  grounder.h  intmap.h  iter.h  literal.h  maybe.h  print.h setup.h  term.h)

set_target_properties (LELA PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries (LELA ${LIBS})

target_include_directories (LELA PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/lela>)

