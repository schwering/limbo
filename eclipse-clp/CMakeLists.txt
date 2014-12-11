add_library (EclipseLELA SHARED ec_interface.cc)

find_package (ECLiPSe-CLP REQUIRED)
include_directories (${ECLIPSE_CLP_INCLUDE_DIR})
target_link_libraries (EclipseLELA LINK_PUBLIC LELA BATS)

