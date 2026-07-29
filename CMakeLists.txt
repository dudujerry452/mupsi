cmake_minimum_required(VERSION 3.16)
project(mupsi VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ——— dependencies ———
find_package(Eigen3 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(OpenMP REQUIRED)
find_package(nlohmann_json REQUIRED)

# ——— core library ———
file(GLOB_RECURSE CORE_SOURCES src/*.cpp src/*.hpp src/*.h)
list(FILTER CORE_SOURCES EXCLUDE REGEX "src/main\\.cpp")
list(FILTER CORE_SOURCES EXCLUDE REGEX "src/editor/.*")
list(FILTER CORE_SOURCES EXCLUDE REGEX "src/thirdparty/imgui/.*")
list(FILTER CORE_SOURCES EXCLUDE REGEX "src/thirdparty/imgui/backends/.*")

add_library(mupsi_core STATIC ${CORE_SOURCES})

target_include_directories(mupsi_core PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(mupsi_core PUBLIC
    ${OpenCV_LIBS}
    OpenMP::OpenMP_CXX
    nlohmann_json::nlohmann_json
    Eigen3::Eigen
)

target_compile_options(mupsi_core PRIVATE
    -Wall -Wextra -pedantic
    -march=native
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(mupsi_core PRIVATE -O3 -DNDEBUG)
else()
    target_compile_options(mupsi_core PRIVATE -O2 -g)
endif()

# ——— headless CLI ———
add_executable(mupsi src/main.cpp)
target_include_directories(mupsi PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(mupsi PRIVATE mupsi_core)
target_compile_options(mupsi PRIVATE -Wall -Wextra -pedantic -O2 -g)

# ——— editor (optional) ———
find_package(OpenGL QUIET)
find_package(glfw3 QUIET)

if(OpenGL_FOUND AND glfw3_FOUND)
    file(GLOB_RECURSE IMGUI_SOURCES src/thirdparty/imgui/*.cpp)
    file(GLOB EDITOR_SOURCES src/editor/*.cpp src/editor/*.h)

    add_executable(mupsi-editor ${EDITOR_SOURCES} ${IMGUI_SOURCES})
    target_include_directories(mupsi-editor PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/thirdparty/imgui
        ${OPENGL_INCLUDE_DIR}
    )
    target_link_libraries(mupsi-editor PRIVATE
        mupsi_core
        glfw
        ${OPENGL_LIBRARIES}
    )
    target_compile_options(mupsi-editor PRIVATE -Wall -Wextra -pedantic -O2 -g)
    target_compile_definitions(mupsi-editor PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)
    message(STATUS "Building editor (GLFW + OpenGL found)")
else()
    message(STATUS "Editor disabled (GLFW or OpenGL not found)")
endif()

