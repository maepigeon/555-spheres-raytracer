#include <optix_device.h>
#include "LaunchParams.h"
#include "gdt/random/random.h"

using namespace osc;
namespace osc {

  extern "C" __constant__ LaunchParams optixLaunchParams;

  enum { SURFACE_RAY_TYPE=0, RAY_TYPE_COUNT };


  // Packs and unpacks 64-bit apointers into two 32-bit payload registers because optixTrace only supports 32-bit payloads.
  static __forceinline__ __device__
  void *unpackPointer(uint32_t i0, uint32_t i1) {
    const uint64_t uptr = static_cast<uint64_t>(i0) << 32 | i1;
    return reinterpret_cast<void*>(uptr);
  }
  static __forceinline__ __device__
  void packPointer(void *ptr, uint32_t &i0, uint32_t &i1) {
    const uint64_t uptr = reinterpret_cast<uint64_t>(ptr);
    i0 = uptr >> 32;
    i1 = uptr & 0x00000000ffffffff;
  }
  template<typename T>
  static __forceinline__ __device__ T *getPRD() {
    return reinterpret_cast<T*>(unpackPointer(optixGetPayload_0(), optixGetPayload_1()));
  }

  //implicit sphere ray-intersection shader
  extern "C" __global__ void __intersection__sphere() {
    const SphereSBTData &sbt = *(const SphereSBTData*)optixGetSbtDataPointer();

    const vec3f org  = optixGetObjectRayOrigin();
    const vec3f dir  = optixGetObjectRayDirection();
    const vec3f oc   = org - sbt.center;

    const float a = dot(dir, dir);
    const float b = 2.f * dot(oc, dir);
    const float c = dot(oc, oc) - sbt.radius * sbt.radius;
    const float disc = b*b - 4.f*a*c;

    if (disc < 0.f) return;

    const float sqrtDisc = sqrtf(disc);
    float t = (-b - sqrtDisc) / (2.f*a);
    if (t < optixGetRayTmin() || t > optixGetRayTmax()) {
      t = (-b + sqrtDisc) / (2.f*a);
      if (t < optixGetRayTmin() || t > optixGetRayTmax()) return;
    }
    optixReportIntersection(t, 0);
  }

  // Closest hit shader - simple lambertian shading without bounces
  // TODO: replace with path tracing logic supporting bounces and dielectric/metal materials
  extern "C" __global__ void __closesthit__radiance() {
    const SphereSBTData &sbt = *(const SphereSBTData*)optixGetSbtDataPointer();

    const vec3f rayOrg = optixGetWorldRayOrigin();
    const vec3f rayDir = optixGetWorldRayDirection();
    const float t      = optixGetRayTmax();
    const vec3f hitP   = rayOrg + t * rayDir;
    const vec3f Ng     = normalize(hitP - sbt.center);

    // simple lambertian shading for now — replace with BRDF for path tracing
    const float cosDN = 0.2f + 0.8f * fabsf(dot(rayDir, Ng));
    vec3f &prd = *(vec3f*)getPRD<vec3f>();
    prd = cosDN * sbt.color;
  }

  extern "C" __global__ void __anyhit__radiance() { }

  // miss shader - a gradient
  extern "C" __global__ void __miss__radiance() {
    const vec3f rayDir = normalize((vec3f)optixGetWorldRayDirection());
    const float t      = 0.5f * (rayDir.y + 1.0f); 
    const vec3f sky    = (1.f - t) * vec3f(0.5f, 0.75f, 0.75f) + t * vec3f(1.0f, 0.5f, 0.5f); 
    *getPRD<vec3f>() = sky;
  }

  // generate rays
  extern "C" __global__ void __raygen__renderFrame() {
    const int ix = optixGetLaunchIndex().x;
    const int iy = optixGetLaunchIndex().y;
    const auto &camera = optixLaunchParams.camera;

    vec3f pixelColor = vec3f(0.f);
    uint32_t u0, u1;
    packPointer(&pixelColor, u0, u1);

    const vec2f screen(vec2f(ix+.5f, iy+.5f)
                      / vec2f(optixLaunchParams.frame.size));

    vec3f rayDir = normalize(camera.direction
                            + (screen.x - 0.5f) * camera.horizontal
                            + (screen.y - 0.5f) * camera.vertical);
    // https://raytracing-docs.nvidia.com/optix7/api/group__optix__device__api.html#ga11c7984d825b2a597e26a2a902386bbc
    // Initiates a ray tracing query starting with the given traversable handle and ray data.
    // The ray is traced through the scene graph until it either hits a geometry or misses the scene. 
    // When a ray hits something, the corresponding hit program is executed. 
    // If the ray misses the scene, the miss program is executed.
    optixTrace(optixLaunchParams.traversable,
              camera.position, rayDir,
              0.f, 1e20f, 0.f,
              OptixVisibilityMask(255),
              OPTIX_RAY_FLAG_DISABLE_ANYHIT,
              SURFACE_RAY_TYPE, RAY_TYPE_COUNT, SURFACE_RAY_TYPE,
              u0, u1);

    const int r = int(255.99f * pixelColor.x);
    const int g = int(255.99f * pixelColor.y);
    const int b = int(255.99f * pixelColor.z);
    const uint32_t rgba = 0xff000000 | (r<<0) | (g<<8) | (b<<16);
    optixLaunchParams.frame.colorBuffer[ix + iy*optixLaunchParams.frame.size.x] = rgba;
  }
}