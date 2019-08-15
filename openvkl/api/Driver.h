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

#include <functional>
#include <memory>
#include "../common/VKLCommon.h"
#include "../common/simd.h"
#include "openvkl/openvkl.h"
#include "ospcommon/math/box.h"
#include "ospcommon/math/vec.h"
#include "ospcommon/utility/ArrayView.h"
#include "ospcommon/utility/ParameterizedObject.h"

using namespace ospcommon;
using namespace ospcommon::math;

using VKLIntervalIterator1 = VKLIntervalIterator;
using VKLHitIterator1      = VKLHitIterator;

namespace openvkl {
  namespace api {

    struct Driver : public ospcommon::utility::ParameterizedObject
    {
      static std::shared_ptr<Driver> current;

      Driver()                   = default;
      virtual ~Driver() override = default;

      static Driver *createDriver(const char *driverName);

      // user-provided logging callbacks
      std::function<void(const char *)> messageFunction{[](const char *) {}};

      std::function<void(VKLError, const char *)> errorFunction{
          [](VKLError, const char *) {}};

      std::function<void(const char *)> traceFunction{[](const char *) {}};

      // error tracking
      VKLError lastErrorCode       = VKL_NO_ERROR;
      std::string lastErrorMessage = "no error";

      virtual bool supportsWidth(int width) = 0;

      virtual int getNativeSIMDWidth() = 0;

      virtual void commit();
      bool isCommitted();
      virtual void commit(VKLObject object) = 0;

      virtual void release(VKLObject object) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Data /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLData newData(size_t numItems,
                              VKLDataType dataType,
                              const void *source,
                              VKLDataCreationFlags dataCreationFlags) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Interval iterator ////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

#define __define_initIntervalIteratorN(WIDTH)                            \
  virtual void initIntervalIterator##WIDTH(                              \
      const int *valid,                                                  \
      vVKLIntervalIteratorN<WIDTH> &iterator,                            \
      VKLVolume volume,                                                  \
      const vvec3fn<WIDTH> &origin,                                      \
      const vvec3fn<WIDTH> &direction,                                   \
      const vrange1fn<WIDTH> &tRange,                                    \
      VKLSamplesMask samplesMask)                                        \
  {                                                                      \
    throw std::runtime_error(                                            \
        "initIntervalIterator##WIDTH() not implemented on this driver"); \
  }

      __define_initIntervalIteratorN(1);
      __define_initIntervalIteratorN(4);
      __define_initIntervalIteratorN(8);
      __define_initIntervalIteratorN(16);

#undef __define_initIntervalIteratorN

#define __define_iterateIntervalN(WIDTH)                                      \
  virtual void iterateInterval##WIDTH(const int *valid,                       \
                                      vVKLIntervalIteratorN<WIDTH> &iterator, \
                                      vVKLIntervalN<WIDTH> &interval,         \
                                      vintn<WIDTH> &result)                   \
  {                                                                           \
    throw std::runtime_error(                                                 \
        "iterateInterval##WIDTH() not implemented on this driver");           \
  }

      __define_iterateIntervalN(1);
      __define_iterateIntervalN(4);
      __define_iterateIntervalN(8);
      __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

      /////////////////////////////////////////////////////////////////////////
      // Hit iterator /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

#define __define_initHitIteratorN(WIDTH)                                 \
  virtual void initHitIterator##WIDTH(const int *valid,                  \
                                      vVKLHitIteratorN<WIDTH> &iterator, \
                                      VKLVolume volume,                  \
                                      const vvec3fn<WIDTH> &origin,      \
                                      const vvec3fn<WIDTH> &direction,   \
                                      const vrange1fn<WIDTH> &tRange,    \
                                      VKLSamplesMask samplesMask)        \
  {                                                                      \
    throw std::runtime_error(                                            \
        "initHitIterator##WIDTH() not implemented on this driver");      \
  }

      __define_initHitIteratorN(1);
      __define_initHitIteratorN(4);
      __define_initHitIteratorN(8);
      __define_initHitIteratorN(16);

#undef __define_initHitIteratorN

#define __define_iterateHitN(WIDTH)                                 \
  virtual void iterateHit##WIDTH(const int *valid,                  \
                                 vVKLHitIteratorN<WIDTH> &iterator, \
                                 vVKLHitN<WIDTH> &hit,              \
                                 vintn<WIDTH> &result)              \
  {                                                                 \
    throw std::runtime_error(                                       \
        "iterateHit##WIDTH() not implemented on this driver");      \
  }

      __define_iterateHitN(1);
      __define_iterateHitN(4);
      __define_iterateHitN(8);
      __define_iterateHitN(16);

#undef __define_iterateHitN

      /////////////////////////////////////////////////////////////////////////
      // Module ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLError loadModule(const char *moduleName) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Parameters ///////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual void setBool(VKLObject object,
                           const char *name,
                           const bool b)                                    = 0;
      virtual void set1f(VKLObject object, const char *name, const float x) = 0;
      virtual void set1i(VKLObject object, const char *name, const int x)   = 0;
      virtual void setVec3f(VKLObject object,
                            const char *name,
                            const vec3f &v)                                 = 0;
      virtual void setVec3i(VKLObject object,
                            const char *name,
                            const vec3i &v)                                 = 0;
      virtual void setObject(VKLObject object,
                             const char *name,
                             VKLObject setObject)                           = 0;
      virtual void setString(VKLObject object,
                             const char *name,
                             const std::string &s)                          = 0;
      virtual void setVoidPtr(VKLObject object, const char *name, void *v)  = 0;

      /////////////////////////////////////////////////////////////////////////
      // Samples mask /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLSamplesMask newSamplesMask(VKLVolume volume) = 0;

      virtual void samplesMaskSetRanges(
          VKLSamplesMask samplesMask,
          const utility::ArrayView<const range1f> &ranges) = 0;

      virtual void samplesMaskSetValues(
          VKLSamplesMask samplesMask,
          const utility::ArrayView<const float> &values) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Volume ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLVolume newVolume(const char *type) = 0;

#define __define_computeSampleN(WIDTH)                                       \
  virtual void computeSample##WIDTH(const int *valid,                        \
                                    VKLVolume volume,                        \
                                    const vvec3fn<WIDTH> &objectCoordinates, \
                                    vfloatn<WIDTH> &samples) = 0;

      __define_computeSampleN(1);
      __define_computeSampleN(4);
      __define_computeSampleN(8);
      __define_computeSampleN(16);

#undef __define_computeSampleN

      virtual vec3f computeGradient(VKLVolume volume,
                                    const vec3f &objectCoordinates) = 0;

      virtual box3f getBoundingBox(VKLVolume volume) = 0;

     private:
      bool committed = false;
    };

    // shorthand functions to query current API device
    OPENVKL_CORE_INTERFACE bool driverIsSet();
    OPENVKL_CORE_INTERFACE Driver &currentDriver();

#define VKL_REGISTER_DRIVER(InternalClass, external_name) \
  VKL_REGISTER_OBJECT(                                    \
      ::openvkl::api::Driver, driver, InternalClass, external_name)

  }  // namespace api
}  // namespace openvkl
