
##################
# nanopb
##################

find_package(PythonInterp 3 REQUIRED) # Make sure nanopb doesn't find Python 2.7 because Python 2 should just die.

add_subdirectory(nanopb)
set(NANOPB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/nanopb")
set(NANOPB_SOURCE_DIR "${NANOPB_SOURCE_DIR}" PARENT_SCOPE)
set(NANOPB_GENERATOR_PY "${NANOPB_SOURCE_DIR}/generator/nanopb_generator.py" PARENT_SCOPE)

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
