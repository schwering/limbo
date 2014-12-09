find_package (Prolog REQUIRED)

add_library (BATS SHARED kr2014.cc ecai2014.cc kitchen.cc testbat.cc)
target_link_libraries (BATS LINK_PUBLIC ESBL)
target_include_directories (BATS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

foreach (batname kr2014 ecai2014 kitchen testbat)
        add_custom_command(
                OUTPUT ${batname}.cc ${batname}.h
                COMMAND ${PROLOG_INTERPRETER} ${PROLOG_INTERPRETER_LOAD} properplus.pl ${PROLOG_INTERPRETER_QUERY} "\"compile_all(${batname}),halt.\""
                DEPENDS ${batname}.pl properplus.pl)
endforeach()

