#include "SphereRenderer.h"
#include <optix_function_table_definition.h>
#include <glm/glm.hpp>
#include <iostream>
#include <cstdio>

namespace spt {

extern "C" char devicePrograms_ptx[];

struct __align__(OPTIX_SBT_RECORD_ALIGNMENT) RaygenRecord {
  __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
  void *data;
};
struct __align__(OPTIX_SBT_RECORD_ALIGNMENT) MissRecord {
  __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
  void *data;
};
struct __align__(OPTIX_SBT_RECORD_ALIGNMENT) HitgroupRecord {
  __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
  SphereSBTData data;
};

SampleRenderer::SampleRenderer(const std::vector<Sphere> &spheres)
  : spheres(spheres)
{
  initOptix();
  std::cout << "creating optix context" << std::endl;
  createContext();
  std::cout << "setting up optix module" << std::endl;
  createModule();
  std::cout << "loading ray generation programs" << std::endl;
  createRaygenPrograms(); 
  createMissPrograms();
  createHitgroupPrograms();
  launchParams.traversable = buildAccel(spheres);
  std::cout << "setting up the render pipeline" << std::endl;
  createPipeline();
  std::cout << "building SBT" << std::endl;
  buildSBT();
  launchParamsBuffer.alloc(sizeof(launchParams));
  launchParams.airRefractiveIndex = 1.f;
  launchParams.maxDepth = 8;
  std::cout << "fully set up" << std::endl;
}

OptixTraversableHandle SampleRenderer::buildAccel(const std::vector<Sphere> &spheres)
{
  std::vector<OptixAabb> aabbs;
  for (const Sphere& s : spheres) {
    OptixAabb aabb;
    aabb.minX = s.center.x - s.radius;
    aabb.minY = s.center.y - s.radius;
    aabb.minZ = s.center.z - s.radius;
    aabb.maxX = s.center.x + s.radius;
    aabb.maxY = s.center.y + s.radius;
    aabb.maxZ = s.center.z + s.radius;
    aabbs.push_back(aabb);
  }

  CUDABuffer aabbBuffer;
  aabbBuffer.alloc_and_upload(aabbs);

  std::vector<uint32_t> sbtIndices(spheres.size());
  for (uint32_t i = 0; i < spheres.size(); i++) sbtIndices[i] = i;
  CUDABuffer sbtIndexBuffer;
  sbtIndexBuffer.alloc_and_upload(sbtIndices);

  OptixBuildInput customInput = {};
  customInput.type = OPTIX_BUILD_INPUT_TYPE_CUSTOM_PRIMITIVES;
  CUdeviceptr d_aabbs = aabbBuffer.d_pointer();
  customInput.customPrimitiveArray.aabbBuffers   = &d_aabbs;
  customInput.customPrimitiveArray.numPrimitives = (uint32_t)spheres.size();

  std::vector<uint32_t> flags(spheres.size(), OPTIX_GEOMETRY_FLAG_NONE);
  customInput.customPrimitiveArray.flags         = flags.data();
  customInput.customPrimitiveArray.numSbtRecords               = (uint32_t)spheres.size();
  customInput.customPrimitiveArray.sbtIndexOffsetBuffer        = 0; // we'll use a per-prim index buffer
  customInput.customPrimitiveArray.sbtIndexOffsetSizeInBytes   = sizeof(uint32_t);
  customInput.customPrimitiveArray.sbtIndexOffsetStrideInBytes = sizeof(uint32_t);

  customInput.customPrimitiveArray.sbtIndexOffsetBuffer = sbtIndexBuffer.d_pointer();

  OptixAccelBuildOptions accelOptions = {};
  accelOptions.buildFlags = OPTIX_BUILD_FLAG_ALLOW_COMPACTION;
  accelOptions.operation  = OPTIX_BUILD_OPERATION_BUILD;
  accelOptions.motionOptions.numKeys = 1;

  OptixAccelBufferSizes sizes;
  OPTIX_CHECK(optixAccelComputeMemoryUsage(optixContext, &accelOptions,
                                            &customInput, 1, &sizes));

  CUDABuffer tempBuffer, outputBuffer, compactedSizeBuffer;
  tempBuffer.alloc(sizes.tempSizeInBytes);
  outputBuffer.alloc(sizes.outputSizeInBytes);
  compactedSizeBuffer.alloc(sizeof(uint64_t));

  OptixAccelEmitDesc emitDesc;
  emitDesc.type   = OPTIX_PROPERTY_TYPE_COMPACTED_SIZE;
  emitDesc.result = compactedSizeBuffer.d_pointer();

  OptixTraversableHandle asHandle;
  OPTIX_CHECK(optixAccelBuild(optixContext, 0,
                               &accelOptions, &customInput, 1,
                               tempBuffer.d_pointer(), tempBuffer.sizeInBytes,
                               outputBuffer.d_pointer(), outputBuffer.sizeInBytes,
                               &asHandle, &emitDesc, 1));
  CUDA_SYNC_CHECK();

  uint64_t compactedSize;
  compactedSizeBuffer.download(&compactedSize, 1);
  if (asBuffer.d_ptr) asBuffer.free();  // Free old allocation if it exists
  asBuffer.alloc(compactedSize);
  OPTIX_CHECK(optixAccelCompact(optixContext, 0, asHandle,
                                 asBuffer.d_pointer(), asBuffer.sizeInBytes,
                                 &asHandle));
  CUDA_SYNC_CHECK();

  sbtIndexBuffer.free();
  outputBuffer.free();
  tempBuffer.free();
  compactedSizeBuffer.free();
  aabbBuffer.free();

  return asHandle;
}

void SampleRenderer::initOptix()
{
  std::cout << "#osc: initializing optix..." << std::endl;
  cudaFree(0);
  int numDevices;
  cudaGetDeviceCount(&numDevices);
  if (numDevices == 0)
    throw std::runtime_error("#osc: no CUDA capable devices found!");
  std::cout << "#osc: found " << numDevices << " CUDA devices" << std::endl;
  OPTIX_CHECK(optixInit());
  std::cout << "#osc: successfully initialized optix... yay!" << std::endl;
}

static void context_log_cb(unsigned int level, const char *tag,
                            const char *message, void *)
{
  fprintf(stderr, "[%2d][%12s]: %s\n", (int)level, tag, message);
}

void SampleRenderer::createContext()
{
  const int deviceID = 0;
  CUDA_CHECK(SetDevice(deviceID));
  CUDA_CHECK(StreamCreate(&stream));
  cudaGetDeviceProperties(&deviceProps, deviceID);
  std::cout << "#osc: running on device: " << deviceProps.name << std::endl;

  CUresult cuRes = cuCtxGetCurrent(&cudaContext);
  if (cuRes != CUDA_SUCCESS)
    fprintf(stderr, "Error querying current context: error code %d\n", cuRes);

  OPTIX_CHECK(optixDeviceContextCreate(cudaContext, 0, &optixContext));
  OPTIX_CHECK(optixDeviceContextSetLogCallback(optixContext, context_log_cb,
                                                nullptr, 4));
}

void SampleRenderer::createModule()
{
  moduleCompileOptions.maxRegisterCount = 50;
  moduleCompileOptions.optLevel         = OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
  moduleCompileOptions.debugLevel       = OPTIX_COMPILE_DEBUG_LEVEL_NONE;

  pipelineCompileOptions = {};
  pipelineCompileOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
  pipelineCompileOptions.usesMotionBlur        = false;
  pipelineCompileOptions.numPayloadValues      = 2;
  pipelineCompileOptions.numAttributeValues    = 2;
  pipelineCompileOptions.exceptionFlags        = OPTIX_EXCEPTION_FLAG_NONE;
  pipelineCompileOptions.pipelineLaunchParamsVariableName = "optixLaunchParams";

  pipelineLinkOptions.maxTraceDepth = 2;

  const std::string ptxCode = devicePrograms_ptx;
  char log[2048];
  size_t sizeof_log = sizeof(log);
  OPTIX_CHECK(optixModuleCreate(optixContext,
                              &moduleCompileOptions,
                              &pipelineCompileOptions,
                              ptxCode.c_str(), ptxCode.size(),
                              log, &sizeof_log, &module));
  if (sizeof_log > 1) printf("%s\n", log);    
}

void SampleRenderer::createRaygenPrograms() {
  raygenPGs.resize(1);
  OptixProgramGroupOptions pgOptions = {};
  OptixProgramGroupDesc pgDesc       = {};
  pgDesc.kind                        = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
  pgDesc.raygen.module               = module;
  pgDesc.raygen.entryFunctionName    = "__raygen__renderFrame";

  char log[2048];
  size_t sizeof_log = sizeof(log);
  OPTIX_CHECK(optixProgramGroupCreate(optixContext, &pgDesc, 1, &pgOptions,
                                       log, &sizeof_log, &raygenPGs[0]));
  if (sizeof_log > 1) printf("%s\n", log);
}

// If the ray misses, return a color based on the ray direction to create a gradient background
void SampleRenderer::createMissPrograms() {
  missPGs.resize(1);
  OptixProgramGroupOptions pgOptions = {};
  OptixProgramGroupDesc pgDesc       = {};
  pgDesc.kind                        = OPTIX_PROGRAM_GROUP_KIND_MISS;
  pgDesc.miss.module                 = module;
  pgDesc.miss.entryFunctionName      = "__miss__radiance";

  char log[2048];
  size_t sizeof_log = sizeof(log);
  OPTIX_CHECK(optixProgramGroupCreate(optixContext, &pgDesc, 1, &pgOptions,
                                       log, &sizeof_log, &missPGs[0]));
  if (sizeof_log > 1) printf("%s\n", log);
}

// Send optix the program descriptor for intersection and hit shaders
// Optix uses this to set up the shader binding table (SBT) which is how data gets passed to shaders
void SampleRenderer::createHitgroupPrograms() {
  hitgroupPGs.resize(1);
  OptixProgramGroupOptions pgOptions = {};
  OptixProgramGroupDesc pgDesc       = {};
  pgDesc.kind                                     = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
  pgDesc.hitgroup.moduleCH                        = module;
  pgDesc.hitgroup.entryFunctionNameCH             = "__closesthit__radiance";
  pgDesc.hitgroup.moduleAH                        = module;
  pgDesc.hitgroup.entryFunctionNameAH             = "__anyhit__radiance";
  pgDesc.hitgroup.moduleIS                        = module;
  pgDesc.hitgroup.entryFunctionNameIS             = "__intersection__sphere";

  char log[2048];
  size_t sizeof_log = sizeof(log);
  OPTIX_CHECK(optixProgramGroupCreate(optixContext, &pgDesc, 1, &pgOptions,
                                       log, &sizeof_log, &hitgroupPGs[0]));
  if (sizeof_log > 1) printf("%s\n", log);
}

// Creates an optix pipiline with the raygen, miss, and hitgroup programs
void SampleRenderer::createPipeline() {
  std::vector<OptixProgramGroup> programGroups;
  for (auto pg : raygenPGs)   programGroups.push_back(pg);
  for (auto pg : missPGs)     programGroups.push_back(pg);
  for (auto pg : hitgroupPGs) programGroups.push_back(pg);

  char log[2048];
  size_t sizeof_log = sizeof(log);
  OPTIX_CHECK(optixPipelineCreate(optixContext,
                                   &pipelineCompileOptions,
                                   &pipelineLinkOptions,
                                   programGroups.data(),
                                   (int)programGroups.size(),
                                   log, &sizeof_log, &pipeline));
  if (sizeof_log > 1) printf("%s\n", log);

  OPTIX_CHECK(optixPipelineSetStackSize(pipeline, 2*1024, 2*1024, 2*1024, 1));
  if (sizeof_log > 1) printf("%s\n", log);
}

// builds shader binding table (tells opitix what shaders to run and the data to use in a shader)
void SampleRenderer::buildSBT() {
  std::vector<RaygenRecord> raygenRecords;
  for (auto &pg : raygenPGs) {
    RaygenRecord rec;
    OPTIX_CHECK(optixSbtRecordPackHeader(pg, &rec));
    rec.data = nullptr;
    raygenRecords.push_back(rec);
  }
  raygenRecordsBuffer.alloc_and_upload(raygenRecords);
  sbt.raygenRecord = raygenRecordsBuffer.d_pointer();

  std::vector<MissRecord> missRecords;
  for (auto &pg : missPGs) {
    MissRecord rec;
    OPTIX_CHECK(optixSbtRecordPackHeader(pg, &rec));
    rec.data = nullptr;
    missRecords.push_back(rec);
  }
  missRecordsBuffer.alloc_and_upload(missRecords);
  sbt.missRecordBase = missRecordsBuffer.d_pointer();
  sbt.missRecordStrideInBytes = sizeof(MissRecord);
  sbt.missRecordCount = (int)missRecords.size();

  std::vector<HitgroupRecord> hitgroupRecords;
  for (Sphere& s : spheres) {
    HitgroupRecord rec;
    OPTIX_CHECK(optixSbtRecordPackHeader(hitgroupPGs[0], &rec));
    rec.data.color = s.color;
    rec.data.center = s.center;
    rec.data.radius = s.radius;
    rec.data.emissionColor = s.emissionColor;
    rec.data.emissiveStrength = s.emissiveStrength;
	rec.data.transparency = s.transparency;
    rec.data.refractiveIndex = s.refractiveIndex;
    rec.data.materialType = s.materialType; 
    hitgroupRecords.push_back(rec);
  }
  hitgroupRecordsBuffer.alloc_and_upload(hitgroupRecords);
  sbt.hitgroupRecordBase = hitgroupRecordsBuffer.d_pointer();
  sbt.hitgroupRecordStrideInBytes = sizeof(HitgroupRecord);
  sbt.hitgroupRecordCount = (int)hitgroupRecords.size();
}


// Launches the optix pipeline to render the scene. Called every frame in the main loop
void SampleRenderer::render() {
  if (launchParams.frame.size.x == 0) return;
  launchParamsBuffer.upload(&launchParams, 1);
  OPTIX_CHECK(optixLaunch(pipeline, stream,
                           launchParamsBuffer.d_pointer(),
                           launchParamsBuffer.sizeInBytes,
                           &sbt,
                           launchParams.frame.size.x,
                           launchParams.frame.size.y,  // NOTE: optix launches one thread per pixel, which results in one pixel rendered
                           1));
  CUDA_SYNC_CHECK();
}


// Initialize camera parameters
void SampleRenderer::setCamera(const Camera &camera) {
  lastSetCamera = camera;
  launchParams.camera.position  = camera.position;
  launchParams.camera.direction = normalize(camera.lookAt - camera.position);
  const float cosFovy = 0.66f;
  const float aspect  = launchParams.frame.size.x / float(launchParams.frame.size.y);
  launchParams.camera.horizontal = cosFovy * aspect * 
    normalize(cross(launchParams.camera.direction, camera.sceneUpDirection));
  launchParams.camera.vertical = cosFovy * normalize(cross(launchParams.camera.horizontal,
    launchParams.camera.direction));
}

// window resize logic - re-allocates color buffer and updates launch params with new buffer pointer and size
void SampleRenderer::resize(const glm::ivec2 &newSize) {
  if (newSize.x == 0 || newSize.y == 0) return;
  colorBuffer.resize(newSize.x * newSize.y * sizeof(uint32_t));
  launchParams.frame.size        = newSize;
  launchParams.frame.colorBuffer = (uint32_t*)colorBuffer.d_pointer();
  setCamera(lastSetCamera);
}

// Gets the rendered image back from the gpu
// and stores it in the provided h_pixels array
void SampleRenderer::downloadPixels(uint32_t h_pixels[]) {
    colorBuffer.download(h_pixels, launchParams.frame.size.x * launchParams.frame.size.y);
  }

// Update sphere properties and rebuild the SBT for changes to take effect
void SampleRenderer::updateSpheres() {
  CUDA_SYNC_CHECK();  // Ensure GPU finished with previous frame before modifying SBT
  launchParams.traversable = buildAccel(spheres);  // Rebuild acceleration structure for new geometry

  buildSBT();
}

void SampleRenderer::updateSpheres(const std::vector<Sphere>& updatedSpheres) {
    spheres = updatedSpheres;
    
	updateSpheres();
}
} 