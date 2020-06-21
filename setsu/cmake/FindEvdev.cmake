# Provides: Evdev::libevdev

set(_prefix Evdev)
set(_target "${_prefix}::libevdev")

find_package(PkgConfig REQUIRED)

pkg_check_modules("${_prefix}" libevdev)

function(resolve_location)

endfunction()

if(${_prefix}_FOUND)
	add_library("${_target}" INTERFACE IMPORTED)
	target_link_libraries("${_target}" INTERFACE ${${_prefix}_LIBRARIES})
	target_include_directories("${_target}" INTERFACE ${${_prefix}_INCLUDE_DIRS})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("${_prefix}"
	REQUIRED_VARS "${_prefix}_LIBRARIES"
	VERSION_VAR "${_prefix}_VERSION")

unset(_prefix)
unset(_target)
