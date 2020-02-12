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

#include "ValueSelector.h"
#include "../volume/Volume.h"
#include "ValueSelector_ispc.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    ValueSelector<W>::ValueSelector(const Volume<W> *volume) : volume(volume)
    {
    }

    template <int W>
    ValueSelector<W>::~ValueSelector()
    {
      if (ispcEquivalent) {
        ispc::ValueSelector_Destructor(ispcEquivalent);
      }
    }

    template <int W>
    void ValueSelector<W>::commit()
    {
      if (ispcEquivalent) {
        ispc::ValueSelector_Destructor(ispcEquivalent);
      }

      ispcEquivalent =
          ispc::ValueSelector_Constructor(nullptr,
                                          ranges.size(),
                                          (const ispc::box1f *)ranges.data(),
                                          values.size(),
                                          (const float *)values.data());
    }

    template <int W>
    void ValueSelector<W>::setRanges(
        const utility::ArrayView<const range1f> &ranges)
    {
      this->ranges.clear();

      for (const auto &r : ranges) {
        this->ranges.push_back(r);
      }
    }

    template <int W>
    void ValueSelector<W>::setValues(
        const utility::ArrayView<const float> &values)
    {
      this->values.clear();

      for (const auto &v : values) {
        this->values.push_back(v);
      }
    }

#if TARGET_WIDTH_ENABLED_4
    template struct ValueSelector<4>;
#endif

#if TARGET_WIDTH_ENABLED_8
    template struct ValueSelector<8>;
#endif

#if TARGET_WIDTH_ENABLED_16
    template struct ValueSelector<16>;
#endif

  }  // namespace ispc_driver
}  // namespace openvkl
