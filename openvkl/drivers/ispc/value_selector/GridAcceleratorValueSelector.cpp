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

#include "GridAcceleratorValueSelector.h"
#include "../volume/StructuredRegularVolume.h"
#include "GridAcceleratorValueSelector_ispc.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    GridAcceleratorValueSelector<W>::GridAcceleratorValueSelector(
        const Volume<W> *volume)
        : volume(static_cast<const StructuredRegularVolume<W> *>(volume))
    {
    }

    template <int W>
    GridAcceleratorValueSelector<W>::~GridAcceleratorValueSelector()
    {
      if (ispcEquivalent) {
        ispc::GridAcceleratorValueSelector_Destructor(ispcEquivalent);
      }
    }

    template <int W>
    void GridAcceleratorValueSelector<W>::commit()
    {
      if (ispcEquivalent) {
        ispc::GridAcceleratorValueSelector_Destructor(ispcEquivalent);
      }

      ispcEquivalent = ispc::GridAcceleratorValueSelector_Constructor(
          volume->getISPCEquivalent(),
          ranges.size(),
          (const ispc::box1f *)ranges.data(),
          values.size(),
          (const float *)values.data());
    }

    template struct GridAcceleratorValueSelector<4>;
    template struct GridAcceleratorValueSelector<8>;
    template struct GridAcceleratorValueSelector<16>;

  }  // namespace ispc_driver
}  // namespace openvkl
