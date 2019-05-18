add_executable (sat sat.cc)
target_link_libraries (sat LINK_PUBLIC limbo)

add_executable (limsat limsat.cc)
target_link_libraries (limsat LINK_PUBLIC limbo)

