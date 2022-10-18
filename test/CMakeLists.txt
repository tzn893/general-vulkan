file(GLOB WINDOW_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/window/*.cpp)

file(GLOB SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/shader/*.cpp)

file(GLOB TRIANGLE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/triangle/*.cpp)
file(GLOB TRIANGLE_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/triangle/*.h)



add_executable(window-test ${WINDOW_SOURCE})
add_executable(shader-test ${SHADER_SOURCE})
add_executable(triangle ${TRIANGLE_SOURCE} ${TRIANGLE_HEADER})

target_link_libraries(window-test gvk)

add_compile_definitions(TEST_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/shader")
target_link_libraries(shader-test gvk)

add_compile_definitions(TRIANGLE_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/triangle")
add_compile_definitions(SHADER_DIRECTORY="${CMAKE_SOURCE_DIR}/src")
target_link_libraries(triangle gvk)
