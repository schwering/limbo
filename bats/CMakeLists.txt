find_package (Prolog REQUIRED)

foreach (batname kr2014)
        add_library (BAT-${batname} SHARED ${batname}.cc)

        target_link_libraries (BAT-${batname} LINK_PUBLIC ESBL)

        add_custom_command(
                OUTPUT ${batname}.cc ${batname}.h
                COMMAND ${PROLOG_INTERPRETER} ${PROLOG_INTERPRETER_LOAD} proper-plus.pl ${PROLOG_INTERPRETER_QUERY} "\"compile_all(${batname}),halt.\""
                DEPENDS ${batname}.pl proper-plus.pl)

        target_include_directories (BAT-${batname} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()

