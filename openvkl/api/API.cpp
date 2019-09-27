// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#ifdef _WIN32
#include <process.h>  // for getpid
#endif

using namespace openvkl;

#define TRACE_PREFIX "[openvkl] "

inline std::string getPidString()
{
  char s[100];
  sprintf(s, "(pid %i)", getpid());
  return s;
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

#define THROW_IF_NULL(obj, name)                         \
  if (obj == nullptr)                                    \
  throw std::runtime_error(std::string("null ") + name + \
                           std::string(" provided to ") + __FUNCTION__)

// convenience macros for repeated use of the above
#define THROW_IF_NULL_OBJECT(obj) THROW_IF_NULL(obj, "handle")
#define THROW_IF_NULL_STRING(str) THROW_IF_NULL(str, "string")

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
  THROW_IF_NULL_STRING(driverName);
  return (VKLDriver)openvkl::api::Driver::createDriver(driverName);
}
OPENVKL_CATCH_END(nullptr)

extern "C" void vklCommitDriver(VKLDriver driver) OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(driver);
  auto *object = (openvkl::api::Driver *)driver;
  object->commit();
}
OPENVKL_CATCH_END()

extern "C" void vklSetCurrentDriver(VKLDriver driver) OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(driver);
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
  THROW_IF_NULL_OBJECT(object);
  openvkl::api::currentDriver().commit(object);
}
OPENVKL_CATCH_END()

extern "C" void vklRelease(VKLObject object) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  THROW_IF_NULL_OBJECT(object);
  openvkl::api::currentDriver().release(object);
}
OPENVKL_CATCH_END()

extern "C" void vklShutdown() OPENVKL_CATCH_BEGIN
{
  openvkl::api::Driver::current.reset();
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Interval iterator //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" void vklInitIntervalIterator(VKLIntervalIterator *iterator,
                                        VKLVolume volume,
                                        const vkl_vec3f *origin,
                                        const vkl_vec3f *direction,
                                        const vkl_range1f *tRange,
                                        VKLValueSelector valueSelector)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  constexpr int valid = 1;
  return openvkl::api::currentDriver().initIntervalIterator1(
      &valid,
      reinterpret_cast<vVKLIntervalIteratorN<1> &>(*iterator),
      volume,
      reinterpret_cast<const vvec3fn<1> &>(*origin),
      reinterpret_cast<const vvec3fn<1> &>(*direction),
      reinterpret_cast<const vrange1fn<1> &>(*tRange),
      valueSelector);
}
OPENVKL_CATCH_END()

#define __define_vklInitIntervalIteratorN(WIDTH)                      \
  extern "C" void vklInitIntervalIterator##WIDTH(                     \
      const int *valid,                                               \
      VKLIntervalIterator##WIDTH *iterator,                           \
      VKLVolume volume,                                               \
      const vkl_vvec3f##WIDTH *origin,                                \
      const vkl_vvec3f##WIDTH *direction,                             \
      const vkl_vrange1f##WIDTH *tRange,                              \
      VKLValueSelector valueSelector) OPENVKL_CATCH_BEGIN             \
  {                                                                   \
    ASSERT_DRIVER();                                                  \
    return openvkl::api::currentDriver().initIntervalIterator##WIDTH( \
        valid,                                                        \
        reinterpret_cast<vVKLIntervalIteratorN<WIDTH> &>(*iterator),  \
        volume,                                                       \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*origin),            \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*direction),         \
        reinterpret_cast<const vrange1fn<WIDTH> &>(*tRange),          \
        valueSelector);                                               \
  }                                                                   \
  OPENVKL_CATCH_END()

__define_vklInitIntervalIteratorN(4);
__define_vklInitIntervalIteratorN(8);
__define_vklInitIntervalIteratorN(16);

#undef __define_vklInitIntervalIteratorN

extern "C" int vklIterateInterval(VKLIntervalIterator *iterator,
                                  VKLInterval *interval) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  constexpr int valid = 1;
  vVKLIntervalN<1> intervalInternal;
  int result;
  openvkl::api::currentDriver().iterateInterval1(
      &valid,
      reinterpret_cast<vVKLIntervalIteratorN<1> &>(*iterator),
      intervalInternal,
      reinterpret_cast<vintn<1> &>(result));
  *interval = static_cast<VKLInterval>(intervalInternal);
  return result;
}
OPENVKL_CATCH_END(false)

#define __define_vklIterateIntervalN(WIDTH)                          \
  extern "C" void vklIterateInterval##WIDTH(                         \
      const int *valid,                                              \
      VKLIntervalIterator##WIDTH *iterator,                          \
      VKLInterval##WIDTH *interval,                                  \
      int *result) OPENVKL_CATCH_BEGIN                               \
  {                                                                  \
    ASSERT_DRIVER();                                                 \
    vVKLIntervalN<WIDTH> intervalInternal;                           \
    openvkl::api::currentDriver().iterateInterval##WIDTH(            \
        valid,                                                       \
        reinterpret_cast<vVKLIntervalIteratorN<WIDTH> &>(*iterator), \
        intervalInternal,                                            \
        reinterpret_cast<vintn<WIDTH> &>(*result));                  \
    *interval = static_cast<VKLInterval##WIDTH>(intervalInternal);   \
  }                                                                  \
  OPENVKL_CATCH_END()

__define_vklIterateIntervalN(4);
__define_vklIterateIntervalN(8);
__define_vklIterateIntervalN(16);

#undef __define_vklIterateIntervalN

///////////////////////////////////////////////////////////////////////////////
// Hit iterator ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" void vklInitHitIterator(VKLHitIterator *iterator,
                                   VKLVolume volume,
                                   const vkl_vec3f *origin,
                                   const vkl_vec3f *direction,
                                   const vkl_range1f *tRange,
                                   VKLValueSelector valueSelector)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  constexpr int valid = 1;
  return openvkl::api::currentDriver().initHitIterator1(
      &valid,
      reinterpret_cast<vVKLHitIteratorN<1> &>(*iterator),
      volume,
      reinterpret_cast<const vvec3fn<1> &>(*origin),
      reinterpret_cast<const vvec3fn<1> &>(*direction),
      reinterpret_cast<const vrange1fn<1> &>(*tRange),
      valueSelector);
}
OPENVKL_CATCH_END()

#define __define_vklInitHitIteratorN(WIDTH)                      \
  extern "C" void vklInitHitIterator##WIDTH(                     \
      const int *valid,                                          \
      VKLHitIterator##WIDTH *iterator,                           \
      VKLVolume volume,                                          \
      const vkl_vvec3f##WIDTH *origin,                           \
      const vkl_vvec3f##WIDTH *direction,                        \
      const vkl_vrange1f##WIDTH *tRange,                         \
      VKLValueSelector valueSelector) OPENVKL_CATCH_BEGIN        \
  {                                                              \
    ASSERT_DRIVER();                                             \
    return openvkl::api::currentDriver().initHitIterator##WIDTH( \
        valid,                                                   \
        reinterpret_cast<vVKLHitIteratorN<WIDTH> &>(*iterator),  \
        volume,                                                  \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*origin),       \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*direction),    \
        reinterpret_cast<const vrange1fn<WIDTH> &>(*tRange),     \
        valueSelector);                                          \
  }                                                              \
  OPENVKL_CATCH_END()

__define_vklInitHitIteratorN(4);
__define_vklInitHitIteratorN(8);
__define_vklInitHitIteratorN(16);

#undef __define_vklInitHitIteratorN

extern "C" int vklIterateHit(VKLHitIterator *iterator,
                             VKLHit *hit) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  constexpr int valid = 1;
  vVKLHitN<1> hitInternal;
  int result;
  openvkl::api::currentDriver().iterateHit1(
      &valid,
      reinterpret_cast<vVKLHitIteratorN<1> &>(*iterator),
      hitInternal,
      reinterpret_cast<vintn<1> &>(result));
  *hit = static_cast<VKLHit>(hitInternal);
  return result;
}
OPENVKL_CATCH_END(false)

#define __define_vklIterateHitN(WIDTH)                                  \
  extern "C" void vklIterateHit##WIDTH(const int *valid,                \
                                       VKLHitIterator##WIDTH *iterator, \
                                       VKLHit##WIDTH *hit,              \
                                       int *result) OPENVKL_CATCH_BEGIN \
  {                                                                     \
    ASSERT_DRIVER();                                                    \
    vVKLHitN<WIDTH> hitInternal;                                        \
    openvkl::api::currentDriver().iterateHit##WIDTH(                    \
        valid,                                                          \
        reinterpret_cast<vVKLHitIteratorN<WIDTH> &>(*iterator),         \
        hitInternal,                                                    \
        reinterpret_cast<vintn<WIDTH> &>(*result));                     \
    *hit = static_cast<VKLHit##WIDTH>(hitInternal);                     \
  }                                                                     \
  OPENVKL_CATCH_END()

__define_vklIterateHitN(4);
__define_vklIterateHitN(8);
__define_vklIterateHitN(16);

#undef __define_vklIterateHitN

///////////////////////////////////////////////////////////////////////////////
// Module /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLError vklLoadModule(const char *moduleName) OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(moduleName);
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

extern "C" void vklSetBool(VKLObject object,
                           const char *name,
                           int b) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDriver().setBool(object, name, static_cast<bool>(b));
}
OPENVKL_CATCH_END()

extern "C" void vklSetFloat(VKLObject object,
                            const char *name,
                            float x) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDriver().set1f(object, name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklSetVec3f(VKLObject object,
                            const char *name,
                            float x,
                            float y,
                            float z) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDriver().setVec3f(object, name, vec3f(x, y, z));
}
OPENVKL_CATCH_END()

extern "C" void vklSetInt(VKLObject object,
                          const char *name,
                          int x) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDriver().set1i(object, name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklSetVec3i(
    VKLObject object, const char *name, int x, int y, int z) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDriver().setVec3i(object, name, vec3i(x, y, z));
}
OPENVKL_CATCH_END()

extern "C" void vklSetData(VKLObject object,
                           const char *name,
                           VKLData data) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDriver().setObject(object, name, (VKLObject)data);
}
OPENVKL_CATCH_END()

extern "C" void vklSetString(VKLObject object,
                             const char *name,
                             const char *s) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDriver().setString(object, name, std::string(s));
}
OPENVKL_CATCH_END()

extern "C" void vklSetVoidPtr(VKLObject object,
                              const char *name,
                              void *v) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDriver().setVoidPtr(object, name, v);
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Value selector /////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLValueSelector vklNewValueSelector(VKLVolume volume)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  VKLValueSelector valueSelector =
      openvkl::api::currentDriver().newValueSelector(volume);
  if (valueSelector == nullptr) {
    postLogMessage(openvkl::VKL_LOG_ERROR) << "could not create value selector";
  }

  return valueSelector;
}
OPENVKL_CATCH_END(nullptr)

extern "C" void vklValueSelectorSetRanges(VKLValueSelector valueSelector,
                                          size_t numRanges,
                                          const vkl_range1f *ranges)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  openvkl::api::currentDriver().valueSelectorSetRanges(
      valueSelector,
      utility::ArrayView<const range1f>(
          reinterpret_cast<const range1f *>(ranges), numRanges));
}
OPENVKL_CATCH_END()

extern "C" void vklValueSelectorSetValues(VKLValueSelector valueSelector,
                                          size_t numValues,
                                          const float *values)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  openvkl::api::currentDriver().valueSelectorSetValues(
      valueSelector,
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
  THROW_IF_NULL_STRING(type);
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
  constexpr int valid = 1;
  vkl_vec3f gradient;
  openvkl::api::currentDriver().computeGradient1(
      &valid,
      volume,
      reinterpret_cast<const vvec3fn<1> &>(*objectCoordinates),
      reinterpret_cast<vvec3fn<1> &>(gradient));
  return gradient;
}
OPENVKL_CATCH_END(vkl_vec3f{ospcommon::math::nan})

#define __define_vklComputeGradientN(WIDTH)                           \
  extern "C" void vklComputeGradient##WIDTH(                          \
      const int *valid,                                               \
      VKLVolume volume,                                               \
      const vkl_vvec3f##WIDTH *objectCoordinates,                     \
      vkl_vvec3f##WIDTH *gradients) OPENVKL_CATCH_BEGIN               \
  {                                                                   \
    ASSERT_DRIVER();                                                  \
    ASSERT_DRIVER_SUPPORTS_WIDTH(WIDTH);                              \
                                                                      \
    openvkl::api::currentDriver().computeGradient##WIDTH(             \
        valid,                                                        \
        volume,                                                       \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*objectCoordinates), \
        reinterpret_cast<vvec3fn<WIDTH> &>(*gradients));              \
  }                                                                   \
  OPENVKL_CATCH_END()

__define_vklComputeGradientN(4);
__define_vklComputeGradientN(8);
__define_vklComputeGradientN(16);

#undef __define_vklComputeGradientN

extern "C" vkl_box3f vklGetBoundingBox(VKLVolume volume) OPENVKL_CATCH_BEGIN
{
  ASSERT_DRIVER();
  const box3f result = openvkl::api::currentDriver().getBoundingBox(volume);
  return reinterpret_cast<const vkl_box3f &>(result);
}
OPENVKL_CATCH_END(vkl_box3f{ospcommon::math::nan})
