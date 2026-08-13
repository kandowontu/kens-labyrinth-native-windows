cmake_minimum_required(VERSION 3.24)

project(KensLabyrinthNative VERSION 1.1.0 LANGUAGES C RC)

if(NOT WIN32)
    message(FATAL_ERROR "This project currently targets native Windows builds.")
endif()

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED OFF)

set(SDL12_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl12-compat")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SDL12_ARCH x64)
else()
    set(SDL12_ARCH x86)
endif()

if(NOT EXISTS "${SDL12_ROOT}/include/SDL.h")
    message(FATAL_ERROR "Bundled sdl12-compat headers are missing.")
endif()

add_library(SDL12::SDL SHARED IMPORTED GLOBAL)
set_target_properties(SDL12::SDL PROPERTIES
    IMPORTED_IMPLIB "${SDL12_ROOT}/lib/${SDL12_ARCH}/SDL.lib"
    IMPORTED_LOCATION "${SDL12_ROOT}/lib/${SDL12_ARCH}/SDL.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL12_ROOT}/include"
)

add_library(SDL12::Main STATIC IMPORTED GLOBAL)
set_target_properties(SDL12::Main PROPERTIES
    IMPORTED_LOCATION "${SDL12_ROOT}/lib/${SDL12_ARCH}/SDLmain.lib"
)

set(KEN_SOURCES
    src/lab3d/adlibemu.c
    src/lab3d/graphx.c
    src/lab3d/init.c
    src/lab3d/lab3d.c
    src/lab3d/oldlab3d.c
    src/lab3d/opengl.c
    src/lab3d/setup.c
    src/lab3d/software.c
    src/lab3d/subs.c
    src/lab3d/ken.rc
)

add_executable(kens_labyrinth WIN32 ${KEN_SOURCES})
set_target_properties(kens_labyrinth PROPERTIES
    OUTPUT_NAME "KensLabyrinth"
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:kens_labyrinth>"
)

target_include_directories(kens_labyrinth PRIVATE src/lab3d)
target_compile_definitions(kens_labyrinth PRIVATE
    WIN32
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    # The upstream 2002-2013 port intentionally uses old C idioms and many
    # narrowing conversions inherited from the 16-bit original.
    target_compile_options(kens_labyrinth PRIVATE
        /W3
        /wd4013
        /wd4047
        /wd4090
        /wd4244
        /wd4267
        /wd4305
        /wd4996
    )
endif()

target_link_libraries(kens_labyrinth PRIVATE
    SDL12::Main
    SDL12::SDL
    opengl32
    glu32
    winmm
    ole32
    shell32
    uuid
)

set(KEN_RUNTIME_FILES
    "${SDL12_ROOT}/lib/${SDL12_ARCH}/SDL.dll"
    "${SDL12_ROOT}/lib/${SDL12_ARCH}/SDL2.dll"
    "${SDL12_ROOT}/lib/${SDL12_ARCH}/SDL3.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/lab3d/ken.bmp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/lab3d/ksmmidi.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/lab3d/readme.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/lab3d/run.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/game-data/v2.1/boards.kzp"
    "${CMAKE_CURRENT_SOURCE_DIR}/game-data/v2.1/helpme.doc"
    "${CMAKE_CURRENT_SOURCE_DIR}/game-data/v2.1/hints.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/game-data/v2.1/hiscore.dat"
    "${CMAKE_CURRENT_SOURCE_DIR}/game-data/v2.1/insts.dat"
    "${CMAKE_CURRENT_SOURCE_DIR}/game-data/v2.1/lab3d.kzp"
    "${CMAKE_CURRENT_SOURCE_DIR}/game-data/v2.1/songs.kzp"
    "${CMAKE_CURRENT_SOURCE_DIR}/game-data/v2.1/sounds.kzp"
    "${CMAKE_CURRENT_SOURCE_DIR}/game-data/v2.1/story.kzp"
    "${CMAKE_CURRENT_SOURCE_DIR}/game-data/v2.1/tables.dat"
    "${CMAKE_CURRENT_SOURCE_DIR}/game-data/v2.1/walls.kzp"
)

foreach(runtime_file IN LISTS KEN_RUNTIME_FILES)
    add_custom_command(TARGET kens_labyrinth POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${runtime_file}" "$<TARGET_FILE_DIR:kens_labyrinth>"
        VERBATIM
    )
endforeach()

install(TARGETS kens_labyrinth RUNTIME DESTINATION .)
install(FILES ${KEN_RUNTIME_FILES} DESTINATION .)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/NOTICE.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md"
    DESTINATION .
)
install(FILES "${SDL12_ROOT}/LICENSE.txt" DESTINATION . RENAME "LICENSE-SDL.txt")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/" DESTINATION docs
    FILES_MATCHING PATTERN "*.md"
)
