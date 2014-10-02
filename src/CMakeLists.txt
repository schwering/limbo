# Create a library called "ESBL" which includes the source file "hello.cxx".
# The extension is already found. Any number of sources could be listed here.
add_library (ESBL SHARED belief.c literal.c map.c query.c set.c setup.c term.c vector.c)

find_package (GC REQUIRED)
include_directories (${BOEHM_GC_INCLUDE_DIRS})
set (LIBS ${LIBS} ${BOEHM_GC_LIBRARIES})
target_link_libraries (ESBL ${LIBS})

# Make sure the compiler can find include files for our ESBL library
# when other libraries or executables link to ESBL
target_include_directories (ESBL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

