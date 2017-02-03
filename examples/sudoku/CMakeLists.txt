add_executable (sudoku sudoku.cc)
target_link_libraries (sudoku LINK_PUBLIC lela)


# add_test (NAME sudokus COMMAND ./test-sudokus.sh)


find_package (Emscripten)
if (EMCC)
        if (CMAKE_BUILD_TYPE MATCHES Release)
                add_custom_command (OUTPUT jquery-3.1.1.min.js
                        COMMAND wget -O "jquery-3.1.1.min.js" "https://code.jquery.com/jquery-3.1.1.min.js"
                        DEPENDS jquery-3.1.1.min.js)

                separate_arguments(FLAGS UNIX_COMMAND "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
                add_custom_command (OUTPUT sudoku-js.js sudoku-js.js.mem
                    COMMAND ${EMCC} ${FLAGS} -I../../src sudoku-js.cc -o sudoku-js.js -s EXPORTED_FUNCTIONS=['_lela_init','_lela_play_turn']
                        VERBATIM)

                add_custom_target (sudoku-js
                        DEPENDS sudoku-js.js sudoku-js.js.mem jquery-3.1.1.min.js)

                add_dependencies (sudoku sudoku-js)
        endif ()
endif ()


