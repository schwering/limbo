add_library (ESBL SHARED term.cc atom.cc literal.cc)

target_link_libraries (ESBL ${LIBS})

# Make sure the compiler can find include files for our ESBL library
# when other libraries or executables link to ESBL
target_include_directories (ESBL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

