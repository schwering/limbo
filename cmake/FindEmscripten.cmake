FIND_PROGRAM (EMCC em++)

IF (EMCC)
    MESSAGE(STATUS "Searching Emscripten - found")
    #MESSAGE(STATUS ${EMCC})
ELSE()
    MESSAGE(STATUS "Searching Emscripten - not found")
ENDIF()
 

