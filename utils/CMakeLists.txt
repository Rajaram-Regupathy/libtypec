cmake_minimum_required(VERSION 3.0.0)
project(lstypec VERSION 0.1.0)

include(CTest)
enable_testing()

get_filename_component(libtypec_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_executable(lstypec lstypec.c)
target_link_libraries(lstypec "${libtypec_PROJECT_ROOT}/libtypec/bin/liblibtypec.a")


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
