file(GLOB WINDOW_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/window/*.cpp)

add_executable(window ${WINDOW_SOURCE})

target_link_libraries(window gvk)