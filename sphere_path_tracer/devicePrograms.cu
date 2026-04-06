#include <optix_device.h>
#include "LaunchParams.h"
#include <glm/glm.hpp>

using namespace spt;
namespace spt {

  extern "C" __constant__ LaunchParams optixLaunchParams;

  enum { SURFACE_RAY_TYPE=0, RAY_TYPE_COUNT };

  // Helper functions to convert between float3 (OptiX) and glm::vec3
  __forceinline__ __device__ glm::vec3 make_glm_vec3(const float3 &v) {
    return glm::vec3(v.x, v.y, v.z);
  }
  
  __forceinline__ __device__ float3 to_float3(const glm::vec3 &v) {
    return make_float3(v.x, v.y, v.z);
  }

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
  static __forceinline__ __device__ T *getRayData() {
    return reinterpret_cast<T*>(unpackPointer(optixGetPayload_0(), optixGetPayload_1()));
  }

  //implicit sphere ray-intersection shader
  extern "C" __global__ void __intersection__sphere() {
    const SphereSBTData &sbt = *(const SphereSBTData*)optixGetSbtDataPointer();

    const glm::vec3 org  = make_glm_vec3(optixGetObjectRayOrigin());
    const glm::vec3 dir  = make_glm_vec3(optixGetObjectRayDirection());
    const glm::vec3 oc   = org - sbt.center;

    const float a = glm::dot(dir, dir);
    const float b = 2.f * glm::dot(oc, dir);
    const float c = glm::dot(oc, oc) - sbt.radius * sbt.radius;
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
    RayData &rd = *(RayData*)getRayData<RayData>();

    if (sbt.emissiveStrength > 0.f) {
      rd.color += rd.attenuation * sbt.emissionColor * sbt.emissiveStrength;
      rd.isDone = true;
      return;
    }

    const glm::vec3 rayDir = make_glm_vec3(optixGetWorldRayDirection());
    const float t      = optixGetRayTmax(); // How far along the ray the intersection is, which optixTrace will automatically set to the closest hit
    const glm::vec3 hitPoint = make_glm_vec3(optixGetWorldRayOrigin()) + (t * rayDir);
    const glm::vec3 Ng = glm::normalize(hitPoint - sbt.center);

    if (sbt.transparency > 0.f) {
        // Create a new ray object for testing whats on the other side of the sphere
        RayData newRay = rd;
        
        // Create a pointer to the ray that we can pass to optixTrace
        uint32_t u0, u1;
        packPointer(&newRay, u0, u1);

        // Default values
        newRay.attenuation = glm::vec3(1.f, 1.f, 1.f);
        newRay.depth = rd.depth + 1;
        newRay.isDone = false;
        newRay.color = glm::vec3(0.f);

        // Find where the ray exits the sphere
        // Hacky method that only works when the objects are spaced apart enough
		newRay.origin = hitPoint + ((2*sbt.radius + 1e-3f) * rayDir);

        // Send out the new ray
        while (!newRay.isDone && newRay.depth < optixLaunchParams.maxDepth) {
            optixTrace(optixLaunchParams.traversable,
                to_float3(newRay.origin), to_float3(newRay.direction),
                1e-3f, 1e20f, 0.f,
                OptixVisibilityMask(255),
                OPTIX_RAY_FLAG_DISABLE_ANYHIT,
                SURFACE_RAY_TYPE, RAY_TYPE_COUNT, SURFACE_RAY_TYPE,
                u0, u1);
        }

        // Take a weighted average of the sphere's color and the ray's color
        rd.attenuation *= (newRay.color * sbt.transparency) + (sbt.color * (1.f - sbt.transparency));
    }
    else {
        rd.attenuation *= sbt.color;
    }

    rd.origin = hitPoint;
    rd.direction = glm::reflect(glm::normalize(rayDir), Ng);
    rd.depth++;
    
  

    // simple lambertian shading for now — replace with BRDF for path tracing
    //const float cosDN = fabsf(dot(rayDir, Ng));
  }

  extern "C" __global__ void __anyhit__radiance() { }

  // miss shader - a gradient
  extern "C" __global__ void __miss__radiance() {
    RayData &rd = *(RayData*)getRayData<RayData>();
    const glm::vec3 rayDir = glm::normalize(make_glm_vec3(optixGetWorldRayDirection()));
    const float t = 0.5f * (rayDir.y + 1.0f); 
    const glm::vec3 sky = (1.f - t) * glm::vec3(0.5f, 0.75f, 0.75f) + t * glm::vec3(1.0f, 0.5f, 0.5f); 
    rd.color += rd.attenuation * sky;
    rd.isDone = true;
  }

  // generate rays
  extern "C" __global__ void __raygen__renderFrame() {
    const int ix = optixGetLaunchIndex().x;
    const int iy = optixGetLaunchIndex().y;
    const auto &camera = optixLaunchParams.camera;

    RayData rd;
    rd.origin = camera.position;
    rd.attenuation = glm::vec3(1.f, 1.f, 1.f);
    rd.depth = 0;
    rd.isDone = false;
    rd.color = glm::vec3(0.f);
    const glm::vec2 screenCoord (glm::vec2(ix+.5f, iy+.5f) / glm::vec2(optixLaunchParams.frame.size));
    rd.direction = glm::normalize(camera.direction + (screenCoord.x - 0.5f) * camera.horizontal
                                              + (screenCoord.y - 0.5f) * camera.vertical);


    uint32_t u0, u1;
    packPointer(&rd, u0, u1);
    while (!rd.isDone && rd.depth < optixLaunchParams.maxDepth) {
      // https://raytracing-docs.nvidia.com/optix7/api/group__optix__device__api.html#ga11c7984d825b2a597e26a2a902386bbc
      // Initiates a ray tracing query starting with the given traversable handle and ray data.
      // The ray is traced through the scene graph until it either hits a geometry or misses the scene. 
      // When a ray hits something, the corresponding hit program is executed. 
      // If the ray misses the scene, the miss program is executed.
      optixTrace(optixLaunchParams.traversable,
                to_float3(rd.origin), to_float3(rd.direction),
                1e-3f, 1e20f, 0.f,
                OptixVisibilityMask(255),
                OPTIX_RAY_FLAG_DISABLE_ANYHIT,
                SURFACE_RAY_TYPE, RAY_TYPE_COUNT, SURFACE_RAY_TYPE,
                u0, u1);
    }
    if (!rd.isDone) {
        const glm::vec3 dir = glm::normalize(rd.direction);
        const float t   = 0.5f * (dir.y + 1.0f);
        const glm::vec3 sky = (1.f-t)*glm::vec3(0.5f,0.75f,0.75f) + t*glm::vec3(1.f,0.5f,0.5f);
        rd.color += rd.attenuation * sky;
    }

    const int r = int(255.99f * glm::clamp(rd.color.x, 0.f, 1.f));
    const int g = int(255.99f * glm::clamp(rd.color.y, 0.f, 1.f));
    const int b = int(255.99f * glm::clamp(rd.color.z, 0.f, 1.f));
    const uint32_t rgba = 0xff000000 | (r<<0) | (g<<8) | (b<<16);
    optixLaunchParams.frame.colorBuffer[ix + iy*optixLaunchParams.frame.size.x] = rgba;
  }
}