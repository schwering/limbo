add_executable (ti ti.cc) 
target_link_libraries (ti LINK_PUBLIC lela)

file (GLOB tests RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "test*.txt")
foreach (test ${tests})
    add_test (NAME ${test} COMMAND ./ti ${test})
endforeach ()

