cmake_minimum_required(VERSION 3.0)
project(unifyfs_pegasus_test)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_SOURCE_DIR}/dependency/.spack-env/view)
include_directories(${CMAKE_SOURCE_DIR}/dependency/.spack-env/view/include)

add_subdirectory(apps)