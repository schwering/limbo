add_executable (tui tui.cc linenoise/linenoise.c) 
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
                        COMMAND ${EMCC} ${FLAGS} -I../../src tui-js.cc -o tui-js.js
				-s EXPORTED_FUNCTIONS=['_lela_init','_lela_parse','_lela_free']
                                -s TOTAL_MEMORY=16777216
                        DEPENDS tui-js.cc "*.h"
                        VERBATIM)
                add_custom_command (OUTPUT jquery.terminal-1.0.10.min.js COMMAND cp jquery.terminal/js/jquery.terminal-1.0.10.min.js .)
                add_custom_command (OUTPUT jquery.mousewheel-min.js      COMMAND cp jquery.terminal/js/jquery.mousewheel-min.js      .)
                add_custom_command (OUTPUT unix_formatting.js            COMMAND cp jquery.terminal/js/unix_formatting.js            .)
                add_custom_command (OUTPUT jquery.terminal-1.0.10.css    COMMAND cp jquery.terminal/css/jquery.terminal-1.0.10.css   .)
                add_custom_target (tui-js DEPENDS tui-js.js tui-js.js.mem jquery.terminal-1.0.10.min.js jquery.mousewheel-min.js unix_formatting.js jquery.terminal-1.0.10.css)
                add_dependencies (tui tui-js)
        endif ()
endif ()

