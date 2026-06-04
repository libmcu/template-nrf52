# SPDX-License-Identifier: MIT

function(madi_idf_register_local_components)
	file(GLOB local_component_cmakelists CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/components/*/CMakeLists.txt"
	)

	foreach(component_cmakelist IN LISTS local_component_cmakelists)
		get_filename_component(component_dir "${component_cmakelist}" DIRECTORY)
		idf_build_component("${component_dir}")
	endforeach()
endfunction()

function(madi_idf_link_build_components target)
	idf_build_get_property(build_components BUILD_COMPONENTS)

	foreach(component IN LISTS build_components)
		if (TARGET idf::${component})
			target_link_libraries(${target} idf::${component})
		endif()
	endforeach()
endfunction()

# Enable component manager in the freestanding idf_build_process flow.
set(ENV{IDF_TARGET} ${IDF_TARGET})
idf_build_set_property(IDF_COMPONENT_MANAGER 1)
idf_build_set_property(__COMPONENT_MANAGER_INTERFACE_VERSION 4)
idf_build_set_property(DEPENDENCIES_LOCK "${CMAKE_BINARY_DIR}/dependencies.lock")

madi_idf_register_local_components()

idf_build_process(${IDF_TARGET}
	COMPONENTS
		madi_idf_deps
	PROJECT_VER
		"${PROJECT_VER}"
	SDKCONFIG_DEFAULTS
		"${CMAKE_CURRENT_LIST_DIR}/boards/${TARGET_PLATFORM}/sdkconfig.defaults"
	BUILD_DIR
		${CMAKE_CURRENT_BINARY_DIR}
)

set(mapfile "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map")
# project_description.json metadata file used for the flash and the monitor of
# idf.py to get the project information.
set(build_components_json "[]")
set(build_component_paths_json "[]")
set(common_component_reqs_json "\"\"")
set(build_component_info_json "\"\"")
set(all_component_info_json "\"\"")
configure_file("${IDF_PATH}/tools/cmake/project_description.json.in"
	"${CMAKE_CURRENT_BINARY_DIR}/project_description.json")

idf_build_set_property(COMPILE_DEFINITIONS -DxPortIsInsideInterrupt=xPortInIsrContext APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Wno-implicit-function-declaration" APPEND)

madi_idf_link_build_components(${PROJECT_EXECUTABLE})
target_link_options(${PROJECT_EXECUTABLE}
	PRIVATE
		"-Wl,--cref"
		"-Wl,--Map=${mapfile}"
)

if(NOT Python3_EXECUTABLE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()
set(idf_size ${Python3_EXECUTABLE} $ENV{IDF_PATH}/tools/idf_size.py)

add_custom_target(size DEPENDS ${mapfile} COMMAND ${idf_size} ${mapfile})
add_custom_target(size-files DEPENDS ${mapfile} COMMAND ${idf_size} --files ${mapfile})
add_custom_target(size-components DEPENDS ${mapfile} COMMAND ${idf_size} --archives ${mapfile})

# Attach additional targets to the executable file for flashing,
# linker script generation, partition_table generation, etc.
idf_build_executable(${PROJECT_EXECUTABLE})
