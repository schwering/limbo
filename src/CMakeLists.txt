add_library (lela SHARED lela/term.cc)

target_link_libraries (lela ${LIBS})

target_include_directories (lela PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include/lela>
)

