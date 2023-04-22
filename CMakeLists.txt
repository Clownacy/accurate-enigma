cmake_minimum_required(VERSION 3.21.6)

project(accurate-enigma LANGUAGES C)

add_library(decompressor STATIC
	"decompressor.c"
	"decompressor.h"
)

add_executable(decompress
	"decompress.c"
)

target_link_libraries(decompress PRIVATE decompressor)

add_library(compressor STATIC
	"compressor.c"
	"compressor.h"
)

add_executable(compress
	"compress.c"
)

target_link_libraries(compress PRIVATE compressor)
