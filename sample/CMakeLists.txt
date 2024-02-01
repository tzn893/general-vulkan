add_subdirectory(common/glm)

file(GLOB COMMON_FILE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp )
file(GLOB COMMON_FILE_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/common/*.h   )

message("found common files : ${COMMON_FILE_SOURCE} ${COMMON_FILE_HEADER}")

file(GLOB RT_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/raytracer/*.cpp)

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

file(GLOB MESH_SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/mesh_shader/*.cpp)
file(GLOB MESH_SHADER_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/mesh_shader/*.h)


file(GLOB DEBUG_PRINT_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/debug_print/*.cpp)
file(GLOB DEBUG_PRINT_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/debug_print/*.h)

add_executable(window-test ${WINDOW_SOURCE} ${COMMON_FILE_SOURCE} ${COMMON_FILE_HEADER})
add_executable(shader-test ${SHADER_SOURCE} ${COMMON_FILE_SOURCE} ${COMMON_FILE_HEADER})
add_executable(triangle ${TRIANGLE_SOURCE} ${TRIANGLE_HEADER} ${COMMON_FILE_SOURCE} ${COMMON_FILE_HEADER})
add_executable(geometry ${GEOMETRY_SOURCE} ${GEOMETRY_HEADER} ${GEOMETRY_SHADER} ${COMMON_FILE_SOURCE} ${COMMON_FILE_HEADER})
add_executable(compute ${COMPUTE_SOURCE} ${COMPUTE_SHADER} ${COMMON_FILE_SOURCE} ${COMMON_FILE_HEADER})
add_executable(cube ${CUBE_SOURCE} ${CUBE_HEADER} ${COMMON_FILE_SOURCE} ${COMMON_FILE_HEADER})
add_executable(multi-subpass ${M_SUBPASS_SOURCE} ${M_SUBPASS_HEADER} ${COMMON_FILE_SOURCE} ${COMMON_FILE_HEADER})
add_executable(mesh-shader ${MESH_SHADER_SOURCE} ${MESH_SHADER_HEADER} ${COMMON_FILE_SOURCE} ${COMMON_FILE_HEADER})
add_executable(debug-print ${DEBUG_PRINT_SOURCE} ${DEBUG_PRINT_HEADER} ${COMMON_FILE_SOURCE} ${COMMON_FILE_HEADER})
add_executable(rt ${RT_SOURCE} ${COMMON_FILE_SOURCE} ${COMMON_FILE_HEADER})

target_link_libraries(window-test gvk glm)

add_compile_definitions(ASSET_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
add_compile_definitions(TEST_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/shader")
target_link_libraries(shader-test gvk glm)
target_include_directories(shader-test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_compile_definitions(TRIANGLE_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/triangle")
add_compile_definitions(SHADER_DIRECTORY="${CMAKE_SOURCE_DIR}/src")
target_link_libraries(triangle gvk glm)
target_include_directories(triangle PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_compile_definitions(GEOMETRY_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/geometry_shader")
target_link_libraries(geometry gvk glm)
target_include_directories(geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_compile_definitions(COMPUTE_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/compute")
target_link_libraries(compute gvk glm)
target_include_directories(compute PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_compile_definitions(CUBE_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/cube")
target_link_libraries(cube gvk glm)
target_include_directories(cube PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_compile_definitions(M_SUBPASS_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/multiple_subpasses")
target_link_libraries(multi-subpass gvk glm)
target_include_directories(multi-subpass PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_compile_definitions(MESH_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/mesh_shader")
target_link_libraries(mesh-shader gvk glm)
target_include_directories(mesh-shader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_compile_definitions(DEBUG_PRINT_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/debug_print")
target_link_libraries(debug-print gvk glm)
target_include_directories(debug-print PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_compile_definitions(RT_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/raytracer")
target_link_libraries(rt gvk glm)
target_include_directories(rt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)
