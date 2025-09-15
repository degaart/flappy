set(PROGRAM_NAME testDdraw)

file(GLOB SOURCE_FILES *.cpp *.c)
file(GLOB HEADER_FILES *.hpp *.h)

add_executable(${PROGRAM_NAME} WIN32 ${SOURCE_FILES} ${HEADER_FILES})
target_compile_definitions(${PROGRAM_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
target_link_libraries(${PROGRAM_NAME} PUBLIC shlwapi.lib ddraw.lib dxguid.lib)


