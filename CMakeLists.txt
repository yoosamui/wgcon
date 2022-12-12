# minimum cmake version
cmake_minimum_required(VERSION 3.10)

# project name, version
project(libnm_example)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNM REQUIRED libnm)
pkg_check_modules(GLIB REQUIRED glib-2.0)

link_libraries( ${LIBNM_LIBRARIES} ${GLIB_LIBRARIES})
include_directories( ${LIBNM_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})
add_compile_options( ${LIBNM_CFLAGS_OTHER} ${GLIB_CFLAGS_OTHER})

include_directories(
  include include/
)


add_executable(wgcon wgcon.cpp)

