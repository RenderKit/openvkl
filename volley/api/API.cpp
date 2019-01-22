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

#include "Driver.h"
#include "common/logging.h"
#include "ospcommon/utility/OnScopeExit.h"
#include "volley/volley.h"

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

#define VOLLEY_CATCH_BEGIN                 \
  try {                                    \
    auto *fcn_name_ = __PRETTY_FUNCTION__; \
    ospcommon::utility::OnScopeExit guard( \
        [&]() { postTraceMessage(fcn_name_); });

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

extern "C" VLYError vlyLoadModule(const char *moduleName) VOLLEY_CATCH_BEGIN
{
  if (volley::api::driverIsSet()) {
    return (VLYError)volley::api::currentDriver().loadModule(moduleName);
  } else {
    return volley::loadLocalModule(moduleName);
  }
}
VOLLEY_CATCH_END(VLY_UNKNOWN_ERROR)

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

extern "C" void vlyIntersectVolume(VLYVolume volume,
                                   size_t numValues,
                                   const vly_vec3f *origins,
                                   const vly_vec3f *directions,
                                   vly_range1f *ranges) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().intersectVolume(
      volume, numValues, origins, directions, ranges);
}
VOLLEY_CATCH_END()

extern "C" void vlySampleVolume(VLYVolume volume,
                                VLYSamplingType samplingType,
                                size_t numValues,
                                const vly_vec3f *worldCoordinates,
                                float *results) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().sampleVolume(
      volume, samplingType, numValues, worldCoordinates, results);
}
VOLLEY_CATCH_END()

extern "C" void vlyAdvanceRays(VLYVolume volume,
                               float samplingRate,
                               size_t numValues,
                               const vly_vec3f *origins,
                               const vly_vec3f *directions,
                               float *t) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().advanceRays(
      volume, samplingRate, numValues, origins, directions, t);
}
VOLLEY_CATCH_END()

extern "C" void vlyIntegrateVolume(
    VLYVolume volume,
    VLYSamplingType samplingType,
    float samplingRate,
    size_t numValues,
    const vly_vec3f *origins,
    const vly_vec3f *directions,
    const vly_range1f *ranges,
    void *rayUserData,
    IntegrationStepFunction integrationStepFunction) VOLLEY_CATCH_BEGIN
{
  ASSERT_DRIVER();
  volley::api::currentDriver().integrateVolume(volume,
                                               samplingType,
                                               samplingRate,
                                               numValues,
                                               origins,
                                               directions,
                                               ranges,
                                               rayUserData,
                                               integrationStepFunction);
}
VOLLEY_CATCH_END()
