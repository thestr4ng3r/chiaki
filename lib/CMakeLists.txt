
set(HEADER_FILES
		include/chiaki/session.h
		include/chiaki/common.h
		include/chiaki/thread.h
		include/chiaki/base64.h
		include/chiaki/http.h
		include/chiaki/log.h
		include/chiaki/ctrl.h
		include/chiaki/rpcrypt.h
		include/chiaki/takion.h
		include/chiaki/senkusha.h
		include/chiaki/nagare.h
		include/chiaki/mirai.h
		include/chiaki/ecdh.h
		include/chiaki/launchspec.h
		include/chiaki/random.h
		include/chiaki/gkcrypt.h
		include/chiaki/audio.h
		include/chiaki/audioreceiver.h
		include/chiaki/video.h
		include/chiaki/videoreceiver.h
		include/chiaki/frameprocessor.h
		include/chiaki/seqnum.h
		include/chiaki/discovery.h
		include/chiaki/congestioncontrol.h
		include/chiaki/stoppipe.h)

set(SOURCE_FILES
		src/common.c
		src/session.c
		src/thread.c
		src/base64.c
		src/http.c
		src/log.c
		src/ctrl.c
		src/rpcrypt.c
		src/takion.c
		src/senkusha.c
		src/utils.h
		src/pb_utils.h
		src/nagare.c
		src/mirai.c
		src/ecdh.c
		src/launchspec.c
		src/random.c
		src/gkcrypt.c
		src/audio.c
		src/audioreceiver.c
		src/videoreceiver.c
		src/frameprocessor.c
		src/discovery.c
		src/congestioncontrol.c
		src/stoppipe.c)

add_subdirectory(protobuf)
include_directories("${NANOPB_SOURCE_DIR}")
set_source_files_properties(${CHIAKI_LIB_PROTO_SOURCE_FILES} ${CHIAKI_LIB_PROTO_HEADER_FILES} PROPERTIES GENERATED TRUE)
include_directories("${CHIAKI_LIB_PROTO_INCLUDE_DIR}")

find_package(Opus REQUIRED)
include_directories(${Opus_INCLUDE_DIRS})

add_library(chiaki-lib ${HEADER_FILES} ${SOURCE_FILES} ${CHIAKI_LIB_PROTO_SOURCE_FILES} ${CHIAKI_LIB_PROTO_HEADER_FILES})
add_dependencies(chiaki-lib chiaki-pb)
set_target_properties(chiaki-lib PROPERTIES OUTPUT_NAME chiaki)

target_include_directories(chiaki-lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(Threads)
target_link_libraries(chiaki-lib Threads::Threads)

find_package(OpenSSL REQUIRED)
target_link_libraries(chiaki-lib OpenSSL::Crypto)

target_link_libraries(chiaki-lib protobuf-nanopb-static)

target_link_libraries(chiaki-lib ${Opus_LIBRARIES})