if(zbar_FOUND)
    return()
endif()

find_library(ZBAR_LIB zbar
             PATH_SUFFIXES lib64 lib
)

find_path(ZBAR_INCLUDE_DIR
          NAMES zbar.h
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zbar
    REQUIRED_VARS ZBAR_LIB ZBAR_INCLUDE_DIR
)

mark_as_advanced(ZBAR_FOUND ZBAR_LIB ZBAR_INCLUDE_DIR)

add_library(zbar::zbar INTERFACE IMPORTED)
set_target_properties(zbar::zbar PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${ZBAR_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "${ZBAR_LIB}"
)
