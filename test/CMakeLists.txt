file(GLOB WINDOW_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/window/*.cpp)
file(GLOB SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/shader/*.cpp)


add_executable(window-test ${WINDOW_SOURCE})
add_executable(shader-test ${SHADER_SOURCE})

target_link_libraries(window-test gvk)

add_compile_definitions(TEST_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/shader")
target_link_libraries(shader-test gvk)