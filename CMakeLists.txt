cmake_minimum_required(VERSION 3.0.0)
project(libtypec VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(libtypec libtypec.c libtypec_sysfs_ops.c)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
