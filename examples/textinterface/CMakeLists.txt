add_executable (ti ti.cc) 
target_link_libraries (ti LINK_PUBLIC lela)


file (GLOB tests RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "test*.txt")
foreach (test ${tests})
    add_test (NAME ${test} COMMAND ./ti ${test})
endforeach ()


find_package (Emscripten)
if (EMCC)
        if (CMAKE_BUILD_TYPE MATCHES Release)
                add_custom_command (OUTPUT jquery-3.1.1.min.js
                        COMMAND wget -O "jquery-3.1.1.min.js" "https://code.jquery.com/jquery-3.1.1.min.js"
                        DEPENDS jquery-3.1.1.min.js)

                separate_arguments(FLAGS UNIX_COMMAND "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
                add_custom_command (OUTPUT ti-js.js ti-js.js.mem
                        COMMAND ${EMCC} ${FLAGS} -I../../src ti-js.cc -o ti-js.js -s EXPORTED_FUNCTIONS=['_lela_parse']
                        VERBATIM)

                add_custom_target (ti-js
                        DEPENDS ti-js.js ti-js.js.mem jquery-3.1.1.min.js)

                add_dependencies (ti ti-js)
        endif ()
endif ()

