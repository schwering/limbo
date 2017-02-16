add_executable (tui tui.cc) 
target_link_libraries (tui LINK_PUBLIC lela)


file (GLOB tests RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.lela")
foreach (test ${tests})
    add_test (NAME ${test} COMMAND ./tui ${test})
endforeach ()


find_package (Emscripten)
if (EMCC)
        if (CMAKE_BUILD_TYPE MATCHES Release)
                separate_arguments(FLAGS UNIX_COMMAND "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
                add_custom_command (OUTPUT tui-js.js tui-js.js.mem
                        COMMAND ${EMCC} ${FLAGS} -I../../src tui-js.cc -o tui-js.js -s EXPORTED_FUNCTIONS=['_lela_parse']
                                -s TOTAL_MEMORY=16777216
                        VERBATIM)

                add_custom_target (tui-js DEPENDS tui-js.js tui-js.js.mem)
                add_dependencies (tui tui-js)
        endif ()
endif ()

