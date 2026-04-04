#pragma once
#include "CUDABuffer.h"
#include "LaunchParams.h"
#include <glm/glm.hpp>

namespace spt {
  // Define a simple camera data type with its position, a point that the camera is
  // facing towards, and a scene up direction
  struct Camera {
    glm::vec3 position;
    glm::vec3 lookAt;
    glm::vec3 sceneUpDirection;
  };

  // Define sphere data type we can render
  struct Sphere {
    glm::vec3 center;
    float radius;
    glm::vec3 color;
    float emissiveStrength;
    glm::vec3 emissionColor;
  };

  class SampleRenderer {
  public:
    SampleRenderer(const std::vector<Sphere> &spheres);
    void render();
    void resize(const glm::ivec2 &newSize);
    void downloadPixels(uint32_t h_pixels[]);
    void setCamera(const Camera &camera);

  protected:
    void initOptix();
    void createContext();
    void createModule();
    void createRaygenPrograms();
    void createMissPrograms();
    void createHitgroupPrograms();
    void createPipeline();
    void buildSBT();
    OptixTraversableHandle buildAccel(const std::vector<Sphere> &spheres);

  protected:
    CUcontext cudaContext;
    CUstream stream;
    cudaDeviceProp deviceProps;
    OptixDeviceContext optixContext;

    OptixPipeline pipeline;
    OptixPipelineCompileOptions pipelineCompileOptions = {};
    OptixPipelineLinkOptions pipelineLinkOptions = {};

    OptixModule module;
    OptixModuleCompileOptions moduleCompileOptions = {};

    std::vector<OptixProgramGroup> raygenPGs;
    CUDABuffer raygenRecordsBuffer;
    std::vector<OptixProgramGroup> missPGs; 
    CUDABuffer missRecordsBuffer;
    std::vector<OptixProgramGroup> hitgroupPGs;
    CUDABuffer hitgroupRecordsBuffer;
    OptixShaderBindingTable sbt = {}; // shader binding table - sends data to optix shaders

    // launch params that are passed to the gpu
    // contains camera parameters, frame buffer pointer and size, and traversable handle
    LaunchParams launchParams;
    CUDABuffer launchParamsBuffer;

    CUDABuffer colorBuffer;
    Camera lastSetCamera;

    std::vector<Sphere> spheres;
    CUDABuffer asBuffer;
  };
} 