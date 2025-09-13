cmake_minimum_required(VERSION 3.24..3.31)
project(game)

# use current timestamp when extracting archives with ExternalProject or FetchContent
cmake_policy(SET CMP0135 NEW)

# emscripten
if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX .html)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/template.html")
endif()

# needed for SDL3
set(BUILD_SHARED_LIBS OFF)

### dependencies
include(FetchContent)

# fmt
FetchContent_Declare(
    fmt
    URL https://github.com/fmtlib/fmt/releases/download/11.2.0/fmt-11.2.0.zip
)
FetchContent_MakeAvailable(fmt)

# SDL3
FetchContent_Declare(
    sdl3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.22/SDL3-3.2.22.tar.gz
)
FetchContent_MakeAvailable(sdl3)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(glm)

### executable
file(GLOB SOURCE_FILES *.cpp *.c)
file(GLOB HEADER_FILES *.hpp *.h)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

if (NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions)
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        fmt
        SDL3::SDL3
        glm::glm
)

