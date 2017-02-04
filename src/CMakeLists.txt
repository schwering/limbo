add_library (lela INTERFACE)

target_include_directories (lela INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include/lela>
)

