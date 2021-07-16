# BuildVeyonPlugin.cmake - Copyright (c) 2017-2021 Tobias Junghans
#
# description: build Veyon plugin
# usage: build_hamonize_plugin(<NAME> <SOURCES>)

MACRO(build_veyon_plugin PLUGIN_NAME)
	ADD_LIBRARY(${PLUGIN_NAME} MODULE ${ARGN})

	TARGET_INCLUDE_DIRECTORIES(${PLUGIN_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
	TARGET_LINK_LIBRARIES(${PLUGIN_NAME} hamonize-core)

	SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES PREFIX "")
	SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES LINK_FLAGS "-Wl,-no-undefined")
	INSTALL(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION ${VEYON_INSTALL_PLUGIN_DIR})

	set_default_target_properties(${PLUGIN_NAME})

	if(WITH_PCH)
		target_precompile_headers(${PLUGIN_NAME} REUSE_FROM veyon-pch)
	endif()
ENDMACRO()

