add_library (ESBL SHARED term.c literal.c vector.c bitmap.c set.c map.c ewff.c setup.c belief.c query.c)

find_package (GC REQUIRED)
include_directories (${BOEHM_GC_INCLUDE_DIRS})
set (LIBS ${LIBS} ${BOEHM_GC_LIBRARIES})
target_link_libraries (ESBL ${LIBS})

# Make sure the compiler can find include files for our ESBL library
# when other libraries or executables link to ESBL
target_include_directories (ESBL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

