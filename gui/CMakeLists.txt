
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Multimedia OpenGL)

find_package(QtAV REQUIRED)

add_executable(chiaki
		src/main.cpp
		include/streamwindow.h
		src/streamwindow.cpp)
target_include_directories(chiaki PRIVATE include)

target_link_libraries(chiaki chiaki-lib)

target_link_libraries(chiaki Qt5::Core Qt5::Widgets Qt5::Gui Qt5::OpenGL Qt5::Multimedia)

target_include_directories(chiaki PRIVATE ${QTAV_INCLUDE_DIRS} ${QTAVWIDGETS_INCLUDE_DIRS})
target_link_libraries(chiaki ${QTAV_LIBRARIES} ${QTAVWIDGETS_LIBRARIES})