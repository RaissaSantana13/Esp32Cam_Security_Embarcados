idf_component_register(
    SRCS
        "tinycbor/src/cborencoder_close_container_checked.c"
        "tinycbor/src/cborencoder.c"
        "tinycbor/src/cborencoder_float.c"
        "tinycbor/src/cborerrorstrings.c"
        "tinycbor/src/cborparser_dup_string.c"
        "tinycbor/src/cborparser.c"
        "tinycbor/src/cborparser_float.c"
        "tinycbor/src/cborpretty_stdio.c"
        "tinycbor/src/cborpretty.c"
        "tinycbor/src/cbortojson.c"
        "tinycbor/src/cborvalidation.c"

    INCLUDE_DIRS
        "tinycbor/src"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
                           WITHOUT_OPEN_MEMSTREAM)

target_compile_options(${COMPONENT_LIB} PRIVATE
                       "SHELL:-include \"${CMAKE_CURRENT_SOURCE_DIR}/port/include/unreachable_fix.h\"")