
cmake_minimum_required(VERSION 3.2)

project(chiaki)

option(CHIAKI_ENABLE_TESTS "Enable tests for Chiaki" ON)
option(CHIAKI_ENABLE_CLI "Enable CLI for Chiaki" OFF)
option(CHIAKI_GUI_ENABLE_QT_GAMEPAD "Use QtGamepad for Input" ON)

set(CHIAKI_VERSION_MAJOR 1)
set(CHIAKI_VERSION_MINOR 0)
set(CHIAKI_VERSION_PATCH 0)
set(CHIAKI_VERSION ${CHIAKI_VERSION_MAJOR}.${CHIAKI_VERSION_MINOR}.${CHIAKI_VERSION_PATCH})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(third-party)

add_definitions(-DCHIAKI_VERSION_MAJOR=${CHIAKI_VERSION_MAJOR} -DCHIAKI_VERSION_MINOR=${CHIAKI_VERSION_MINOR} -DCHIAKI_VERSION_PATCH=${CHIAKI_VERSION_PATCH})

add_subdirectory(lib)

if(CHIAKI_ENABLE_CLI)
	add_subdirectory(cli)
endif()

add_subdirectory(gui)

if(CHIAKI_ENABLE_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()
