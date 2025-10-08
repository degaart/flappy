file(GLOB SRC_FILES *.c *.cpp)
file(GLOB HDR_FILES *.h *.hpp)

if (ZORRO_ENGINE_DDRAW)
    file(GLOB DDRAW_SRC_FILES ddraw/*.c ddraw/*.cpp)
    file(GLOB DDRAW_HDR_FILES ddraw/*.h ddraw/*.hpp)
    list(APPEND SRC_FILES ${DDRAW_SRC_FILES})
    list(APPEND HDR_FILES ${DDRAW_HDR_FILES})
endif()

add_library(zorro ${SRC_FILES} ${HDR_FILES})

file(REAL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/.. INCLUDE_DIRS)
target_include_directories(zorro PUBLIC ${INCLUDE_DIRS})
target_compile_definitions(zorro PUBLIC WIN32_LEAN_AND_MEAN DIRECTSOUND_VERSION=0x0600)

if (ZORRO_ENGINE_DDRAW)
    target_link_libraries(zorro
        PUBLIC
            ddraw
            dxguid
            dsound
            shlwapi
            ole32
            gdi32
    )

    if (NOT MSVC)
        target_link_options(zorro INTERFACE "-Wl,--undefined=_WinMain@16")
    endif()

endif()

