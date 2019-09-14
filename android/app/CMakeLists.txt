
cmake_minimum_required(VERSION 3.2)

add_library(chiaki-jni SHARED src/main/cpp/chiaki-jni.c)
target_link_libraries(chiaki-jni chiaki-lib)