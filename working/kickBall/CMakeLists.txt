# cmake needs this line
cmake_minimum_required(VERSION 2.8)

# Define project name
project(MOT_project)

set (CMAKE_PREFIX_PATH "/usr/local/lib/cmake")
# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# via the command line or GUI
find_package(OpenCV REQUIRED)
find_package(raspicam REQUIRED)
# If the package has been found, several variables will
# be set, you can find the full list with descriptions
# in the OpenCVConfig.cmake file.
# Print some message showing some of them
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

# Add OpenCV headers location to your include paths
include_directories(${OpenCV_INCLUDE_DIRS})

# Declare the executable target built from your sources
add_executable(MOT objectTracer.cpp uart_api.cpp)

# Link your application with OpenCV libraries
target_link_libraries(MOT ${OpenCV_LIBS})
target_link_libraries(MOT ${raspicam_CV_LIBS})