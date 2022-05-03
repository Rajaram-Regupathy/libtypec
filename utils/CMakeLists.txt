cmake_minimum_required(VERSION 3.0.0)
project(lstypec VERSION 0.1.0)

include(CTest)
enable_testing()

get_filename_component(libtypec_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_executable(lstypec lstypec.c names.c)
add_executable(typecstatus typecstatus.c names.c)

target_link_libraries(lstypec "${CMAKE_CURRENT_LIST_DIR}/../bin/liblibtypec.a" udev)
target_link_libraries(typecstatus "${CMAKE_CURRENT_LIST_DIR}/../bin/liblibtypec.a" udev)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
