
cmake_minimum_required(VERSION 3.10)
project(Wizard3D)
set(CMAKE_CXX_STANDARD 17)
add_subdirectory(3rdParty/glfw)
# disable ReactPhysics3D example application
set(RP3D_BUILD_TESTBED OFF CACHE BOOL "Disable building the reactphysics3d testbed")
add_subdirectory(3rdParty/reactphysics3d)
include_directories(3rdParty/glfw/include)
include_directories(3rdParty/glad/include)
include_directories(3rdParty/glm)
include_directories(3rdParty/stb)
include_directories(src)
add_library(glad 3rdParty/glad/src/glad.c
        src/engine/texture.cpp
        src/engine/texture.h)
file(GLOB SOURCES src/*.cpp src/engine/*.cpp)
add_executable(Wizard3D src/engine/scene.cpp  ${SOURCES})

add_custom_command(TARGET Wizard3D POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Wizard3D>/assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Wizard3D>/assets

        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Wizard3D>/src/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/shaders
        $<TARGET_FILE_DIR:Wizard3D>/src/shaders
)


target_link_libraries(Wizard3D glad glfw reactphysics3d ${CMAKE_DL_LIBS})

