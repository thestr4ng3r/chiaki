# https://github.com/nxengine/nxengine-evo

# Find DEVKITPRO
if(NOT DEFINED ENV{DEVKITPRO})
	message(FATAL_ERROR "You must have defined DEVKITPRO before calling cmake.")
endif()

set(DEVKITPRO $ENV{DEVKITPRO})

function(switchvar cmakevar var default)
	# read or set env var 
	if(NOT DEFINED "ENV{$var}")
		set("ENV{$var}" default)
	endif()
	set("$cmakevar" "ENV{$var}")
endfunction()

# allow gcc -g to use 
# aarch64-none-elf-addr2line -e build_switch/switch/chiaki -f -p -C -a 0xCCB5C
set(CMAKE_BUILD_TYPE Debug)

set( TOOL_OS_SUFFIX "" )
if( CMAKE_HOST_WIN32 )
 set( TOOL_OS_SUFFIX ".exe" )
endif()

set(CMAKE_SYSTEM_PROCESSOR "armv8-a")
set(CMAKE_C_COMPILER   "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc${TOOL_OS_SUFFIX}"     CACHE PATH "C compiler")
set(CMAKE_CXX_COMPILER "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-g++${TOOL_OS_SUFFIX}"     CACHE PATH "C++ compiler")
set(CMAKE_ASM_COMPILER "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-as${TOOL_OS_SUFFIX}"      CACHE PATH "C++ compiler")
set(CMAKE_STRIP        "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-strip${TOOL_OS_SUFFIX}"   CACHE PATH "strip")
set(CMAKE_AR           "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-ar${TOOL_OS_SUFFIX}"      CACHE PATH "archive")
set(CMAKE_LINKER       "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-ld${TOOL_OS_SUFFIX}"      CACHE PATH "linker")
set(CMAKE_NM           "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-nm${TOOL_OS_SUFFIX}"      CACHE PATH "nm")
set(CMAKE_OBJCOPY      "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-objcopy${TOOL_OS_SUFFIX}" CACHE PATH "objcopy")
set(CMAKE_OBJDUMP      "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-objdump${TOOL_OS_SUFFIX}" CACHE PATH "objdump")
set(CMAKE_RANLIB       "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-ranlib${TOOL_OS_SUFFIX}"  CACHE PATH "ranlib")

# custom /opt/devkitpro/switchvars.sh
switchvar(PORTLIBS_PREFIX PORTLIBS_PREFIX "${DEVKITPRO}/portlibs/switch")
switchvar(ARCH ARCH "-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIC -ftls-model=local-exec")
switchvar(CMAKE_C_FLAGS CFLAGS "${ARCH} -O2 -ffunction-sections -fdata-sections")
switchvar(CMAKE_CXX_FLAGS CXXFLAGS "${CMAKE_C_FLAGS}")
switchvar(CMAKE_CPP_FLAGS CPPFLAGS "-D__SWITCH__ -I${PORTLIBS_PREFIX}/include -isystem${DEVKITPRO}/libnx/include")
switchvar(CMAKE_LD_FLAGS LDFLAGS "${ARCH} -L${PORTLIBS_PREFIX}/lib -L${DEVKITPRO}/libnx/lib")
switchvar(LIBS LIBS "-lnx")



# cache flags
set( CMAKE_CXX_FLAGS           ""                        CACHE STRING "c++ flags" )
set( CMAKE_C_FLAGS             ""                        CACHE STRING "c flags" )
set( CMAKE_CXX_FLAGS_RELEASE   "-O3 -DNDEBUG"            CACHE STRING "c++ Release flags" )
set( CMAKE_C_FLAGS_RELEASE     "-O3 -DNDEBUG"            CACHE STRING "c Release flags" )
set( CMAKE_CXX_FLAGS_DEBUG     "-O0 -g -DDEBUG -D_DEBUG" CACHE STRING "c++ Debug flags" )
set( CMAKE_C_FLAGS_DEBUG       "-O0 -g -DDEBUG -D_DEBUG" CACHE STRING "c Debug flags" )
set( CMAKE_SHARED_LINKER_FLAGS ""                        CACHE STRING "shared linker flags" )
set( CMAKE_MODULE_LINKER_FLAGS ""                        CACHE STRING "module linker flags" )
set( CMAKE_EXE_LINKER_FLAGS    "-mtp=soft -fPIE -L${DEVKITPRO}/portlibs/switch/lib -L${DEVKITPRO}/libnx/lib -specs=${DEVKITPRO}/libnx/switch.specs -g"      CACHE STRING "executable linker flags" )

# we require the relocation table
set(CMAKE_C_FLAGS "-I/opt/devkitpro/libnx/include -D__SWITCH__ -march=armv8-a -mtune=cortex-a57 -mtp=soft -ffunction-sections -fdata-sections -fPIE")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti")

# switchvar(CMAKE_CXX_FLAGS CXXFLAGS "${CMAKE_C_FLAGS} -fno-rtti")
include_directories(${DEVKITPRO}/libnx/include ${PORTLIBS_PREFIX}/include)

# where is the target environment
set(CMAKE_FIND_ROOT_PATH 
	${DEVKITPRO}/devkitA64
	${DEVKITPRO}/libnx 
	${DEVKITPRO}/portlibs/switch)

# only search for libraries and includes in toolchain
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

find_program(ELF2NRO elf2nro ${DEVKITPRO}/tools/bin)
if (ELF2NRO)
	message(STATUS "elf2nro: ${ELF2NRO} - found")
else ()
    message(WARNING "elf2nro - not found")
endif ()

find_program(NACPTOOL nacptool ${DEVKITPRO}/tools/bin)
if (NACPTOOL)
    message(STATUS "nacptool: ${NACPTOOL} - found")
else ()
    message(WARNING "nacptool - not found")
endif ()

function(__add_nacp target APP_TITLE APP_AUTHOR APP_VERSION)
    set(__NACP_COMMAND ${NACPTOOL} --create ${APP_TITLE} ${APP_AUTHOR} ${APP_VERSION} ${CMAKE_CURRENT_BINARY_DIR}/${target})

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${target}
            COMMAND ${__NACP_COMMAND}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            VERBATIM
            )
endfunction()

function(add_nro_target target title author version icon romfs)
    get_filename_component(target_we ${target} NAME_WE)
        if (NOT ${target_we}.nacp)
            __add_nacp(${target_we}.nacp ${title} ${author} ${version})
        endif ()
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${target_we}.nro
            COMMAND ${ELF2NRO} $<TARGET_FILE:${target}> ${CMAKE_CURRENT_BINARY_DIR}/${target_we}.nro --icon=${icon} --nacp=${CMAKE_CURRENT_BINARY_DIR}/${target_we}.nacp
			# --romfsdir=${romfs}
            DEPENDS ${target} ${CMAKE_CURRENT_BINARY_DIR}/${target_we}.nacp
            VERBATIM
        )
        add_custom_target(${target_we}_nro ALL SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${target_we}.nro)
endfunction()


