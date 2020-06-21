
cmake_minimum_required(VERSION 3.2)

project(chiaki)

option(CHIAKI_ENABLE_TESTS "Enable tests for Chiaki" ON)
option(CHIAKI_ENABLE_CLI "Enable CLI for Chiaki" OFF)
option(CHIAKI_ENABLE_GUI "Enable Qt GUI" ON)
option(CHIAKI_ENABLE_ANDROID "Enable Android (Use only as part of the Gradle Project)" OFF)
option(CHIAKI_ENABLE_SWITCH "Enable Nintendo Switch (Requires devKitPro libnx)" OFF)
option(CHIAKI_LIB_ENABLE_OPUS "Use Opus as part of Chiaki Lib" ON)
option(CHIAKI_LIB_ENABLE_MBEDTLS "Use mbedtls instead of OpenSSL as part of Chiaki Lib" OFF)
option(CHIAKI_LIB_OPENSSL_EXTERNAL_PROJECT "Use OpenSSL as CMake external project" OFF)
option(CHIAKI_GUI_ENABLE_QT_GAMEPAD "Use QtGamepad for Input" OFF)
option(CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER "Use SDL Gamecontroller for Input" ON)
option(CHIAKI_CLI_ARGP_STANDALONE "Search for standalone argp lib for CLI" OFF)

set(CHIAKI_VERSION_MAJOR 1)
set(CHIAKI_VERSION_MINOR 2)
set(CHIAKI_VERSION_PATCH 1)
set(CHIAKI_VERSION ${CHIAKI_VERSION_MAJOR}.${CHIAKI_VERSION_MINOR}.${CHIAKI_VERSION_PATCH})

set(CPACK_PACKAGE_NAME "chiaki")
set(CPACK_PACKAGE_DESCRIPTION "Open Source PS4 remote play client")
set(CPACK_PACKAGE_VERSION_MAJOR ${CHIAKI_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${CHIAKI_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${CHIAKI_VERSION_PATCH})
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
set(CPACK_DEBIAN_PACKAGE_SECTION "games")
include(CPack)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# configure nintendo switch toolchain
if(CHIAKI_ENABLE_SWITCH AND CHIAKI_ENABLE_SWITCH_LINUX)
	# CHIAKI_ENABLE_SWITCH_LINUX is a special testing version
	# the aim is to troubleshoot nitendo switch chiaki verison
	# from a x86 linux os
	add_definitions(-DCHIAKI_ENABLE_SWITCH_LINUX)
	set(CMAKE_BUILD_TYPE Debug)
elseif(CHIAKI_ENABLE_SWITCH)
	add_definitions(-D__SWITCH__)
	# TODO check if android ... or other versions are enabled
	# force mbedtls as crypto lib
	set(CHIAKI_LIB_ENABLE_MBEDTLS ON)
endif()

add_subdirectory(third-party)

add_definitions(-DCHIAKI_VERSION_MAJOR=${CHIAKI_VERSION_MAJOR} -DCHIAKI_VERSION_MINOR=${CHIAKI_VERSION_MINOR} -DCHIAKI_VERSION_PATCH=${CHIAKI_VERSION_PATCH} -DCHIAKI_VERSION=\"${CHIAKI_VERSION}\")

if(CHIAKI_LIB_OPENSSL_EXTERNAL_PROJECT)
	include(OpenSSLExternalProject)
endif()

if(CHIAKI_LIB_ENABLE_MBEDTLS)
	add_definitions(-DCHIAKI_LIB_ENABLE_MBEDTLS)
endif()

add_subdirectory(lib)

if(CHIAKI_ENABLE_CLI)
	add_subdirectory(cli)
endif()

if(CHIAKI_ENABLE_GUI)
	#add_subdirectory(setsu)
	add_subdirectory(gui)
endif()

if(CHIAKI_ENABLE_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()

if(CHIAKI_ENABLE_ANDROID)
	add_subdirectory(android/app)
endif()

if(CHIAKI_ENABLE_SWITCH)
	#TODO
	#add_subdirectory(switch)
endif()
