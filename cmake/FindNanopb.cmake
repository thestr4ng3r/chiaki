# Provides Nanopb::nanopb and NANOPB_GENERATOR_PY

find_package(nanopb CONFIG)
find_file(NANOPB_GENERATOR_PY nanopb_generator.py PATH_SUFFIXES bin)

find_path(Jerasure_INCLUDE_DIR NAMES pb_encode.h pb_decode.h)
find_library(Jerasure_LIBRARY NAMES Jerasure)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Nanopb
	FOUND_VAR Nanopb_FOUND
	REQUIRED_VARS
		nanopb_FOUND
		NANOPB_GENERATOR_PY
)

if(Nanopb_FOUND)
	if(NOT TARGET Nanopb::nanopb)
		add_library(Nanopb::nanopb ALIAS nanopb::protobuf-nanopb-static)
		set_target_properties(Jerasure::Jerasure PROPERTIES
		  IMPORTED_LOCATION "${Jerasure_LIBRARY}"
		  INTERFACE_INCLUDE_DIRECTORIES "${Jerasure_INCLUDE_DIRS}"
		)
	endif()
endif()
