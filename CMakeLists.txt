cmake_minimum_required(VERSION 3.1)

# Project name & version (MAJOR.MINOR-RELTYPE)
project (tumble)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

SET(TUMBLE_VERSION "0.38")

if(CTL_LANG)
	find_package(BISON)
	find_package(FLEX)

	if(NOT BISON_FOUND)
		MESSAGE(FATAL_ERROR "Bison not found!")
	endif(NOT BISON_FOUND)
	if(NOT FLEX_FOUND)
		MESSAGE(FATAL_ERROR "Flex not found!")
	endif(NOT FLEX_FOUND)
endif(CTL_LANG)

find_package(TIFF)
find_package(NETPBM)
find_package(JPEG)

if(NOT TIFF_FOUND)
	MESSAGE(FATAL_ERROR "libTIFF not found!")
endif()
if(NOT NETPBM_FOUND)
	MESSAGE(FATAL_ERROR "NETPBM not found!")
endif()
if(NOT JPEG_FOUND)
	MESSAGE(FATAL_ERROR "libJPEG not found!")
endif()


add_executable(bitblt_table_gen bitblt_table_gen.c)
add_custom_command(OUTPUT bitblt_tables.c bitblt_tables.h
	COMMAND ${CMAKE_SOURCE_DIR}/build_bitblt_tables.sh ${CMAKE_BINARY_DIR}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	DEPENDS bitblt_table_gen)

add_executable(g4_table_gen g4_table_gen.c)
add_custom_command(OUTPUT g4_tables.c g4_tables.h
	COMMAND ${CMAKE_SOURCE_DIR}/build_g4_tables.sh ${CMAKE_BINARY_DIR}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	DEPENDS g4_table_gen)

set(TUMBLE_SRCS tumble.c semantics.c tumble_input.c
	tumble_tiff.c tumble_jpeg.c tumble_pbm.c tumble_png.c tumble_blank.c
	bitblt.c bitblt_g4.c bitblt_tables.c g4_tables.c
	pdf.c pdf_util.c pdf_prim.c pdf_name_tree.c
	pdf_bookmark.c pdf_page_label.c pdf_text.c pdf_g4.c pdf_jpeg.c pdf_png.c)

if(CTL_LANG)
	BISON_TARGET(CtlParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.c)
	FLEX_TARGET(CtlScanner scanner.l ${CMAKE_CURRENT_BINARY_DIR}/scanner.c)
	ADD_FLEX_BISON_DEPENDENCY(CtlScanner CtlParser)

	list(APPEND TUMBLE_SRCS ${BISON_CtlParser_OUTPUTS} ${FLEX_CtlScanner_OUTPUTS})
endif(CTL_LANG)

add_executable(tumble ${TUMBLE_SRCS})
target_include_directories(tumble PUBLIC ${TIFF_INCLUDE_DIR} ${NETPBM_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(tumble ${TIFF_LIBRARY_RELEASE} ${NETPBM_LIBRARY} ${JPEG_LIBRARY})
target_compile_definitions(tumble PUBLIC TUMBLE_VERSION=${TUMBLE_VERSION})

if(CTL_LANG)
	target_compile_definitions(tumble PUBLIC CTL_LANG)
endif(CTL_LANG)
