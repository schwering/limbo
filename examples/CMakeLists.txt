add_executable (kr-2014 kr-2014.c)
add_executable (ecai-2014 ecai-2014.c)

# Link the executable to the LibESBL library. Since the Hello library has
# public include directories we will use those link directories when building
# kr-2014 and ecai-2014.
target_link_libraries (kr-2014 LINK_PUBLIC LibESBL)
target_link_libraries (ecai-2014 LINK_PUBLIC LibESBL)

