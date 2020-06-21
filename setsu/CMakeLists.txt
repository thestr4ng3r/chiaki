
cmake_minimum_required(VERSION 3.2)

project(libsetsu)

option(SETSU_BUILD_TEST "Build testing executable for libsetsu" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_library(setsu
	include/setsu.h
	src/setsu.c)

target_include_directories(setsu PUBLIC include)

find_package(UDEV REQUIRED)
target_link_libraries(setsu UDEV::libudev)

if(SETSU_BUILD_TEST)
	add_executable(setsutest
		test/main.c)
	target_link_libraries(setsutest setsu)
endif()

