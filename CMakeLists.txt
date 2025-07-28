cmake_minimum_required(VERSION 3.10)
project(LightingCube)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(3rdParty/glfw)
include_directories(3rdParty/glfw/include)
include_directories(3rdParty/glad/include)
include_directories(3rdParty/glm)
include_directories(3rdParty/stb)
include_directories(src)

add_library(glad 3rdParty/glad/src/glad.c)

file(GLOB SOURCES src/*.cpp)
add_executable(LightingCube ${SOURCES})
target_link_libraries(LightingCube glad glfw ${CMAKE_DL_LIBS})
