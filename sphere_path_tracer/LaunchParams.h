#pragma once
#include "gdt/math/vec.h"
#include "optix7.h"

namespace osc {
  using namespace gdt;

  struct SphereSBTData {
    vec3f  color;
    float  roughness;
    float  reflectivity;
    vec3f  center;
    float  radius;
  };

  // launch parameters that are passed to the gpu
  // contains camera parameters, frame buffer pointer 
  // and size, and traversable handle
  struct LaunchParams {
    struct {
      uint32_t *colorBuffer;
      vec2i     size;
      int       frameID;  // for accumulation later
    } frame;

    struct {
      vec3f position;
      vec3f direction;
      vec3f horizontal;
      vec3f vertical;
    } camera;

    OptixTraversableHandle traversable;
  };

} // ::osc