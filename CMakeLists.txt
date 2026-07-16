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

# ——— sources ———
file(GLOB_RECURSE MUPSI_SOURCES src/*.cpp src/*.hpp src/*.h)

add_executable(mupsi ${MUPSI_SOURCES})

target_include_directories(mupsi PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${EIGEN3_INCLUDE_DIR}
)

target_link_libraries(mupsi PRIVATE
    ${OpenCV_LIBS}
    OpenMP::OpenMP_CXX
)

# ——— compiler flags ———
target_compile_options(mupsi PRIVATE
    -Wall -Wextra -pedantic
    -march=native
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(mupsi PRIVATE -O3 -DNDEBUG)
else()
    target_compile_options(mupsi PRIVATE -O2 -g)
endif()
