# find_package (Boost REQUIRED)
# include_directories (${Boost_INCLUDE_DIRS})

add_library (LELA SHARED lela/bloom.h  lela/clause.h  lela/compar.h  lela/formula.h  lela/grounder.h  lela/intmap.h  lela/iter.h  lela/kb.h  lela/literal.h  lela/maybe.h  lela/pretty.h  lela/setup.h  lela/term.h)

set_target_properties (LELA PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries (LELA ${LIBS})

target_include_directories (LELA PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/lela>)

