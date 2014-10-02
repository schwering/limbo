# Create a library called "ESBL" which includes the source file "hello.cxx".
# The extension is already found. Any number of sources could be listed here.
add_library (BAT-KR2014 SHARED kr2014.c common.c)
add_library (BAT-ECAI2014 SHARED ecai2014.c common.c)

target_link_libraries (BAT-KR2014 LINK_PUBLIC ESBL)
target_link_libraries (BAT-ECAI2014 LINK_PUBLIC ESBL)

add_custom_command(
	OUTPUT kr2014.c kr2014.h
	COMMAND swipl -f proper-plus.pl -g "\"compile_all(kr2014),halt.\""
	DEPENDS kr2014.pl proper-plus.pl)
add_custom_command(
	OUTPUT ecai2014.c ecai2014.h
	COMMAND swipl -f proper-plus.pl -g "\"compile_all(ecai2014),halt.\""
	DEPENDS ecai2014.pl proper-plus.pl)

target_include_directories (BAT-KR2014 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (BAT-ECAI2014 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

