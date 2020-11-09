
cmake_minimum_required(VERSION 3.2)

project(libsetsu)

option(SETSU_BUILD_DEMO "Build testing executable for libsetsu" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_library(setsu
	include/setsu.h
	src/setsu.c)

target_include_directories(setsu PUBLIC include)

find_package(Udev REQUIRED)
find_package(Evdev REQUIRED)
target_link_libraries(setsu Udev::libudev Evdev::libevdev)

if(SETSU_BUILD_DEMO)
	add_executable(setsu-demo
		demo/main.c)
	target_link_libraries(setsu-demo setsu)
endif()

