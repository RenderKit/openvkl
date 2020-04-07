// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ValueSelector.h"
#include "../common/export_util.h"
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
        CALL_ISPC(ValueSelector_Destructor, ispcEquivalent);
      }
    }

    template <int W>
    void ValueSelector<W>::commit()
    {
      if (ispcEquivalent) {
        CALL_ISPC(ValueSelector_Destructor, ispcEquivalent);
      }

      ispcEquivalent = CALL_ISPC(ValueSelector_Constructor,
                                 nullptr,
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

    template struct ValueSelector<VKL_TARGET_WIDTH>;

  }  // namespace ispc_driver
}  // namespace openvkl
