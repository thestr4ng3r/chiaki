
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Multimedia)

find_package(FFMPEG REQUIRED COMPONENTS avcodec)

add_executable(chiaki
		src/main.cpp
		include/streamwindow.h
		src/streamwindow.cpp
		include/videodecoder.h
		src/videodecoder.cpp
		include/discoverycmd.h
		src/discoverycmd.cpp
		include/mainwindow.h
		src/mainwindow.cpp
		include/dynamicgridwidget.h
		src/dynamicgridwidget.cpp
		include/serveritemwidget.h
		src/serveritemwidget.cpp)
target_include_directories(chiaki PRIVATE include)

target_link_libraries(chiaki chiaki-lib)
target_link_libraries(chiaki FFMPEG::avcodec)
target_link_libraries(chiaki Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Multimedia)
