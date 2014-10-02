add_executable (Example-KR2014 kr2014.c)
add_executable (Example-ECAI2014 ecai2014.c)

# Link the executable to the ESBL library. Since the ESBL library has
# public include directories we will use those link directories when building
# kr2014 and ecai2014.
target_link_libraries (Example-KR2014 LINK_PUBLIC ESBL BAT-KR2014)
target_link_libraries (Example-ECAI2014 LINK_PUBLIC ESBL BAT-ECAI2014)

