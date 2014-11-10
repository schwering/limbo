add_library (EclipseESBL SHARED ec_interface.cc)

find_package (ECLiPSe-CLP REQUIRED)
include_directories (${ECLIPSE_CLP_INCLUDE_DIR})
target_link_libraries (EclipseESBL LINK_PUBLIC ESBL BAT-kr2014 BAT-ecai2014)

