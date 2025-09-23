function(add_program PROGRAM_NAME)
    add_executable(${PROGRAM_NAME} WIN32 ${PROGRAM_NAME}.cpp)
    target_compile_definitions(${PROGRAM_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
    target_link_libraries(${PROGRAM_NAME} PUBLIC gdi32 shlwapi ddraw dxguid ole32)
    target_compile_options(${PROGRAM_NAME} PRIVATE -O3 -ffast-math -march=pentium -mtune=pentium)
endfunction()

add_program(test1)
add_program(test2)
add_program(test3)

