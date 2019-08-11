
set(SOURCE
		include/chiaki-cli.h
		src/discover.c)

add_library(chiaki-cli-lib STATIC ${SOURCE})
target_include_directories(chiaki-cli-lib PUBLIC "include")
target_link_libraries(chiaki-cli-lib chiaki-lib)

add_executable(chiaki-cli src/main.c)
target_link_libraries(chiaki-cli chiaki-cli-lib)
