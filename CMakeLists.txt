
cmake_minimum_required(VERSION 4.3)
project(sphere_path_tracer VERSION 1.0.2 LANGUAGES C CXX CUDA)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_ARCHITECTURES native)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
  "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

# ------------------------------------------------------------------
# first, include gdt project to do some general configuration stuff
# (build modes, glut, optix, etc)
# ------------------------------------------------------------------
set(link-cuda-tools-dir ${PROJECT_SOURCE_DIR}/include/link-cuda-tools/)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${link-cuda-tools-dir}/cmake/")
include(${link-cuda-tools-dir}/cmake/configure_build_type.cmake)
include(${link-cuda-tools-dir}/cmake/configure_optix.cmake)

#set(glfw_dir ${PROJECT_SOURCE_DIR}/submodules/glfw/)
#include(${gdt_dir}/cmake/configure_glfw.cmake)

mark_as_advanced(CUDA_SDK_ROOT_DIR)

# import link-cuda-tools submodule
include_directories(${link-cuda-tools-dir})
add_subdirectory(${link-cuda-tools-dir} EXCLUDE_FROM_ALL)

# include GLM
set(GLM_HEADER_ONLY ON)
include_directories(${PROJECT_SOURCE_DIR}/include/3rdParty)

# build glfw
set(OpenGL_GL_PREFERENCE LEGACY)
if (WIN32)
  set(glfw_dir ${PROJECT_SOURCE_DIR}/include/3rdParty/glfw/)
  include_directories(${glfw_dir}/include)
  add_subdirectory(${glfw_dir} EXCLUDE_FROM_ALL)
else()
  find_package(glfw3 REQUIRED)
endif()
include_directories(include)
add_subdirectory(include/glfWindow EXCLUDE_FROM_ALL)


# ------------------------------------------------------------------
# and final build rules for the project
# ------------------------------------------------------------------

# creates context and sets up an entire optix 7 pipeline, renders a
# sphere,to a glfwindow with continuous rendering
add_subdirectory(sphere_path_tracer)
