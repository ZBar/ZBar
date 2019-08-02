project(zbar_qrcode)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

option(BUILD_SHARED_LIBS "Build shared library" ON)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads QUIET)

FIND_FILE(HAVE_INTTYPES_H "inttypes.h" DOC "inttypes.h exists")
FIND_FILE(HAVE_FEATURES_H "features.h" DOC "features.h exists")

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

add_library(zbar ${SOURCE_FILES} ${HEADER_FILES})
target_compile_definitions(zbar PRIVATE ENABLE_QRCODE)
if (HAVE_INTTYPES_H)
    target_compile_definitions(zbar PRIVATE HAVE_INTTYPES_H=1)
endif()
if (HAVE_FEATURES_H)
    target_compile_definitions(zbar PRIVATE HAVE_FEATURES_H=1)
endif()
if(THREADS_FOUND)
    target_compile_definitions(zbar PRIVATE HAVE_LIBPTHREAD=1)
    target_link_libraries(zbar
        PRIVATE
            Threads::Threads
    )
endif()

target_include_directories(zbar
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    PRIVATE
        zbar
        zbar/qrcode
        zbar/decoder
)

install(TARGETS zbar
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(
    FILES include/zbar.h
    DESTINATION include
)
install(
    DIRECTORY include/zbar
    DESTINATION include
)
