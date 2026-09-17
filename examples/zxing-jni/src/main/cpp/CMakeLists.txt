cmake_minimum_required(VERSION 3.21)
project(ZXingJni LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CMAKE_PREFIX_PATH muss auf den Installationsprefix von zxing-cpp zeigen.
find_package(JNI REQUIRED)
find_package(ZXing CONFIG REQUIRED)

add_library(ZXingJni SHARED ZXingJni.cpp)
target_link_libraries(ZXingJni PRIVATE JNI::JNI ZXing::ZXing)

add_executable(ZXingJpegCli ZXingJpegCli.cpp)
target_link_libraries(ZXingJpegCli PRIVATE ZXing::ZXing windowscodecs ole32)
