
cmake_minimum_required(VERSION 3.2)

project(chiaki)

option(CHIAKI_ENABLE_TESTS "Enable tests for Chiaki" ON)
option(CHIAKI_ENABLE_CLI "Enable CLI for Chiaki" OFF)
option(CHIAKI_ENABLE_GUI "Enable Qt GUI" ON)
option(CHIAKI_ENABLE_ANDROID "Enable Android (Use only as part of the Gradle Project)" OFF)
option(CHIAKI_LIB_ENABLE_OPUS "Use Opus as part of Chiaki Lib" ON)
option(CHIAKI_LIB_OPENSSL_EXTERNAL_PROJECT "Use OpenSSL as CMake external project" OFF)
option(CHIAKI_GUI_ENABLE_QT_GAMEPAD "Use QtGamepad for Input" OFF)
option(CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER "Use SDL Gamecontroller for Input" ON)
option(CHIAKI_CLI_ARGP_STANDALONE "Search for standalone argp lib for CLI" OFF)

set(CHIAKI_VERSION_MAJOR 1)
set(CHIAKI_VERSION_MINOR 2)
set(CHIAKI_VERSION_PATCH 0)
set(CHIAKI_VERSION ${CHIAKI_VERSION_MAJOR}.${CHIAKI_VERSION_MINOR}.${CHIAKI_VERSION_PATCH})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(third-party)

add_definitions(-DCHIAKI_VERSION_MAJOR=${CHIAKI_VERSION_MAJOR} -DCHIAKI_VERSION_MINOR=${CHIAKI_VERSION_MINOR} -DCHIAKI_VERSION_PATCH=${CHIAKI_VERSION_PATCH} -DCHIAKI_VERSION=\"${CHIAKI_VERSION}\")

if(CHIAKI_LIB_OPENSSL_EXTERNAL_PROJECT)
	include(OpenSSLExternalProject)
endif()

add_subdirectory(lib)

if(CHIAKI_ENABLE_CLI)
	add_subdirectory(cli)
endif()

if(CHIAKI_ENABLE_GUI)
	add_subdirectory(gui)
endif()

if(CHIAKI_ENABLE_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()

if(CHIAKI_ENABLE_ANDROID)
	add_subdirectory(android/app)
endif()