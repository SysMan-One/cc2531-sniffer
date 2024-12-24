cmake_minimum_required(VERSION 3.16)
project(cc2531 VERSION 0.1 LANGUAGES C)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_COLOR_MAKEFILE   ON)

find_package(PkgConfig)


pkg_search_module(LIBUSB REQUIRED libusb-1.0)					# LIBUSB stuff


set (3RDPARTY_ROOT	3rdparty/ )
set (STARLET_ROOT       ${3RDPARTY_ROOT}/utility_routines/ )

include_directories (
	./
	${LIBUSB_INCLUDE_DIRS}
	${STARLET_ROOT}
	${3RDPARTY_ROOT}
)




set (EXE cc2531)								# Set target excutable

add_executable(${EXE} cc2531.c  cc2531-sniffer.c  ieee802154.c)
add_subdirectory(${STARLET_ROOT})						# Set of utility routines for general purpose
add_dependencies( ${EXE} starlet)


target_compile_options( ${EXE} PRIVATE ${LIBUSB_CFLAGS} )
target_link_libraries( ${EXE} PRIVATE ${LIBUSB_LIBRARIES} )

target_link_libraries(${EXE} PRIVATE starlet)
