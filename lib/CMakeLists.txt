
set(HEADER_FILES
		include/chiaki/session.h
		include/chiaki/common.h
		include/chiaki/thread.h
		include/chiaki/base64.h
		include/chiaki/http.h
		include/chiaki/log.h
		include/chiaki/ctrl.h)

set(SOURCE_FILES
		src/common.c
		src/session.c
		src/thread.c
		src/base64.c
		src/http.c
		src/log.c
		src/ctrl.c)

add_library(chiaki-lib ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(chiaki-lib PROPERTIES OUTPUT_NAME chiaki)

target_include_directories(chiaki-lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(Threads)
target_link_libraries(chiaki-lib Threads::Threads)
