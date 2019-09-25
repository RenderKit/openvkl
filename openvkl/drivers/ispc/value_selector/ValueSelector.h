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

#pragma once

#include "../common/ManagedObject.h"
#include "../common/objectFactory.h"
#include "ospcommon/math/range.h"
#include "ospcommon/utility/ArrayView.h"

using namespace ospcommon;
using namespace ospcommon::math;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct Volume;

    template <int W>
    struct ValueSelector : public ManagedObject
    {
      ValueSelector(const Volume<W> *volume);
      ~ValueSelector();

      void commit();

      void setRanges(const utility::ArrayView<const range1f> &ranges);
      void setValues(const utility::ArrayView<const float> &values);

      void *getISPCEquivalent() const;

     private:
      const Volume<W> *volume{nullptr};

      std::vector<range1f> ranges;
      std::vector<float> values;

      void *ispcEquivalent{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline void *ValueSelector<W>::getISPCEquivalent() const
    {
      return ispcEquivalent;
    }

  }  // namespace ispc_driver
}  // namespace openvkl
