file(GLOB WINDOW_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/window/*.cpp)
file(GLOB SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/shader/*.cpp)

file(GLOB TRIANGLE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/triangle/*.cpp)
file(GLOB TRIANGLE_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/triangle/*.h)

file(GLOB CUBE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/cube/*.cpp)
file(GLOB CUBE_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/cube/*.h)

file(GLOB GEOMETRY_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/geometry_shader/*.cpp)
file(GLOB GEOMETRY_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/geometry_shader/*.h)
file(GLOB GEOMETRY_SHADER ${CMAKE_CURRENT_SOURCE_DIR}/geometry_shader/geom.*)

file(GLOB COMPUTE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compute/*.cpp)
file(GLOB COMPUTE_SHADER ${CMAKE_CURRENT_SOURCE_DIR}/compute/julia.*)

file(GLOB M_SUBPASS_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/multiple_subpasses/*.cpp)
file(GLOB M_SUBPASS_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/multiple_subpasses/*.h)



add_executable(window-test ${WINDOW_SOURCE})
add_executable(shader-test ${SHADER_SOURCE})
add_executable(triangle ${TRIANGLE_SOURCE} ${TRIANGLE_HEADER})
add_executable(geometry ${GEOMETRY_SOURCE} ${GEOMETRY_HEADER} ${GEOMETRY_SHADER})
add_executable(compute ${COMPUTE_SOURCE} ${COMPUTE_SHADER})
add_executable(cube ${CUBE_SOURCE} ${CUBE_HEADER})
add_executable(multi-subpass ${M_SUBPASS_SOURCE} ${M_SUBPASS_HEADER})

target_link_libraries(window-test gvk)

add_compile_definitions(TEST_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/shader")
target_link_libraries(shader-test gvk)

add_compile_definitions(TRIANGLE_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/triangle")
add_compile_definitions(SHADER_DIRECTORY="${CMAKE_SOURCE_DIR}/src")
target_link_libraries(triangle gvk)

add_compile_definitions(GEOMETRY_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/geometry_shader")
target_link_libraries(geometry gvk)

add_compile_definitions(COMPUTE_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/compute")
target_link_libraries(compute gvk)

add_compile_definitions(CUBE_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/cube")
target_link_libraries(cube gvk)

add_compile_definitions(M_SUBPASS_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/multiple_subpasses")
target_link_libraries(multi-subpass gvk)