#pragma once
#include <glm/glm.hpp>
#include "optix7.h"

namespace spt {

  // Material types for spheres in the scene
  enum MaterialType {
    MATERIAL_REFLECTIVE = 0,   // Mirror-like specular reflection
    MATERIAL_LAMBERTIAN = 1    // Diffuse Lambertian scattering
  };

  // Sphere object. SBT - Shader Binding Table - data for each sphere. 
  // This is where we can store per-sphere data that we want to access in the ray tracing shaders
  struct SphereSBTData {
    glm::vec3 color;
    glm::vec3 emissionColor;
    float emissiveStrength;
    glm::vec3 center;
    float radius;
	  float transparency;
    MaterialType materialType;
  };

  struct __align__(16) RayData {
      glm::vec3 origin;
      glm::vec3 direction;
      glm::vec3 attenuation;  // How much the original light has been filtered by surfaces
                              // that the ray has bounced on so far.
                              // This is used to accumulate the final color of the ray.
      int depth;  // Current ray bounce depth
      bool isDone;  // Whether the ray is done bouncing and thus ready for pixel color determination
      glm::vec3 color; // The final ray color that will be output
      char rngState[48];  // Cuda Random state for this ray, used for stochastic effects like diffuse scattering
  };

  // launch parameters that are passed to the gpu
  // contains camera parameters, frame buffer pointer 
  // and size, and traversable handle
  struct LaunchParams {
    struct {
      uint32_t *colorBuffer;
      glm::ivec2 size;
      int frameID;  // for accumulation later
    } frame;

    struct {
      glm::vec3 position;
      glm::vec3 direction;
      glm::vec3 horizontal;
      glm::vec3 vertical;
    } camera;

    int maxDepth;
    OptixTraversableHandle traversable;
  };

}