// ======================================================================== //
// Copyright 2019-2020 Intel Corporation                                    //
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

#include "Iterator.h"

namespace openvkl {
  namespace ispc_driver {

    ///////////////////////////////////////////////////////////////////////////
    // Uniform iterator ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    struct GridAcceleratorIteratorU : public IteratorU<W>
    {
      GridAcceleratorIteratorU(const Volume<W> *volume,
                               const vvec3fn<1> &origin,
                               const vvec3fn<1> &direction,
                               const vrange1fn<1> &tRange,
                               const ValueSelector<W> *valueSelector);

      const Interval<1> *getCurrentInterval() const override;
      void iterateInterval(vintn<1> &result) override;

      const Hit<1> *getCurrentHit() const override;
      void iterateHit(vintn<1> &result) override;

      // required size of ISPC-side object for width; exported to support
      // functional tests
      static constexpr int OPENVKL_DLLEXPORT ispcStorageSize = 120;

     protected:
      alignas(simd_alignment_for_width(W)) char ispcStorage[ispcStorageSize];
    };

    ///////////////////////////////////////////////////////////////////////////
    // Varying iterator ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    struct GridAcceleratorIteratorV : public IteratorV<W>
    {
      GridAcceleratorIteratorV(const vintn<W> &valid,
                               const Volume<W> *volume,
                               const vvec3fn<W> &origin,
                               const vvec3fn<W> &direction,
                               const vrange1fn<W> &tRange,
                               const ValueSelector<W> *valueSelector);

      const Interval<W> *getCurrentInterval() const override;
      void iterateInterval(const vintn<W> &valid, vintn<W> &result) override;

      const Hit<W> *getCurrentHit() const override;
      void iterateHit(const vintn<W> &valid, vintn<W> &result) override;

      // required size of ISPC-side object for width; exported to support
      // functional tests
      static constexpr int OPENVKL_DLLEXPORT ispcStorageSize = 112 * W;

     protected:
      alignas(simd_alignment_for_width(W)) char ispcStorage[ispcStorageSize];
    };

  }  // namespace ispc_driver
}  // namespace openvkl
