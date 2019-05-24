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

#include <ospray/ospcommon/box.h>
#include <ospray/ospcommon/utility/ArrayView.h>
#include <ospray/ospcommon/utility/OnScopeExit.h>
#include <ospray/ospcommon/vec.h>
#include "Driver.h"
#include "common/logging.h"
#include "common/simd.h"
#include "volley/volley.h"

using namespace volley;
using namespace ospcommon;

#define TRACE_PREFIX "[volley] "

inline std::string getPidString()
{
  char s[100];
  sprintf(s, "(pid %i)", getpid());
  return s;
}

void postTraceMessage(const std::string &message)
{
  if (volley::api::driverIsSet()) {
    volley::api::currentDriver().traceFunction(
        (TRACE_PREFIX + message + '\n').c_str());
  }
}

#define ASSERT_DRIVER()                         \
  if (!volley::api::driverIsSet())              \
    throw std::runtime_error(                   \
        "Volley not yet initialized "           \
        "(most likely this means you tried to " \
        "call a Volley API function before "    \
        "first calling vlyInit())" +            \
        getPidString());

#define ASSERT_DRIVER_SUPPORTS_WIDTH(WIDTH)                                \
  if (!volley::api::currentDriver().supportsWidth(WIDTH))                  \
    throw std::runtime_error(                                              \
        "the current Volley driver does not support the requested vector " \
        "width " +                                                         \
        std::string(#WIDTH));

#warning API tracing disabled

#define VOLLEY_CATCH_BEGIN_TRACE           \
  try {                                    \
    auto *fcn_name_ = __PRETTY_FUNCTION__; \
    ospcommon::utility::OnScopeExit guard( \
        [&]() { postTraceMessage(fcn_name_); });

#define VOLLEY_CATCH_BEGIN try {
#define VOLLEY_CATCH_END(a)                                      \
  }                                                              \
  catch (const std::bad_alloc &)                                 \
  {                                                              \
    volley::handleError(VLY_OUT_OF_MEMORY,                       \
                        "Volley was unable to allocate memory"); \
    return a;                                                    \
  }                                                              \
  catch (const std::exception &e)                                \
  {                                                              \
    volley::handleError(VLY_UNKNOWN_ERROR, e.what());            \
    return a;                                                    \
  }                                                              \
  catch (...)                                                    \
  {                                                              \
    volley::handleError(VLY_UNKNOWN_ERROR,                       \
                        "an unrecognized exception was caught"); \
    return a;                                                    \
  }

///////////////////////////////////////////////////////////////////////////////
// Data ///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VLYData vlyNewData(size_t numItems,
                              VLYDataType dataType,
                              const void *source,
                              VLYDataCreationFlags dataCreationFlags)
    VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  VLYData data = volley::api::currentDriver().newData(
      numItems, dataType, source, dataCreationFlags);
  return data;
}
VOLLEY_CATCH_END(nullptr)

///////////////////////////////////////////////////////////////////////////////
// Driver /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VLYDriver vlyNewDriver(const char *driverName) VOLLEY_CATCH_BEGIN
{
  return (VLYDriver)volley::api::Driver::createDriver(driverName);
}
VOLLEY_CATCH_END(nullptr)

extern "C" void vlyCommitDriver(VLYDriver driver) VOLLEY_CATCH_BEGIN
{
  auto *object = (volley::api::Driver *)driver;
  object->commit();
}
VOLLEY_CATCH_END()

extern "C" void vlySetCurrentDriver(VLYDriver driver) VOLLEY_CATCH_BEGIN
{
  auto *object = (volley::api::Driver *)driver;

  if (!object->isCommitted()) {
    throw std::runtime_error("You must commit the driver before using it!");
  }

  volley::api::Driver::current.reset(object);
}
VOLLEY_CATCH_END()

extern "C" void vlyCommit(VLYObject object) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  Assert(object && "invalid object handle to commit to");
  volley::api::currentDriver().commit(object);
}
VOLLEY_CATCH_END()

extern "C" void vlyRelease(VLYObject object) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  Assert(object && "invalid object handle to release");
  volley::api::currentDriver().release(object);
}
VOLLEY_CATCH_END()

extern "C" void vlyShutdown() VOLLEY_CATCH_BEGIN
{
  volley::api::Driver::current.reset();
}
VOLLEY_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Iterator ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VLYRayIterator vlyNewRayIterator(VLYVolume volume,
                                            const vly_vec3f *origin,
                                            const vly_vec3f *direction,
                                            const vly_range1f *tRange,
                                            VLYSamplesMask samplesMask)
    VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  return volley::api::currentDriver().newRayIterator(
      volume,
      reinterpret_cast<const vec3f &>(*origin),
      reinterpret_cast<const vec3f &>(*direction),
      reinterpret_cast<const range1f &>(*tRange),
      samplesMask);
}
VOLLEY_CATCH_END(nullptr)

#define __define_vlyNewRayIteratorN(WIDTH)                     \
  extern "C" VLYRayIterator vlyNewRayIterator##WIDTH(          \
      const int *valid,                                        \
      VLYVolume volume,                                        \
      const vly_vvec3f##WIDTH *origin,                         \
      const vly_vvec3f##WIDTH *direction,                      \
      const vly_vrange1f##WIDTH *tRange,                       \
      VLYSamplesMask samplesMask) VOLLEY_CATCH_BEGIN           \
  {                                                            \
    ASSERT_DRIVER();                                           \
    return volley::api::currentDriver().newRayIterator##WIDTH( \
        valid,                                                 \
        volume,                                                \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*origin),     \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*direction),  \
        reinterpret_cast<const vrange1fn<WIDTH> &>(*tRange),   \
        samplesMask);                                          \
  }                                                            \
  VOLLEY_CATCH_END(nullptr)

__define_vlyNewRayIteratorN(4);
__define_vlyNewRayIteratorN(8);
__define_vlyNewRayIteratorN(16);

#undef __define_vlyNewRayIteratorN

extern "C" bool vlyIterateInterval(
    VLYRayIterator rayIterator, VLYRayInterval *rayInterval) VOLLEY_CATCH_BEGIN
{
  return volley::api::currentDriver().iterateInterval(
      rayIterator, reinterpret_cast<VLYRayInterval &>(*rayInterval));
}
VOLLEY_CATCH_END(false)

extern "C" void vlyIterateInterval8(const int *valid,
                                    VLYRayIterator rayIterator,
                                    VLYRayInterval8 *rayInterval,
                                    int *result) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();

  return volley::api::currentDriver().iterateInterval8(
      valid,
      rayIterator,
      reinterpret_cast<VLYRayInterval8 &>(*rayInterval),
      reinterpret_cast<vintn<8> &>(*result));
}
VOLLEY_CATCH_END()

extern "C" void vlyIterateSurface8(const int *valid,
                                   VLYRayIterator rayIterator,
                                   VLYSurfaceHit8 *surfaceHit,
                                   int *result) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();

  return volley::api::currentDriver().iterateSurface8(
      valid,
      rayIterator,
      reinterpret_cast<VLYSurfaceHit8 &>(*surfaceHit),
      reinterpret_cast<vintn<8> &>(*result));
}
VOLLEY_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Module /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VLYError vlyLoadModule(const char *moduleName) VOLLEY_CATCH_BEGIN
{
  if (volley::api::driverIsSet()) {
    return (VLYError)volley::api::currentDriver().loadModule(moduleName);
  } else {
    return volley::loadLocalModule(moduleName);
  }
}
VOLLEY_CATCH_END(VLY_UNKNOWN_ERROR)

///////////////////////////////////////////////////////////////////////////////
// Parameters /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" void vlySet1f(VLYObject object,
                         const char *name,
                         float x) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().set1f(object, name, x);
}
VOLLEY_CATCH_END()

extern "C" void vlySet3f(VLYObject object,
                         const char *name,
                         float x,
                         float y,
                         float z) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().setVec3f(object, name, vec3f(x, y, z));
}
VOLLEY_CATCH_END()

extern "C" void vlySet1i(VLYObject object,
                         const char *name,
                         int x) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().set1i(object, name, x);
}
VOLLEY_CATCH_END()

extern "C" void vlySet3i(
    VLYObject object, const char *name, int x, int y, int z) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().setVec3i(object, name, vec3i(x, y, z));
}
VOLLEY_CATCH_END()

extern "C" void vlySetData(VLYObject object,
                           const char *name,
                           VLYData data) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().setObject(object, name, (VLYObject)data);
}
VOLLEY_CATCH_END()

extern "C" void vlySetString(VLYObject object,
                             const char *name,
                             const char *s) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().setString(object, name, std::string(s));
}
VOLLEY_CATCH_END()

extern "C" void vlySetVoidPtr(VLYObject object,
                              const char *name,
                              void *v) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().setVoidPtr(object, name, v);
}
VOLLEY_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Samples mask ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VLYSamplesMask vlyNewSamplesMask(VLYVolume volume) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  VLYSamplesMask samplesMask =
      volley::api::currentDriver().newSamplesMask(volume);
  if (samplesMask == nullptr) {
    postLogMessage(volley::VLY_LOG_ERROR) << "could not create samples mask";
  }

  return samplesMask;
}
VOLLEY_CATCH_END(nullptr)

extern "C" void vlySamplesMaskSetRanges(VLYSamplesMask samplesMask,
                                        size_t numRanges,
                                        const vly_range1f *ranges)
    VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().samplesMaskSetRanges(
      samplesMask,
      utility::ArrayView<const range1f>(
          reinterpret_cast<const range1f *>(ranges), numRanges));
}
VOLLEY_CATCH_END()

extern "C" void vlySamplesMaskSetValues(VLYSamplesMask samplesMask,
                                        size_t numValues,
                                        const float *values) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().samplesMaskSetValues(
      samplesMask,
      utility::ArrayView<const float>(reinterpret_cast<const float *>(values),
                                      numValues));
}
VOLLEY_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Volume /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VLYVolume vlyNewVolume(const char *type) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  Assert(type != nullptr && "invalid volume type identifier in vlyNewVolume");
  VLYVolume volume = volley::api::currentDriver().newVolume(type);
  if (volume == nullptr) {
    postLogMessage(volley::VLY_LOG_ERROR)
        << "could not create volume '" << type << "'";
  }

  return volume;
}
VOLLEY_CATCH_END(nullptr)

extern "C" float vlyComputeSample(
    VLYVolume volume, const vly_vec3f *objectCoordinates) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  return volley::api::currentDriver().computeSample(
      volume, reinterpret_cast<const vec3f &>(*objectCoordinates));
}
VOLLEY_CATCH_END(ospcommon::nan)

#define __define_vlyComputeSampleN(WIDTH)                             \
  extern "C" void vlyComputeSample##WIDTH(                            \
      const int *valid,                                               \
      VLYVolume volume,                                               \
      const vly_vvec3f##WIDTH *objectCoordinates,                     \
      float *samples) VOLLEY_CATCH_BEGIN                              \
  {                                                                   \
    ASSERT_DRIVER();                                                  \
    ASSERT_DRIVER_SUPPORTS_WIDTH(WIDTH);                              \
                                                                      \
    volley::api::currentDriver().computeSample##WIDTH(                \
        valid,                                                        \
        volume,                                                       \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*objectCoordinates), \
        reinterpret_cast<vfloatn<WIDTH> &>(*samples));                \
  }                                                                   \
  VOLLEY_CATCH_END()

__define_vlyComputeSampleN(4);
__define_vlyComputeSampleN(8);
__define_vlyComputeSampleN(16);

#undef __define_vlyComputeSampleN

extern "C" vly_vec3f vlyComputeGradient(
    VLYVolume volume, const vly_vec3f *objectCoordinates) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  const vec3f result = volley::api::currentDriver().computeGradient(
      volume, reinterpret_cast<const vec3f &>(*objectCoordinates));
  return reinterpret_cast<const vly_vec3f &>(result);
}
VOLLEY_CATCH_END(vly_vec3f{ospcommon::nan})

extern "C" vly_box3f vlyGetBoundingBox(VLYVolume volume) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  const box3f result = volley::api::currentDriver().getBoundingBox(volume);
  return reinterpret_cast<const vly_box3f &>(result);
}
VOLLEY_CATCH_END(vly_box3f{ospcommon::nan})
