cmake_minimum_required(VERSION 2.8)
project( Stereo )

find_package( OpenCV REQUIRED )

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories( ${OpenCV_INCLUDE_DIRS}) 

set(SOURCES main.cpp spi.cpp spi.h)

add_executable( ${PROJECT_NAME} ${SOURCES} )

target_link_libraries( Stereo ${OpenCV_LIBS} /home/pi/Desktop/ThermalAndPiCameras/leptonSDKEmb32PUB/Debug/libLEPTON_SDK.a )

