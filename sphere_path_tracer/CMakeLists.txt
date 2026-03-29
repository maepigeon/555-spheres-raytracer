embed_ptx(
  OUTPUT_TARGET
  spherePathTracer_devicePrograms
  PTX_LINK_LIBRARIES
  gdt
  SOURCES
  devicePrograms.cu
)

add_executable(spherePathTracer_SBTData
  optix7.h
  CUDABuffer.h
  LaunchParams.h
  SphereRenderer.h
  SphereRenderer.cpp
  main.cpp
  )

target_link_libraries(spherePathTracer_SBTData
  spherePathTracer_devicePrograms
  gdt
  glfWindow
  )
