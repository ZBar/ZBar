cmake_minimum_required(VERSION 3.10)
project(ZBarQRCodeTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

conan_basic_setup(KEEP_RPATHS TARGETS)

add_executable(testApp main.cpp)

target_link_libraries(testApp
    PRIVATE
       CONAN_PKG::zbar
)

