add_executable (KR-2014 kr-2014.c)
add_executable (ECAI-2014 ecai-2014.c)

# Link the executable to the ESBL library. Since the ESBL library has
# public include directories we will use those link directories when building
# kr-2014 and ecai-2014.
target_link_libraries (KR-2014 LINK_PUBLIC ESBL)
target_link_libraries (ECAI-2014 LINK_PUBLIC ESBL)

