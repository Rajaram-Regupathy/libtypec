cmake_minimum_required(VERSION 3.16.3)
project(libtypec_utils VERSION 0.5.1)

configure_file(libtypec_utils_config.h.in libtypec_utils_config.h)


add_executable(lstypec lstypec.c names.c)
target_include_directories(lstypec PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(lstypec PUBLIC libtypec udev)

add_executable(typecstatus typecstatus.c names.c)
target_link_libraries(typecstatus PUBLIC libtypec udev)

option(LIBTYPEC_STRICT_CFLAGS "Compile for strict warnings" ON)
if(LIBTYPEC_STRICT_CFLAGS)
    target_compile_options(lstypec PRIVATE -g -O2 -fstack-protector-strong -Wformat=1 -Werror=format-security -Wdate-time -fasynchronous-unwind-tables -D_FORTIFY_SOURCE=2)
    target_compile_options(typecstatus PRIVATE -g -O2 -fstack-protector-strong -Wformat=1 -Werror=format-security -Wdate-time -fasynchronous-unwind-tables -D_FORTIFY_SOURCE=2)
endif()
