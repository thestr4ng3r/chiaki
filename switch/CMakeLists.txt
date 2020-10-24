
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(FFMPEG REQUIRED COMPONENTS avcodec avutil swscale)
find_library(SDL2 SDL2)
find_library(SWRESAMPLE swresample)

# find -type f | grep -P '\.(h|cpp)$' | sed 's#\./#\t\t#g'
add_executable(chiaki WIN32
	src/discoverymanager.cpp
	src/settings.cpp
	src/io.cpp
	src/host.cpp
	src/main.cpp
	src/gui.cpp
)

target_include_directories(chiaki PRIVATE include)

target_link_libraries(chiaki
	chiaki-lib
	borealis
	${SDL2}
    FFMPEG::avcodec
    FFMPEG::avutil
    FFMPEG::swscale
	${SWRESAMPLE}
	${SWSCALE})

# OS links
if(CHIAKI_SWITCH_ENABLE_LINUX)
	target_link_libraries(chiaki dl)
else()
	# libnx is forced by the switch toolchain
	find_library(Z z)
	find_library(GLAPI glapi)
	find_library(DRM_NOUVEAU drm_nouveau)
	target_link_libraries(chiaki ${Z} ${GLAPI} ${DRM_NOUVEAU})
endif()

install(TARGETS chiaki
	RUNTIME DESTINATION bin
	BUNDLE DESTINATION bin)

if(CHIAKI_ENABLE_SWITCH AND NOT CHIAKI_SWITCH_ENABLE_LINUX)
	add_nro_target(chiaki "chiaki" "Chiaki team" "${CHIAKI_VERSION}" "${CMAKE_SOURCE_DIR}/switch/res/icon.jpg" "${CMAKE_SOURCE_DIR}/switch/res")
endif()
