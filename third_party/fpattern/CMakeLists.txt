include(FetchContent)

FetchContent_Declare(fpattern
    GIT_REPOSITORY "https://github.com/Loadmaster/fpattern"
    GIT_TAG "v1.9"
)

FetchContent_GetProperties(fpattern)
if (NOT fpattern_POPULATED)
    FetchContent_Populate(fpattern)
endif()

add_library(fpattern STATIC
    "${fpattern_SOURCE_DIR}/debug.h"
    "${fpattern_SOURCE_DIR}/fpattern.c"
    "${fpattern_SOURCE_DIR}/fpattern.h"
)

set(FPATTERN_LIBRARY "fpattern" PARENT_SCOPE)
set(FPATTERN_INCLUDE_DIR "${fpattern_SOURCE_DIR}" PARENT_SCOPE)
