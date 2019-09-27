
if(NOT CHIAKI_VERSION OR NOT CHIAKI_DMG OR NOT CHIAKI_DMG_FILENAME OR NOT CHIAKI_CASK_OUT)
	message(FATAL_ERROR "CHIAKI_VERSION, CHIAKI_DMG, CHIAKI_DMG_FILENAME and CHIAKI_CASK_OUT must be set.")
endif()

if(CHIAKI_VERSION MATCHES "^v([0-9].*)$")
	set(CHIAKI_VERSION "${CMAKE_MATCH_1}")
endif()

file(SHA256 "${CHIAKI_DMG}" CHIAKI_DMG_SHA256)
configure_file("${CMAKE_CURRENT_LIST_DIR}/chiaki.rb.in" "${CHIAKI_CASK_OUT}")
