# Provides: Udev::libudev

set(_prefix Udev)
set(_target "${_prefix}::libudev")

find_package(PkgConfig)
if(PkgConfig_FOUND)
	pkg_check_modules("${_prefix}" libudev)
	if(${_prefix}_FOUND AND NOT TARGET "${_target}")
		add_library("${_target}" INTERFACE IMPORTED)
		target_link_libraries("${_target}" INTERFACE ${${_prefix}_LIBRARIES})
		target_include_directories("${_target}" INTERFACE ${${_prefix}_INCLUDE_DIRS})
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("${_prefix}"
	REQUIRED_VARS "${_prefix}_LIBRARIES"
	VERSION_VAR "${_prefix}_VERSION")

unset(_prefix)
unset(_target)
