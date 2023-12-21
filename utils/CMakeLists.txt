cmake_minimum_required(VERSION 3.25.2)
project(libtypec_utils VERSION 0.5.0)

configure_file(libtypec_utils_config.h.in libtypec_utils_config.h)


add_executable(lstypec lstypec.c names.c)
add_executable(typecstatus typecstatus.c names.c)

target_link_libraries(lstypec PUBLIC libtypec udev)
target_link_libraries(typecstatus PUBLIC libtypec udev)

option(LIBTYPEC_STRICT_CFLAGS "Compile for strict warnings" ON)
if(LIBTYPEC_STRICT_CFLAGS)
    set(CMAKE_C_FLAGS "-g -O2 -fstack-protector-strong -Wformat=1 -Werror=format-security -Wdate-time -fasynchronous-unwind-tables -D_FORTIFY_SOURCE=2")
endif()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)