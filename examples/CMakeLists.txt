#add_executable (Example-KR2014 kr2014.cc)
#add_executable (Example-ECAI2014 ecai2014.cc)
add_executable (Minesweeper minesweeper.cc)

# Link the executable to the LELA library. Since the LELA library has
# public include directories we will use those link directories when building
# kr2014 and ecai2014.
#target_link_libraries (Example-KR2014 LINK_PUBLIC LELA BATS)
#target_link_libraries (Example-ECAI2014 LINK_PUBLIC LELA BAT-ecai2014)
target_link_libraries (Minesweeper LINK_PUBLIC lela)

