
cmake_minimum_required(VERSION 3.2)

project(chiaki)

option(CHIAKI_ENABLE_TESTS "Enable tests for Chiaki" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(third-party)

add_subdirectory(lib)
add_subdirectory(gui)

if(CHIAKI_ENABLE_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()
