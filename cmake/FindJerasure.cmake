# Provides Jerasure::Jerasure

find_path(Jerasure_INCLUDE_DIR NAMES jerasure.h)
find_path(Jerasure_INCLUDE_DIR2 NAMES galois.h PATH_SUFFIXES jerasure)
find_library(Jerasure_LIBRARY NAMES Jerasure)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Jerasure
	FOUND_VAR Jerasure_FOUND
	REQUIRED_VARS
		Jerasure_LIBRARY
		Jerasure_INCLUDE_DIR
		Jerasure_INCLUDE_DIR2
)

if(Jerasure_FOUND)
	set(Jerasure_LIBRARIES ${Jerasure_LIBRARY})
	set(Jerasure_INCLUDE_DIRS ${Jerasure_INCLUDE_DIR} ${Jerasure_INCLUDE_DIR2})
	if(NOT TARGET Jerasure::Jerasure)
		add_library(Jerasure::Jerasure UNKNOWN IMPORTED)
		set_target_properties(Jerasure::Jerasure PROPERTIES
		  IMPORTED_LOCATION "${Jerasure_LIBRARY}"
		  INTERFACE_INCLUDE_DIRECTORIES "${Jerasure_INCLUDE_DIRS}"
		)
	endif()
endif()
