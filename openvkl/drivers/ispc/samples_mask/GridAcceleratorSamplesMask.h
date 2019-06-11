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

#include "SamplesMask.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct Volume;

    template <int W>
    struct SharedStructuredVolume;

    template <int W>
    struct GridAcceleratorSamplesMask : public SamplesMask
    {
      GridAcceleratorSamplesMask(const Volume<W> *volume);

      ~GridAcceleratorSamplesMask();

      void commit() override;

      void *getISPCEquivalent() const
      {
        return ispcEquivalent;
      }

     protected:
      void *ispcEquivalent{nullptr};
      const SharedStructuredVolume<W> *volume{nullptr};
    };

  }  // namespace ispc_driver
}  // namespace openvkl
