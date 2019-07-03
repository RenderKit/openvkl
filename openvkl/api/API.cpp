// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "../common/logging.h"
#include "../common/simd.h"
#include "Driver.h"
#include "openvkl/openvkl.h"
#include "ospcommon/math/box.h"
#include "ospcommon/math/vec.h"
#include "ospcommon/utility/ArrayView.h"
#include "ospcommon/utility/OnScopeExit.h"

using namespace openvkl;

#define TRACE_PREFIX "[openvkl] "

inline std::string getPidString()
{
  char s[100];
  sprintf(s, "(pid %i)", getpid());
  return s;
}

void postTraceMessage(const std::string &message)
{
  if (openvkl::api::driverIsSet()) {
    openvkl::api::currentDriver().traceFunction(
        (TRACE_PREFIX + message + '\n').c_str());
  }
}

#define ASSERT_DRIVER()                         \
  if (!openvkl::api::driverIsSet())             \
    throw std::runtime_error(                   \
        "OpenVKL not yet initialized "          \
        "(most likely this means you tried to " \
        "call a OpenVKL API function before "   \
        "first calling vklInit())" +            \
        getPidString());

#define ASSERT_DRIVER_SUPPORTS_WIDTH(WIDTH)                                 \
  if (!openvkl::api::currentDriver().supportsWidth(WIDTH))                  \
    throw std::runtime_error(                                               \
        "the current OpenVKL driver does not support the requested vector " \
        "width " +                                                          \
        std::string(#WIDTH));

#warning API tracing disabled

#define OPENVKL_CATCH_BEGIN_TRACE          \
  try {                                    \
    auto *fcn_name_ = __PRETTY_FUNCTION__; \
    ospcommon::utility::OnScopeExit guard( \
        [&]() { postTraceMessage(fcn_name_); });

#define OPENVKL_CATCH_BEGIN try {
#define OPENVKL_CATCH_END(a)                                       \
  }                                                                \
  catch (const std::bad_alloc &)                                   \
  {                                                                \
    openvkl::handleError(VKL_OUT_OF_MEMORY,                        \
                         "OpenVKL was unable to allocate memory"); \
    return a;                                                      \
  }                                                                \
  catch (const std::exception &e)                                  \
  {                                                                \
    openvkl::handleError(VKL_UNKNOWN_ERROR, e.what());             \
    return a;                                                      \
  }                                                                \
  catch (...)                                                      \
  {                                                                \
    openvkl::handleError(VKL_UNKNOWN_ERROR,                        \
                         "an unrecognized exception was caught");  \
    return a;                                                      \
  }

///////////////////////////////////////////////////////////////////////////////
// Data ///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLData vklNewData(size_t numItems,
                              VKLDataType dataType,
                              const void *source,
                              VKLDataCreationFlags dataCreationFlags)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  VKLData data = openvkl::api::currentDriver().newData(
      numItems, dataType, source, dataCreationFlags);
  return data;
}
OPENVKL_CATCH_END(nullptr)

///////////////////////////////////////////////////////////////////////////////
// Driver /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLDriver vklNewDriver(const char *driverName) OPENVKL_CATCH_BEGIN
{
  return (VKLDriver)openvkl::api::Driver::createDriver(driverName);
}
OPENVKL_CATCH_END(nullptr)

extern "C" void vklCommitDriver(VKLDriver driver) OPENVKL_CATCH_BEGIN
{
  auto *object = (openvkl::api::Driver *)driver;
  object->commit();
}
OPENVKL_CATCH_END()

extern "C" void vklSetCurrentDriver(VKLDriver driver) OPENVKL_CATCH_BEGIN
{
  auto *object = (openvkl::api::Driver *)driver;

  if (!object->isCommitted()) {
    throw std::runtime_error("You must commit the driver before using it!");
  }

  openvkl::api::Driver::current.reset(object);
}
OPENVKL_CATCH_END()

extern "C" int vklGetNativeSIMDWidth() OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  return openvkl::api::currentDriver().getNativeSIMDWidth();
}
OPENVKL_CATCH_END(0)

extern "C" void vklCommit(VKLObject object) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  Assert(object && "invalid object handle to commit to");
  openvkl::api::currentDriver().commit(object);
}
OPENVKL_CATCH_END()

extern "C" void vklRelease(VKLObject object) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  Assert(object && "invalid object handle to release");
  openvkl::api::currentDriver().release(object);
}
OPENVKL_CATCH_END()

extern "C" void vklShutdown() OPENVKL_CATCH_BEGIN
{
  openvkl::api::Driver::current.reset();
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Iterator ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" void vklInitRayIterator(VKLRayIterator *rayIterator,
                                   VKLVolume volume,
                                   const vkl_vec3f *origin,
                                   const vkl_vec3f *direction,
                                   const vkl_range1f *tRange,
                                   VKLSamplesMask samplesMask)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  constexpr int valid = 1;
  return openvkl::api::currentDriver().initRayIterator1(
      &valid,
      reinterpret_cast<vVKLRayIteratorN<1> &>(*rayIterator),
      volume,
      reinterpret_cast<const vvec3fn<1> &>(*origin),
      reinterpret_cast<const vvec3fn<1> &>(*direction),
      reinterpret_cast<const vrange1fn<1> &>(*tRange),
      samplesMask);
}
OPENVKL_CATCH_END()

#define __define_vklInitRayIteratorN(WIDTH)                        \
  extern "C" void vklInitRayIterator##WIDTH(                       \
      const int *valid,                                            \
      VKLRayIterator##WIDTH *rayIterator,                          \
      VKLVolume volume,                                            \
      const vkl_vvec3f##WIDTH *origin,                             \
      const vkl_vvec3f##WIDTH *direction,                          \
      const vkl_vrange1f##WIDTH *tRange,                           \
      VKLSamplesMask samplesMask) OPENVKL_CATCH_BEGIN              \
  {                                                                \
    ASSERT_DRIVER();                                               \
    return openvkl::api::currentDriver().initRayIterator##WIDTH(   \
        valid,                                                     \
        reinterpret_cast<vVKLRayIteratorN<WIDTH> &>(*rayIterator), \
        volume,                                                    \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*origin),         \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*direction),      \
        reinterpret_cast<const vrange1fn<WIDTH> &>(*tRange),       \
        samplesMask);                                              \
  }                                                                \
  OPENVKL_CATCH_END()

__define_vklInitRayIteratorN(4);
__define_vklInitRayIteratorN(8);
__define_vklInitRayIteratorN(16);

#undef __define_vklInitRayIteratorN

extern "C" bool vklIterateInterval(VKLRayIterator *rayIterator,
                                   VKLRayInterval *rayInterval)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  constexpr int valid = 1;
  vVKLRayIntervalN<1> rayIntervalInternal;
  int result;
  openvkl::api::currentDriver().iterateInterval1(
      &valid,
      reinterpret_cast<vVKLRayIteratorN<1> &>(*rayIterator),
      rayIntervalInternal,
      reinterpret_cast<vintn<1> &>(result));
  *rayInterval = static_cast<VKLRayInterval>(rayIntervalInternal);
  return result;
}
OPENVKL_CATCH_END(false)

#define __define_vklIterateIntervalN(WIDTH)                                 \
  extern "C" void vklIterateInterval##WIDTH(                                \
      const int *valid,                                                     \
      VKLRayIterator##WIDTH *rayIterator,                                   \
      VKLRayInterval##WIDTH *rayInterval,                                   \
      int *result) OPENVKL_CATCH_BEGIN                                      \
  {                                                                         \
    ASSERT_DRIVER();                                                        \
    vVKLRayIntervalN<WIDTH> rayIntervalInternal;                            \
    openvkl::api::currentDriver().iterateInterval##WIDTH(                   \
        valid,                                                              \
        reinterpret_cast<vVKLRayIteratorN<WIDTH> &>(*rayIterator),          \
        rayIntervalInternal,                                                \
        reinterpret_cast<vintn<WIDTH> &>(*result));                         \
    *rayInterval = static_cast<VKLRayInterval##WIDTH>(rayIntervalInternal); \
  }                                                                         \
  OPENVKL_CATCH_END()

__define_vklIterateIntervalN(4);
__define_vklIterateIntervalN(8);
__define_vklIterateIntervalN(16);

#undef __define_vklIterateIntervalN

extern "C" bool vklIterateSurface(VKLRayIterator *rayIterator,
                                  VKLSurfaceHit *surfaceHit) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  constexpr int valid = 1;
  vVKLSurfaceHitN<1> surfaceHitInternal;
  int result;
  openvkl::api::currentDriver().iterateSurface1(
      &valid,
      reinterpret_cast<vVKLRayIteratorN<1> &>(*rayIterator),
      surfaceHitInternal,
      reinterpret_cast<vintn<1> &>(result));
  *surfaceHit = static_cast<VKLSurfaceHit>(surfaceHitInternal);
  return result;
}
OPENVKL_CATCH_END(false)

#define __define_vklIterateSurfaceN(WIDTH)                                     \
  extern "C" void vklIterateSurface##WIDTH(const int *valid,                   \
                                           VKLRayIterator##WIDTH *rayIterator, \
                                           VKLSurfaceHit##WIDTH *surfaceHit,   \
                                           int *result) OPENVKL_CATCH_BEGIN    \
  {                                                                            \
    ASSERT_DRIVER();                                                           \
    vVKLSurfaceHitN<WIDTH> surfaceHitInternal;                                 \
    openvkl::api::currentDriver().iterateSurface##WIDTH(                       \
        valid,                                                                 \
        reinterpret_cast<vVKLRayIteratorN<WIDTH> &>(*rayIterator),             \
        surfaceHitInternal,                                                    \
        reinterpret_cast<vintn<WIDTH> &>(*result));                            \
    *surfaceHit = static_cast<VKLSurfaceHit##WIDTH>(surfaceHitInternal);       \
  }                                                                            \
  OPENVKL_CATCH_END()

__define_vklIterateSurfaceN(4);
__define_vklIterateSurfaceN(8);
__define_vklIterateSurfaceN(16);

#undef __define_vklIterateSurfaceN

///////////////////////////////////////////////////////////////////////////////
// Module /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLError vklLoadModule(const char *moduleName) OPENVKL_CATCH_BEGIN
{
  if (openvkl::api::driverIsSet()) {
    return (VKLError)openvkl::api::currentDriver().loadModule(moduleName);
  } else {
    return openvkl::loadLocalModule(moduleName);
  }
}
OPENVKL_CATCH_END(VKL_UNKNOWN_ERROR)

///////////////////////////////////////////////////////////////////////////////
// Parameters /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" void vklSet1f(VKLObject object,
                         const char *name,
                         float x) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  openvkl::api::currentDriver().set1f(object, name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklSet3f(VKLObject object,
                         const char *name,
                         float x,
                         float y,
                         float z) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  openvkl::api::currentDriver().setVec3f(object, name, vec3f(x, y, z));
}
OPENVKL_CATCH_END()

extern "C" void vklSet1i(VKLObject object,
                         const char *name,
                         int x) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  openvkl::api::currentDriver().set1i(object, name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklSet3i(
    VKLObject object, const char *name, int x, int y, int z) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  openvkl::api::currentDriver().setVec3i(object, name, vec3i(x, y, z));
}
OPENVKL_CATCH_END()

extern "C" void vklSetData(VKLObject object,
                           const char *name,
                           VKLData data) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  openvkl::api::currentDriver().setObject(object, name, (VKLObject)data);
}
OPENVKL_CATCH_END()

extern "C" void vklSetString(VKLObject object,
                             const char *name,
                             const char *s) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  openvkl::api::currentDriver().setString(object, name, std::string(s));
}
OPENVKL_CATCH_END()

extern "C" void vklSetVoidPtr(VKLObject object,
                              const char *name,
                              void *v) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  openvkl::api::currentDriver().setVoidPtr(object, name, v);
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Samples mask ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLSamplesMask vklNewSamplesMask(VKLVolume volume)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  VKLSamplesMask samplesMask =
      openvkl::api::currentDriver().newSamplesMask(volume);
  if (samplesMask == nullptr) {
    postLogMessage(openvkl::VKL_LOG_ERROR) << "could not create samples mask";
  }

  return samplesMask;
}
OPENVKL_CATCH_END(nullptr)

extern "C" void vklSamplesMaskSetRanges(VKLSamplesMask samplesMask,
                                        size_t numRanges,
                                        const vkl_range1f *ranges)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  openvkl::api::currentDriver().samplesMaskSetRanges(
      samplesMask,
      utility::ArrayView<const range1f>(
          reinterpret_cast<const range1f *>(ranges), numRanges));
}
OPENVKL_CATCH_END()

extern "C" void vklSamplesMaskSetValues(VKLSamplesMask samplesMask,
                                        size_t numValues,
                                        const float *values) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  openvkl::api::currentDriver().samplesMaskSetValues(
      samplesMask,
      utility::ArrayView<const float>(reinterpret_cast<const float *>(values),
                                      numValues));
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Volume /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLVolume vklNewVolume(const char *type) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  Assert(type != nullptr && "invalid volume type identifier in vklNewVolume");
  VKLVolume volume = openvkl::api::currentDriver().newVolume(type);
  if (volume == nullptr) {
    postLogMessage(openvkl::VKL_LOG_ERROR)
        << "could not create volume '" << type << "'";
  }

  return volume;
}
OPENVKL_CATCH_END(nullptr)

extern "C" float vklComputeSample(
    VKLVolume volume, const vkl_vec3f *objectCoordinates) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  constexpr int valid = 1;
  float sample;
  openvkl::api::currentDriver().computeSample1(
      &valid,
      volume,
      reinterpret_cast<const vvec3fn<1> &>(*objectCoordinates),
      reinterpret_cast<vfloatn<1> &>(sample));
  return sample;
}
OPENVKL_CATCH_END(ospcommon::math::nan)

#define __define_vklComputeSampleN(WIDTH)                             \
  extern "C" void vklComputeSample##WIDTH(                            \
      const int *valid,                                               \
      VKLVolume volume,                                               \
      const vkl_vvec3f##WIDTH *objectCoordinates,                     \
      float *samples) OPENVKL_CATCH_BEGIN                             \
  {                                                                   \
    ASSERT_DRIVER();                                                  \
    ASSERT_DRIVER_SUPPORTS_WIDTH(WIDTH);                              \
                                                                      \
    openvkl::api::currentDriver().computeSample##WIDTH(               \
        valid,                                                        \
        volume,                                                       \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*objectCoordinates), \
        reinterpret_cast<vfloatn<WIDTH> &>(*samples));                \
  }                                                                   \
  OPENVKL_CATCH_END()

__define_vklComputeSampleN(4);
__define_vklComputeSampleN(8);
__define_vklComputeSampleN(16);

#undef __define_vklComputeSampleN

extern "C" vkl_vec3f vklComputeGradient(
    VKLVolume volume, const vkl_vec3f *objectCoordinates) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  const vec3f result = openvkl::api::currentDriver().computeGradient(
      volume, reinterpret_cast<const vec3f &>(*objectCoordinates));
  return reinterpret_cast<const vkl_vec3f &>(result);
}
OPENVKL_CATCH_END(vkl_vec3f{ospcommon::math::nan})

extern "C" vkl_box3f vklGetBoundingBox(VKLVolume volume) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  const box3f result = openvkl::api::currentDriver().getBoundingBox(volume);
  return reinterpret_cast<const vkl_box3f &>(result);
}
OPENVKL_CATCH_END(vkl_box3f{ospcommon::math::nan})
