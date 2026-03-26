cmake_minimum_required(VERSION 3.20)
project(RayTracingInOneWeekend-Hearts)

set(CMAKE_CXX_STANDARD 23)

# Enable OpenMP (Parallel Computing) and O3 (Maximum Optimization)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

add_executable(RayTracer main.cpp)

target_include_directories(RayTracer PRIVATE include)

find_package(OpenMP REQUIRED)

add_executable(RayTracingInOneWeekend-Hearts main.cpp)

if(OpenMP_CXX_FOUND)
    target_link_libraries(RayTracingInOneWeekend-Hearts PUBLIC OpenMP::OpenMP_CXX)
endif()
