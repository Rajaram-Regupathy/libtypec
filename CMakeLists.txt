cmake_minimum_required(VERSION 3.0.0)
project(libtypec VERSION 0.3.1 DESCRIPTION "USB-Type C and USB Power Delivery class devices")

include(GNUInstallDirs)
include(CTest)
enable_testing()

# set arch-specific include dir
set(CMAKE_INSTALL_INCLUDEDIR "include/${CMAKE_LIBRARY_ARCHITECTURE}")

add_library(libtypec SHARED libtypec.c libtypec_sysfs_ops.c libtypec_dbgfs_ops.c)

set(CMAKE_C_FLAGS "-g -O2 -fstack-protector-strong -Wformat=1 -Werror=format-security -Wdate-time -fasynchronous-unwind-tables -D_FORTIFY_SOURCE=2")
set_target_properties(libtypec PROPERTIES
	OUTPUT_NAME "typec"
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION}
	PUBLIC_HEADER libtypec.h)


set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

install(TARGETS libtypec
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

configure_file(
    libtypec.pc.in
    ${CMAKE_BINARY_DIR}/libtypec.pc
    @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/libtypec.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
