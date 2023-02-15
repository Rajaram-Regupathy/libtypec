cmake_minimum_required(VERSION 3.0.0)
project(libtypec VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(libtypec libtypec.c libtypec_sysfs_ops.c)

set(CMAKE_C_FLAGS "-g -O2 -fstack-protector-strong -Wformat=1 -Werror=format-security -Wdate-time -fasynchronous-unwind-tables -D_FORTIFY_SOURCE=2")

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
