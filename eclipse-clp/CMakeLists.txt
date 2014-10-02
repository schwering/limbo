add_library (EclipseESBL SHARED ec_interface.c)

# doesn't work, why?
find_package (ECLiPSe REQUIRED)
include_directories (${ECLIPSE_INCLUDE_DIRS})
target_link_libraries (EclipseESBL LINK_PUBLIC ESBL ${ECLIPSE_INCLUDE_DIRS})

