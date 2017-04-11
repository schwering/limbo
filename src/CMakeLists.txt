add_library (limbo INTERFACE)

target_include_directories (limbo INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include/limbo>
)

