if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(GPU_Development_Tools)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)

find_package(OptiX REQUIRED)
find_package(CUDAToolkit REQUIRED)

add_library(gdt INTERFACE)

target_link_libraries(gdt INTERFACE
  CUDA::cuda_driver
  CUDA::cudart_static
  OptiX::OptiX
)

target_include_directories(gdt INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
)


