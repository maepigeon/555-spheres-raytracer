#pragma once
#include <glm/glm.hpp>
#include "optix7.h"

namespace spt {

  // Completely reflective ray (Can later add roughness)
  struct SphereSBTData {
    glm::vec3 color;
    glm::vec3 emissionColor;
    float emissiveStrength;
    glm::vec3 center;
    float radius;
	float transparency;
  };

  struct RayData {
      glm::vec3 origin;
      glm::vec3 direction;
      glm::vec3 attenuation; // How much the original light has been filtered by surfaces
                        // that the ray has bounced on so far.
                        // This is used to accumulate the final color of the ray.
      int depth;  // Current ray bounce depth
      bool isDone;  // Whether the ray is done bouncing and thus ready for pixel color determination
      glm::vec3 color; // The final ray color that will be output
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