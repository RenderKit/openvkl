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

#pragma once

#include <ospray/ospcommon/box.h>
#include <ospray/ospcommon/utility/ArrayView.h>
#include <ospray/ospcommon/utility/ParameterizedObject.h>
#include <ospray/ospcommon/vec.h>
#include <functional>
#include <memory>
#include "common/VLYCommon.h"
#include "common/simd.h"
#include "volley/volley.h"

using namespace ospcommon;

namespace volley {
  namespace api {

    struct Driver : public ospcommon::utility::ParameterizedObject
    {
      static std::shared_ptr<Driver> current;

      Driver()                   = default;
      virtual ~Driver() override = default;

      static Driver *createDriver(const char *driverName);

      // user-provided logging callbacks
      std::function<void(const char *)> messageFunction{[](const char *) {}};

      std::function<void(VLYError, const char *)> errorFunction{
          [](VLYError, const char *) {}};

      std::function<void(const char *)> traceFunction{[](const char *) {}};

      // error tracking
      VLYError lastErrorCode       = VLY_NO_ERROR;
      std::string lastErrorMessage = "no error";

      virtual bool supportsWidth(int width)
      {
        return false;
      }

      virtual void commit();
      bool isCommitted();
      virtual void commit(VLYObject object) = 0;

      virtual void release(VLYObject object) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Data /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VLYData newData(size_t numItems,
                              VLYDataType dataType,
                              const void *source,
                              VLYDataCreationFlags dataCreationFlags) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Iterator /////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VLYRayIterator newRayIterator(VLYVolume volume,
                                            const vec3f &origin,
                                            const vec3f &direction,
                                            const range1f &tRange,
                                            VLYSamplesMask samplesMask) = 0;

#define __define_newRayIteratorN(WIDTH)                            \
  virtual VLYRayIterator newRayIterator##WIDTH(                    \
      const int *valid,                                            \
      VLYVolume volume,                                            \
      const vvec3fn<WIDTH> &origin,                                \
      const vvec3fn<WIDTH> &direction,                             \
      const vrange1fn<WIDTH> &tRange,                              \
      VLYSamplesMask samplesMask)                                  \
  {                                                                \
    throw std::runtime_error(                                      \
        "newRayIterator##WIDTH() not implemented on this driver"); \
  }

      __define_newRayIteratorN(4);
      __define_newRayIteratorN(8);
      __define_newRayIteratorN(16);

#undef __define_newRayIteratorN

      virtual bool iterateInterval(VLYRayIterator rayIterator,
                                   VLYRayInterval &rayInterval) = 0;

#define __define_iterateIntervalN(WIDTH)                                    \
  virtual void iterateInterval##WIDTH(const int *valid,                     \
                                      VLYRayIterator rayIterator,           \
                                      vVLYRayIntervalN<WIDTH> &rayInterval, \
                                      vintn<WIDTH> &result)                 \
  {                                                                         \
    throw std::runtime_error(                                               \
        "iterateInterval##WIDTH() not implemented on this driver");         \
  }

      __define_iterateIntervalN(4);
      __define_iterateIntervalN(8);
      __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

      virtual void iterateSurface8(const int *valid,
                                   VLYRayIterator rayIterator,
                                   VLYSurfaceHit8 &surfaceHit,
                                   vintn<8> &result)
      {
        throw std::runtime_error(
            "iterateSurface8() not implemented in this driver");
      }

      /////////////////////////////////////////////////////////////////////////
      // Module ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VLYError loadModule(const char *moduleName) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Parameters ///////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual void set1f(VLYObject object, const char *name, const float x) = 0;
      virtual void set1i(VLYObject object, const char *name, const int x)   = 0;
      virtual void setVec3f(VLYObject object,
                            const char *name,
                            const vec3f &v)                                 = 0;
      virtual void setVec3i(VLYObject object,
                            const char *name,
                            const vec3i &v)                                 = 0;
      virtual void setObject(VLYObject object,
                             const char *name,
                             VLYObject setObject)                           = 0;
      virtual void setString(VLYObject object,
                             const char *name,
                             const std::string &s)                          = 0;
      virtual void setVoidPtr(VLYObject object, const char *name, void *v)  = 0;

      /////////////////////////////////////////////////////////////////////////
      // Samples mask /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VLYSamplesMask newSamplesMask(VLYVolume volume) = 0;

      virtual void samplesMaskSetRanges(
          VLYSamplesMask samplesMask,
          const utility::ArrayView<const range1f> &ranges) = 0;

      virtual void samplesMaskSetValues(
          VLYSamplesMask samplesMask,
          const utility::ArrayView<const float> &values) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Volume ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VLYVolume newVolume(const char *type) = 0;

      virtual float computeSample(VLYVolume volume,
                                  const vec3f &objectCoordinates) = 0;

#define __define_computeSampleN(WIDTH)                                       \
  virtual void computeSample##WIDTH(const int *valid,                        \
                                    VLYVolume volume,                        \
                                    const vvec3fn<WIDTH> &objectCoordinates, \
                                    vfloatn<WIDTH> &samples) = 0;

      __define_computeSampleN(4);
      __define_computeSampleN(8);
      __define_computeSampleN(16);

#undef __define_computeSampleN

      virtual vec3f computeGradient(VLYVolume volume,
                                    const vec3f &objectCoordinates) = 0;

      virtual box3f getBoundingBox(VLYVolume volume) = 0;

     private:
      bool committed = false;
    };

    // shorthand functions to query current API device
    VOLLEY_CORE_INTERFACE bool driverIsSet();
    VOLLEY_CORE_INTERFACE Driver &currentDriver();

#define VLY_REGISTER_DRIVER(InternalClass, external_name) \
  VLY_REGISTER_OBJECT(                                    \
      ::volley::api::Driver, driver, InternalClass, external_name)

  }  // namespace api
}  // namespace volley
