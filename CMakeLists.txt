project(zbar_qrcode C)
cmake_minimum_required(VERSION 3.10)

set(LIBRARY_NAME zbar)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

include_directories(
    include
    zbar
    zbar/qrcode
    zbar/decoder
)

set(SOURCE_FILES
    zbar/decoder/qr_finder.c
    zbar/qrcode/qrdec.c
    zbar/qrcode/qrdectxt.c
    zbar/qrcode/rs.c
    zbar/qrcode/isaac.c
    zbar/qrcode/bch15_5.c
    zbar/qrcode/binarize.c
    zbar/qrcode/util.c
)

set(HEADER_FILES
    include/zbar.h
    zbar/decoder/qr_finder.h
    zbar/qrcode.h
    zbar/qrcode/qrdec.h
    zbar/qrcode/rs.h
    zbar/qrcode/isaac.h
    zbar/qrcode/bch15_5.h
    zbar/qrcode/binarize.h
    zbar/qrcode/util.h
)

option(BUILD_SHARED_LIBS "Build shared library" ON)

add_definitions(-DENABLE_QRCODE)

FIND_FILE(HAVE_INTTYPES_H "inttypes.h" DOC "inttypes.h exists")
if (HAVE_INTTYPES_H)
    add_definitions(-DHAVE_INTTYPES_H=1)
endif()

FIND_FILE(HAVE_FEATURES_H "features.h" DOC "features.h exists")
if (HAVE_FEATURES_H)
    add_definitions(-DHAVE_FEATURES_H=1)
endif()

add_library(${LIBRARY_NAME} ${SOURCE_FILES} ${HEADER_FILES})

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads QUIET)
if(THREADS_FOUND)
    add_definitions(-DHAVE_LIBPTHREAD=1)
    target_link_libraries(
        ${LIBRARY_NAME} 
        PRIVATE
            Threads::Threads
    )
endif()


include(GenerateExportHeader)
generate_export_header(${LIBRARY_NAME} EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/zbar_export.h)
target_include_directories(${LIBRARY_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
)

install(TARGETS ${LIBRARY_NAME}
    RUNTIME  DESTINATION "bin"
    LIBRARY  DESTINATION "lib"
    ARCHIVE  DESTINATION "lib"
)
