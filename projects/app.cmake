# SPDX-License-Identifier: MIT

include(${CMAKE_SOURCE_DIR}/projects/arch/cm4f.cmake)

set(src-dirs src)
foreach(dir ${src-dirs})
	file(GLOB_RECURSE ${dir}_SRCS RELATIVE ${CMAKE_SOURCE_DIR} ${dir}/*.c)
	file(GLOB_RECURSE ${dir}_CPP_SRCS RELATIVE ${CMAKE_SOURCE_DIR} ${dir}/*.cpp)
	list(APPEND SRCS_TMP ${${dir}_SRCS} ${${dir}_CPP_SRCS})
endforeach()

set(APP_SRCS
	${SRCS_TMP}
	${CMAKE_SOURCE_DIR}/external/libmcu/ports/armcm/fault.c
	${CMAKE_SOURCE_DIR}/external/libmcu/ports/armcm/assert.c
)
set(APP_INCS
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/external/libmcu/ports/armcm/include
)
set(APP_DEFS
	${PROJECT}
	BUILD_DATE=${BUILD_DATE}
	VERSION_TAG=${VERSION_TAG}
	VERSION=${VERSION}

	_POSIX_THREADS
	_POSIX_C_SOURCE=200809L
)
set(TARGET_PLATFORM nrf52)
set(PLATFORM_SPECIFIC ${CMAKE_SOURCE_DIR}/ports/${TARGET_PLATFORM})
set(LD_SCRIPT ${PLATFORM_SPECIFIC}/nrf52840.ld)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include libmcu/assert.h")
set(elf_file ${PROJECT_NAME}.elf)
add_executable(${elf_file} ${APP_SRCS})
target_include_directories(${elf_file} PRIVATE ${APP_INCS})
target_compile_definitions(${elf_file} PRIVATE ${APP_DEFS})
target_link_options(${elf_file} PRIVATE -T${LD_SCRIPT})
target_link_libraries(${elf_file} PRIVATE
	-Wl,--cref
	-Wl,--Map=\"${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map\"

	${TARGET_PLATFORM}
	rtt
)

# Third Party
add_subdirectory(external/libmcu)
target_compile_definitions(libmcu PUBLIC
	METRICS_USER_DEFINES=\"${PROJECT_SOURCE_DIR}/include/metrics.def\"
	${APP_DEFS}
)
target_include_directories(libmcu PUBLIC
	${CMAKE_SOURCE_DIR}/external/libmcu/modules/common/include/libmcu/posix)

add_subdirectory(ports/rtt)
target_link_libraries(rtt PUBLIC libmcu)

# Platform Specific
add_subdirectory(${PLATFORM_SPECIFIC})
target_link_libraries(${TARGET_PLATFORM} PUBLIC libmcu)

add_custom_target(${PROJECT_NAME}.bin ALL DEPENDS ${elf_file})
add_custom_target(${PROJECT_NAME}.hex ALL DEPENDS ${elf_file})
add_custom_target(flash DEPENDS ${PROJECT_NAME}.hex)
add_custom_target(flash_softdevice)
add_custom_target(flash_usb DEPENDS ${PROJECT_NAME}.bin)
add_custom_target(gdb DEPENDS ${elf_file})

add_custom_command(TARGET ${PROJECT_NAME}.hex
	COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${elf_file}>
			${PROJECT_NAME}.hex
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_command(TARGET ${PROJECT_NAME}.bin
	COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${elf_file}>
			${PROJECT_NAME}.bin
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_command(TARGET flash_usb
	USES_TERMINAL COMMAND
		dfu-util --device 1209:e001 --alt 0 --download ${PROJECT_NAME}.bin
)

add_custom_command(TARGET flash
	USES_TERMINAL
	COMMAND
		nrfjprog -f nrf52 --program ${PROJECT_NAME}.hex --sectorerase --verify
	COMMAND
		nrfjprog -f nrf52 --reset
)

add_custom_command(TARGET flash_softdevice
	USES_TERMINAL
	COMMAND
		nrfjprog -f nrf52 --program ${CMAKE_SOURCE_DIR}/external/nRF5_SDK_17.1.0_ddde560/components/softdevice/s140/hex/s140_nrf52_7.2.0_softdevice.hex --sectorerase --verify
)

add_custom_command(TARGET gdb
	USES_TERMINAL COMMAND
		pyocd gdbserver -t nrf52840
)
