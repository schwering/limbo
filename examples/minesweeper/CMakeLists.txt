add_executable (mw mw.cc)
target_link_libraries (mw LINK_PUBLIC lela)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_DETERMINES")

find_package (Emscripten)
if (EMCC)
        if (CMAKE_BUILD_TYPE MATCHES Release)
                add_custom_command (OUTPUT jquery-3.1.1.min.js
                        COMMAND wget -O "jquery-3.1.1.min.js" "https://code.jquery.com/jquery-3.1.1.min.js"
                        DEPENDS jquery-3.1.1.min.js)

                separate_arguments(FLAGS UNIX_COMMAND "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
                add_custom_command (OUTPUT mw-js.js mw-js.js.mem
                        COMMAND ${EMCC} ${FLAGS} -I../../src mw-js.cc -o mw-js.js
                                -s EXPORTED_FUNCTIONS=['_lela_init','_lela_play_turn']
                                -s TOTAL_MEMORY=67108864
                        VERBATIM)

                add_custom_target (mw-js
                        DEPENDS mw-js.js mw-js.js.mem jquery-3.1.1.min.js)

                add_dependencies (mw mw-js)
        endif ()
endif ()


