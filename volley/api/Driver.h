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

#include <ospcommon/utility/ParameterizedObject.h>
#include <ospcommon/vec.h>
#include <memory>
#include "common/VLYCommon.h"
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

      virtual void commit();
      bool isCommitted();
      virtual void commit(VLYObject object) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Integrator ///////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VLYIntegrator newIntegrator(const char *type) = 0;

      virtual void integrateVolume(
          VLYIntegrator integrator,
          VLYVolume volume,
          size_t numValues,
          const vly_vec3f *origins,
          const vly_vec3f *directions,
          const vly_range1f *ranges,
          void *rayUserData,
          IntegrationStepFunction integrationStepFunction) = 0;

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
      virtual void setVoidPtr(VLYObject object, const char *name, void *v)  = 0;

      /////////////////////////////////////////////////////////////////////////
      // Volume ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VLYVolume newVolume(const char *type) = 0;

      virtual float sampleVolume(VLYVolume volume,
                                 const vly_vec3f *objectCoordinates) = 0;

      virtual vly_vec3f computeGradient(VLYVolume volume,
                                        const vly_vec3f *objectCoordinates) = 0;

      virtual vly_box3f getBoundingBox(VLYVolume volume) = 0;

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
