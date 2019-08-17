
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Multimedia OpenGL Svg)
if(CHIAKI_GUI_ENABLE_QT_GAMEPAD)
	find_package(Qt5 REQUIRED COMPONENTS Gamepad)
endif()

find_package(FFMPEG REQUIRED COMPONENTS avcodec)

set(RESOURCE_FILES "")

if(APPLE)
	list(APPEND RESOURCE_FILES "chiaki.icns")
endif()

add_executable(chiaki
		include/exception.h
		src/main.cpp
		include/streamwindow.h
		src/streamwindow.cpp
		include/videodecoder.h
		src/videodecoder.cpp
		include/mainwindow.h
		src/mainwindow.cpp
		include/dynamicgridwidget.h
		src/dynamicgridwidget.cpp
		include/serveritemwidget.h
		src/serveritemwidget.cpp
		include/discoverymanager.h
		src/discoverymanager.cpp
		include/streamsession.h
		src/streamsession.cpp
		include/sessionlog.h
		src/sessionlog.cpp
		include/avopenglwidget.h
		src/avopenglwidget.cpp
		include/avopenglframeuploader.h
		src/avopenglframeuploader.cpp
		include/servericonwidget.h
		src/servericonwidget.cpp
		include/settings.h
		src/settings.cpp
		include/registdialog.h
		src/registdialog.cpp
		include/host.h
		src/host.cpp
		include/settingsdialog.h
		src/settingsdialog.cpp
		include/manualhostdialog.h
		src/manualhostdialog.cpp
		res/resources.qrc
		${RESOURCE_FILES})
target_include_directories(chiaki PRIVATE include)

target_link_libraries(chiaki chiaki-lib)
if(CHIAKI_ENABLE_CLI)
	add_definitions(CHIAKI_ENABLE_CLI)
	target_link_libraries(chiaki chiaki-cli-lib)
endif()

target_link_libraries(chiaki FFMPEG::avcodec)
target_link_libraries(chiaki Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Multimedia Qt5::OpenGL Qt5::Svg)
if(CHIAKI_GUI_ENABLE_QT_GAMEPAD)
	target_link_libraries(chiaki Qt5::Gamepad)
	target_compile_definitions(chiaki PRIVATE CHIAKI_GUI_ENABLE_QT_GAMEPAD)
endif()

set_target_properties(chiaki PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in"
		MACOSX_BUNDLE_BUNDLE_NAME Chiaki
		MACOSX_BUNDLE_BUNDLE_VERSION ${CHIAKI_VERSION}
		MACOSX_BUNDLE_COPYRIGHT "thestr4ng3r (GPLv3)"
		MACOSX_BUNDLE_GUI_IDENTIFIER "org.chiaki.chiaki"
		MACOSX_BUNDLE_ICON_FILE chiaki.icns
		RESOURCE "${RESOURCE_FILES}")

install(TARGETS chiaki
		RUNTIME DESTINATION bin
		BUNDLE DESTINATION bin)
install(FILES chiaki.desktop DESTINATION share/applications)
install(FILES chiaki.png DESTINATION share/icons/hicolor/512x512)