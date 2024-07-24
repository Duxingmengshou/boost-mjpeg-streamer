cmake_minimum_required(VERSION 3.26)
project(boost-mjpeg-streamer)

set(CMAKE_CXX_STANDARD 17)

#OpenCV
set(OpenCV_DIR "H:/Programming/C++/OpenCV/OpenCV490/opencv/build/x64/vc16/lib")
find_package(OpenCV REQUIRED)

#Boost
set(Boost_ROOT "D:/boost/msvc_bin_complete/lib")
find_package(Boost REQUIRED)

add_executable(boost-mjpeg-streamer
        main.cpp
        main.cpp
)

target_link_libraries(boost-mjpeg-streamer ${OpenCV_LIBS})
target_link_libraries(boost-mjpeg-streamer ${Boost_LIBRARIES})
