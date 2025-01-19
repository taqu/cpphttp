cmake_minimum_required(VERSION 3.2)

set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")

set(PROJECT_NAME cpphttp)
project(${PROJECT_NAME})

set(SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(ZLIB_ROOT "${SOURCE_ROOT}/zlib")
include_directories(AFTER "${ZLIB_ROOT}")

set(ZLIB_PUBLIC_HDRS
    ${ZLIB_ROOT}/zconf.h
    ${ZLIB_ROOT}/zlib.h
)
set(ZLIB_PRIVATE_HDRS
    ${ZLIB_ROOT}/crc32.h
    ${ZLIB_ROOT}/deflate.h
    ${ZLIB_ROOT}/gzguts.h
    ${ZLIB_ROOT}/inffast.h
    ${ZLIB_ROOT}/inffixed.h
    ${ZLIB_ROOT}/inflate.h
    ${ZLIB_ROOT}/inftrees.h
    ${ZLIB_ROOT}/trees.h
    ${ZLIB_ROOT}/zutil.h
)
set(ZLIB_SRCS
    ${ZLIB_ROOT}/adler32.c
    ${ZLIB_ROOT}/compress.c
    ${ZLIB_ROOT}/crc32.c
    ${ZLIB_ROOT}/deflate.c
    ${ZLIB_ROOT}/gzclose.c
    ${ZLIB_ROOT}/gzlib.c
    ${ZLIB_ROOT}/gzread.c
    ${ZLIB_ROOT}/gzwrite.c
    ${ZLIB_ROOT}/inflate.c
    ${ZLIB_ROOT}/infback.c
    ${ZLIB_ROOT}/inftrees.c
    ${ZLIB_ROOT}/inffast.c
    ${ZLIB_ROOT}/trees.c
    ${ZLIB_ROOT}/uncompr.c
    ${ZLIB_ROOT}/zutil.c
)

########################################################################
# Sources
set(HEADERS "../cpphttp.h;catch_amalgamated.hpp")
set(SOURCES "../cpphttp.cpp;main.cpp;test.cpp;catch_amalgamated.cpp")

source_group("include" FILES ${HEADERS})
source_group("include/zlib" FILES ${ZLIB_PUBLIC_HDRS})
source_group("include/zlib" FILES ${ZLIB_PRIVATE_HDRS})
source_group("src" FILES ${SOURCES})
source_group("src/zlib" FILES ${ZLIB_SRCS})

set(FILES ${HEADERS} ${SOURCES} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS} ${ZLIB_SRCS})

set(OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

add_executable(${PROJECT_NAME} ${FILES})

if(MSVC)
    set(DEFAULT_CXX_FLAGS "/DWIN32 /D_WINDOWS /D_MSBC /W4 /WX- /nologo /fp:precise /arch:AVX2 /Zc:wchar_t /TP /Gd /std:c++20")
    if("1800" VERSION_LESS MSVC_VERSION)
        set(DEFAULT_CXX_FLAGS "${DEFAULT_CXX_FLAGS} /EHsc")
    endif()

    set(CMAKE_CXX_FLAGS "${DEFAULT_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MDd /Zi /Ob0 /Od /RTC1 /Gy /GR- /GS /Gm-")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /GL /GR- /DNDEBUG")
    target_link_libraries(${PROJECT_NAME} Ws2_32.lib)

elseif(UNIX)
    set(DEFAULT_CXX_FLAGS "-Wall -O0 -g -std=c++20 -std=gnu++20 -march=native")
    set(CMAKE_CXX_FLAGS "${DEFAULT_CXX_FLAGS}")
elseif(APPLE)
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        OUTPUT_NAME_DEBUG "${PROJECT_NAME}" OUTPUT_NAME_RELEASE "${PROJECT_NAME}"
        VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIRECTORY}")

