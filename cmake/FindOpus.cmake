#  Opus_FOUND
#  Opus_INCLUDE_DIRS
#  Opus_LIBRARIES

find_path(Opus_INCLUDE_DIRS
	NAMES opus/opus.h
	PATH_SUFFIXES include
)

find_library(Opus_LIBRARIES NAMES opus)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Opus
	DEFAULT_MSG
	Opus_INCLUDE_DIRS Opus_LIBRARIES
)
