
##################
# nanopb
##################

find_package(PythonInterp 3 REQUIRED) # Make sure nanopb doesn't find Python 2.7 because Python 2 should just die.

add_subdirectory(nanopb EXCLUDE_FROM_ALL)
set(NANOPB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/nanopb")
set(NANOPB_SOURCE_DIR "${NANOPB_SOURCE_DIR}" PARENT_SCOPE)
set(NANOPB_GENERATOR_PY "${NANOPB_SOURCE_DIR}/generator/nanopb_generator.py" PARENT_SCOPE)

if(NOT CHIAKI_USE_SYSTEM_JERASURE)
	##################
	# gf-complete
	##################

	set(GF_COMPLETE_SOURCE
			gf-complete/src/gf.c
			gf-complete/src/gf_wgen.c
			gf-complete/src/gf_w4.c
			gf-complete/src/gf_w8.c
			gf-complete/src/gf_w16.c
			gf-complete/src/gf_w32.c
			gf-complete/src/gf_w64.c
			gf-complete/src/gf_w128.c
			gf-complete/src/gf_rand.c
			gf-complete/src/gf_general.c
			gf-complete/src/gf_cpu.c)

	# TODO: support NEON

	add_library(gf_complete STATIC ${GF_COMPLETE_SOURCE})
	target_include_directories(gf_complete PUBLIC gf-complete/include)

	##################
	# jerasure
	##################

	set(JERASURE_SOURCE
			jerasure/src/galois.c
			jerasure/src/jerasure.c
			jerasure/src/reed_sol.c
			jerasure/src/cauchy.c
			jerasure/src/liberation.c)

	add_library(jerasure STATIC ${JERASURE_SOURCE})
	target_include_directories(jerasure PUBLIC jerasure/include)
	target_link_libraries(jerasure gf_complete)

	add_library(Jerasure::Jerasure ALIAS jerasure)
endif()

##################
# borealis
##################

if(CHIAKI_ENABLE_SWITCH)
    # do not include
    # borealis/library/lib/switch_wrapper.c
    # switch functions are in switch/src/main.cpp
	set(BOREALIS_SOURCE
        borealis/library/lib/extern/libretro-common/features/features_cpu.c
        borealis/library/lib/extern/libretro-common/encodings/encoding_utf.c
        borealis/library/lib/extern/libretro-common/compat/compat_strl.c
        borealis/library/lib/extern/nxfmtwrapper/format.cpp
        borealis/library/lib/extern/nanovg/nanovg.c
        borealis/library/lib/extern/glad/glad.c
        borealis/library/lib/scroll_view.cpp
        borealis/library/lib/style.cpp
        borealis/library/lib/table.cpp
        borealis/library/lib/task_manager.cpp
        borealis/library/lib/progress_display.cpp
        borealis/library/lib/staged_applet_frame.cpp
        borealis/library/lib/applet_frame.cpp
        borealis/library/lib/hint.cpp
        borealis/library/lib/image.cpp
        borealis/library/lib/logger.cpp
        borealis/library/lib/swkbd.cpp
        borealis/library/lib/crash_frame.cpp
        borealis/library/lib/header.cpp
        borealis/library/lib/progress_spinner.cpp
        borealis/library/lib/layer_view.cpp
        borealis/library/lib/notification_manager.cpp
        borealis/library/lib/rectangle.cpp
        borealis/library/lib/application.cpp
        borealis/library/lib/box_layout.cpp
        borealis/library/lib/sidebar.cpp
        borealis/library/lib/dropdown.cpp
        borealis/library/lib/popup_frame.cpp
        borealis/library/lib/repeating_task.cpp
        borealis/library/lib/absolute_layout.cpp
        borealis/library/lib/i18n.cpp
        borealis/library/lib/tab_frame.cpp
        borealis/library/lib/thumbnail_frame.cpp
        borealis/library/lib/animations.cpp
        borealis/library/lib/dialog.cpp
        borealis/library/lib/view.cpp
        borealis/library/lib/list.cpp
        borealis/library/lib/button.cpp
        borealis/library/lib/label.cpp
        borealis/library/lib/theme.cpp
        borealis/library/lib/material_icon.cpp)

	add_library(borealis STATIC ${BOREALIS_SOURCE})
	target_include_directories(borealis PUBLIC
		borealis/library/include
        borealis/library/include/borealis/extern
		borealis/library/include/borealis/extern/glad
		borealis/library/include/borealis/extern/nanovg
		borealis/library/include/borealis/extern/libretro-common
		borealis/library/lib/extern/fmt/include)

    find_package(glfw3 REQUIRED)
	find_library(EGL EGL)
	find_library(GLAPI glapi)
	find_library(DRM_NOUVEAU drm_nouveau)
	target_link_libraries(borealis
		glfw
		${EGL}
		${GLAPI}
		${DRM_NOUVEAU})
endif()
