#pragma once
#include "gdt/math/vec.h"
#include "optix7.h"

namespace spt {
  using namespace gdt;

  // Completely reflective ray (Can later add roughness)
  struct SphereSBTData {
    vec3f color;
    vec3f emissionColor;
    float emissiveStrength;
    vec3f center;
    float radius;
  };

  struct RayData {
      vec3f origin;
      vec3f direction;
      vec3f attenuation; // How much the original light has been filtered by surfaces
                        // that the ray has bounced on so far.
                        // This is used to accumulate the final color of the ray.
      int depth;  // Current ray bounce depth
      bool isDone;  // Whether the ray is done bouncing and thus ready for pixel color determination
      vec3f color; // The final ray color that will be output
  };

  // launch parameters that are passed to the gpu
  // contains camera parameters, frame buffer pointer 
  // and size, and traversable handle
  struct LaunchParams {
    struct {
      uint32_t *colorBuffer;
      vec2i size;
      int frameID;  // for accumulation later
    } frame;

    struct {
      vec3f position;
      vec3f direction;
      vec3f horizontal;
      vec3f vertical;
    } camera;

    int maxDepth;
    OptixTraversableHandle traversable;
  };

}