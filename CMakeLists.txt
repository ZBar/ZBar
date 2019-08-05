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
FIND_FILE(HAVE_POLL_H "poll.h" DOC "poll.h exists")

set(SOURCE_FILES
    zbar/decoder/qr_finder.c
    zbar/qrcode/qrdec.c
    zbar/qrcode/qrdectxt.c
    zbar/qrcode/rs.c
    zbar/qrcode/isaac.c
    zbar/qrcode/bch15_5.c
    zbar/qrcode/binarize.c
    zbar/qrcode/util.c
    zbar/processor/posix.c
    zbar/symbol.c
    zbar/image.c
    zbar/video.c
    zbar/img_scanner.c
    zbar/scanner.c
    zbar/window.c
    zbar/decoder.c
    zbar/refcnt.c
    zbar/processor/lock.c
    zbar/processor.c
    zbar/convert.c
    zbar/error.c
    zbar/video/null.c
    zbar/processor/null.c
    zbar/window/null.c
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
    zbar/error.h
    zbar/symbol.h
    zbar/image.h
    zbar/processor.h
    zbar/refcnt.h
    zbar/timer.h
    zbar/mutex.h
    zbar/event.h
    zbar/thread.h
    zbar/window.h
    zbar/video.h
    zbar/img_scanner.h
    zbar/decoder.h
    zbar/processor/posix.h
)

add_library(zbar ${SOURCE_FILES} ${HEADER_FILES})
target_compile_definitions(zbar
    PRIVATE
        ENABLE_QRCODE
        ZBAR_VERSION_MAJOR=0
        ZBAR_VERSION_MINOR=1
)

if (HAVE_INTTYPES_H)
    target_compile_definitions(zbar PRIVATE HAVE_INTTYPES_H=1)
endif()
if (HAVE_FEATURES_H)
    target_compile_definitions(zbar PRIVATE HAVE_FEATURES_H=1)
endif()
if (HAVE_POLL_H)
    target_compile_definitions(zbar PRIVATE HAVE_POLL_H=1)
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
    DESTINATION include/zbar
)
install(
    FILES Findzbar.cmake
    DESTINATION share/cmake
)
