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

#include <ospcommon/utility/ArrayView.h>
#include <ospcommon/range.h>
#include "common/ManagedObject.h"
#include "common/objectFactory.h"
#include "volley/volley.h"

using namespace ospcommon;

namespace volley {
  namespace scalar_driver {

    struct SamplesMask : public ManagedObject
    {
      SamplesMask()                   = default;
      virtual ~SamplesMask() override = default;

      static SamplesMask *createInstance()
      {
        return createInstanceHelper<SamplesMask, VLY_SAMPLES_MASK>("base");
      }

      virtual void commit() override
      {
        ManagedObject::commit();
      }

      void addRanges(const utility::ArrayView<const range1f> &ranges);
    };

#define VLY_REGISTER_SAMPLES_MASK(InternalClass, external_name) \
  VLY_REGISTER_OBJECT(SamplesMask, samples_mask, InternalClass, external_name)

  }  // namespace scalar_driver
}  // namespace volley
