cmake_minimum_required(VERSION 3.21.6)

project(accurate-enigma LANGUAGES C)

add_library(decompressor STATIC
	"decompressor.c"
	"decompressor.h"
)

set_target_properties(decompressor PROPERTIES
	C_STANDARD 90
	C_EXTENSIONS OFF
)

add_executable(decompress
	"decompress.c"
)

set_target_properties(decompress PROPERTIES
	C_STANDARD 90
	C_EXTENSIONS OFF
)

target_link_libraries(decompress PRIVATE decompressor)

add_library(compressor STATIC
	"compressor.c"
	"compressor.h"
)

set_target_properties(compressor PROPERTIES
	C_STANDARD 90
	C_EXTENSIONS OFF
)

add_executable(compress
	"compress.c"
)

set_target_properties(compress PROPERTIES
	C_STANDARD 90
	C_EXTENSIONS OFF
)

target_link_libraries(compress PRIVATE compressor)
