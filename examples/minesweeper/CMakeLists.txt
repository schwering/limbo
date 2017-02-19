add_executable (minesweeper minesweeper.cc)
target_link_libraries (minesweeper LINK_PUBLIC lela)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_DETERMINES")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEND_GAME_CLAUSES")

find_package (Emscripten)
if (EMCC)
        if (CMAKE_BUILD_TYPE MATCHES Release)
                separate_arguments(FLAGS UNIX_COMMAND "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
                add_custom_command (OUTPUT minesweeper-js.js minesweeper-js.js.mem
                        COMMAND ${EMCC} ${FLAGS} -I../../src minesweeper-js.cc -o minesweeper-js.js
                                -s EXPORTED_FUNCTIONS=['_lela_init','_lela_play_turn']
                                -s TOTAL_MEMORY=67108864
                        VERBATIM)

                add_custom_target (minesweeper-js DEPENDS minesweeper-js.js minesweeper-js.js.mem)
                add_dependencies (minesweeper minesweeper-js)
        endif ()
endif ()


