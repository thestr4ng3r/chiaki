# FindArgp
# Will define Target Argp::Argp

find_path(Argp_INCLUDE_DIR
	NAMES argp.h)

find_library(Argp_LIBRARY
	NAMES argp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Argp
	REQUIRED_VARS Argp_LIBRARY Argp_INCLUDE_DIR)

if(Argp_FOUND AND NOT TARGET Argp::Argp)
	add_library(Argp::Argp UNKNOWN IMPORTED)
	set_target_properties(Argp::Argp PROPERTIES
		IMPORTED_LOCATION "${Argp_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${Argp_INCLUDE_DIR}")
endif()

