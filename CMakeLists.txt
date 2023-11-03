cmake_minimum_required(VERSION 3.26)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")

project(cmw-coder-loader CXX)

add_executable(${PROJECT_NAME} main.cpp)

find_path(DETOURS_INCLUDE_DIRS "detours/detours.h")
find_library(DETOURS_LIBRARY detours REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${DETOURS_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DETOURS_LIBRARY})