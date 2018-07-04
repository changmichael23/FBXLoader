cmake_minimum_required(VERSION 3.8)
project(OpenGLTemplate)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(Dependencies/glfw)
include_directories(Dependencies/glfw/include)

include_directories(Dependencies/glew/include)
link_directories(Dependencies/glew/lib/Release/x64)

include_directories(Dependencies/fbxsdk/include)
link_directories(Dependencies/fbxsdk/lib/vs2015/x64/debug)

include_directories(Dependencies/stb-master)

include_directories(Dependencies/glm)

file(GLOB SOURCE_FILES src/*.cpp src/*.h)
file(COPY src/shaders/ DESTINATION "${CMAKE_BINARY_DIR}")
add_executable(OpenGLTemplate ${SOURCE_FILES})

target_link_libraries(OpenGLTemplate  ${OPENGL_LIBRARIES} glfw ${GLFW_LIBRARIES} glew32s opengl32 libfbxsdk-md)