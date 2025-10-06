cmake_minimum_required(VERSION 3.24..3.31)
project(game)

# use current timestamp when extracting archives with ExternalProject or FetchContent
cmake_policy(SET CMP0135 NEW)

# Set C++17 as default
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "WIN32: ${WIN32}")
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

# Disable exceptions and RTTI
if (MSVC) # or MSVC to be specific
	# turn off RTTI and Exceptions (including in STL)
	string(REGEX REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REGEX REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	add_definitions(-D_HAS_EXCEPTIONS=0) # for STL
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
endif()

# emscripten
if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX .html)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/template.html")
endif()

# Options
option(ZORRO_ENGINE "Select engine (sdl, ddraw)" "sdl")

# Validate engine
if (ZORRO_ENGINE STREQUAL "sdl")
    set(ZORRO_ENGINE_SDL ON)
elseif (ZORRO_ENGINE STREQUAL "ddraw")
    set(ZORRO_ENGINE_DDRAW ON)
endif()

# needed for SDL3
set(BUILD_SHARED_LIBS OFF)

### dependencies
include(FetchContent)
mark_as_advanced(FETCHCONTENT_BASE_DIR FETCHCONTENT_FULLY_DISCONNECTED FETCHCONTENT_QUIET FETCHCONTENT_UPDATES_DISCONNECTED)

# SDL3
if (ZORRO_ENGINE_SDL)
    FetchContent_Declare(
        sdl3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.22/SDL3-3.2.22.tar.gz
    )
    FetchContent_MakeAvailable(sdl3)
    mark_as_advanced(FETCHCONTENT_SOURCE_DIR_SDL3 FETCHCONTENT_UPDATES_DISCONNECTED_SDL3)
    mark_as_advanced(LibUSB_COMPILE_OPTIONS LibUSB_INCLUDE_PATH LibUSB_LIBRARY LibUSB_LINK_FLAGS LibUSB_LINK_LIBRARIES)
    mark_as_advanced(SDL_ALSA SDL_ASAN SDL_ASSEMBLY SDL_ASSERTIONS SDL_AUDIO SDL_AVX SDL_AVX2
        SDL_AVX512F SDL_BACKGROUNDING_SIGNAL SDL_CAMERA SDL_CCACHE SDL_CLANG_TIDY
        SDL_CLOCK_GETTIME SDL_CPU_ARM32 SDL_CPU_ARM64 SDL_CPU_ARM64EC SDL_CPU_EMSCRIPTEN
        SDL_CPU_LOONGARCH64 SDL_CPU_POWERPC32 SDL_CPU_POWERPC64 SDL_CPU_X64 SDL_CPU_X86
        SDL_DEPS_SHARED SDL_DIALOG SDL_DIRECTX SDL_DISKAUDIO SDL_DUMMYAUDIO SDL_DUMMYCAMERA
        SDL_DUMMYVIDEO SDL_EXAMPLES SDL_FOREGROUNDING_SIGNAL SDL_GCC_ATOMICS SDL_GPU SDL_GPU_DXVK
        SDL_HAPTIC SDL_HIDAPI SDL_HIDAPI_JOYSTICK SDL_HIDAPI_LIBUSB SDL_HIDAPI_LIBUSB_SHARED SDL_INSTALL
        SDL_INSTALL_CMAKEDIR_ROOT SDL_JACK SDL_JOYSTICK SDL_KMSDRM SDL_LIBC SDL_LIBICONV SDL_LIBUDEV
        SDL_MMX SDL_OFFSCREEN SDL_OPENGL SDL_OPENGLES SDL_OPENVR SDL_PIPEWIRE SDL_POWER SDL_PRESEED
        SDL_PTHREADS SDL_PULSEAUDIO SDL_RENDER SDL_RENDER_D3D SDL_RENDER_D3D11 SDL_RENDER_D3D12
        SDL_RENDER_GPU SDL_RENDER_METAL SDL_RENDER_VULKAN SDL_RPATH SDL_SENSOR SDL_SHARED SDL_SNDIO
        SDL_SSE SDL_SSE2 SDL_SSE3 SDL_SSE4_1 SDL_SSE4_2 SDL_STATIC SDL_SYSTEM_ICONV SDL_TESTS
        SDL_TESTS_TIMEOUT_MULTIPLIER SDL_TEST_LIBRARY SDL_UNINSTALL SDL_VENDOR_INFO SDL_VIDEO
        SDL_VIRTUAL_JOYSTICK SDL_VULKAN SDL_WASAPI SDL_WAYLAND SDL_WERROR SDL_X11 SDL_XINPUT)
endif()

### AssetPack
include(ExternalProject)

set(ASSETPACK_EXECUTABLE "${CMAKE_BINARY_DIR}/assetPack/assetPack${CMAKE_HOST_EXECUTABLE_SUFFIX}")
ExternalProject_Add(assetPack
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/assetPack
    BINARY_DIR ${CMAKE_BINARY_DIR}/assetPack
    CMAKE_COMMAND ${CMAKE_COMMAND}
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/assetPack
        -DCMAKE_TOOLCHAIN_FILE=
    BUILD_BYPRODUCTS "${ASSETPACK_EXECUTABLE}"
    INSTALL_COMMAND ""
)

### zorro engine
add_subdirectory(engine)

### Generate assets.hpp
set(ASSETS
    background.bmp
    gameover.bmp
    ground.bmp
    message.bmp
    numbers.bmp
    pipes.bmp
    tiles1.bmp
    die.ogg
    point.ogg
    wing.ogg
    game.pal
)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/assets.hpp
    COMMAND ${ASSETPACK_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/assets.hpp ${ASSETS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS assetPack ${ASSETS}
    COMMENT "Generating ${CMAKE_CURRENT_BINARY_DIR}/assets.hpp"
    VERBATIM
)

### executable
file(GLOB SRC_FILES *.cpp *.c)
file(GLOB HDR_FILES *.hpp *.h)
list(APPEND HDR_FILES ${CMAKE_CURRENT_BINARY_DIR}/assets.hpp)
add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES} ${HDR_FILES} resource.rc)
target_link_libraries(${PROJECT_NAME} PUBLIC zorro)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# ### executable
# file(GLOB SOURCE_FILES *.cpp *.c)
# file(GLOB HEADER_FILES *.hpp *.h)
# 
# if (ZORRO_ENGINE_SDL)
#     file(GLOB SDL_SOURCE_FILES sdl/*.cpp sdl/*.c)
#     file(GLOB SDL_HEADER_FILES sdl/*.hpp sdl/*.h)
#     list(APPEND SOURCE_FILES ${SDL_SOURCE_FILES})
#     list(APPEND HEADER_FILES ${SDL_HEADER_FILES})
# elseif (ZORRO_ENGINE_DDRAW)
#     file(GLOB DDRAW_SOURCE_FILES ddraw/*.cpp ddraw/*.c)
#     file(GLOB DDRAW_HEADER_FILES ddraw/*.hpp ddraw/*.h)
#     list(APPEND SOURCE_FILES ${DDRAW_SOURCE_FILES})
#     list(APPEND HEADER_FILES ${DDRAW_HEADER_FILES})
# endif()
# 
# add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# if (NOT MSVC)
#     target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions)
# endif()
# 
# target_link_libraries(${PROJECT_NAME}
#     PUBLIC
#         glm::glm
# )
# 
# if (WIN32)
#     target_link_libraries(${PROJECT_NAME}
#         PUBLIC
#             shlwapi.lib
#     )
# endif()

# if (ZORRO_ENGINE_SDL)
#     target_include_directories(${PROJECT_NAME}
#         PUBLIC
#             sdl
#     )
#     target_link_libraries(${PROJECT_NAME}
#         PUBLIC
#             SDL3::SDL3
#     )
# elseif (ZORRO_ENGINE_DDRAW)
#     target_include_directories(${PROJECT_NAME}
#         PUBLIC
#             ddraw
#     )
# 
#     target_link_libraries(${PROJECT_NAME}
#     PUBLIC
#         shlwapi
#         ddraw
#         dxguid
#         ole32
#     )
#     target_compile_definitions(${PROJECT_NAME}
#         PRIVATE
#             WIN32_LEAN_AND_MEAN
#     )
#     add_subdirectory(testDdraw)
# endif()




