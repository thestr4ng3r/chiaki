
find_package(PythonInterp 3 REQUIRED) # Make sure nanopb doesn't find Python 2.7 because Python 2 should just die.

add_subdirectory(nanopb)
set(NANOPB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/nanopb")
set(NANOPB_SOURCE_DIR "${NANOPB_SOURCE_DIR}" PARENT_SCOPE)
set(NANOPB_GENERATOR_PY "${NANOPB_SOURCE_DIR}/generator/nanopb_generator.py" PARENT_SCOPE)