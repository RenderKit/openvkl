// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/ManagedObject.h"
#include "../common/objectFactory.h"
#include "rkcommon/math/range.h"
#include "rkcommon/utility/ArrayView.h"

using namespace rkcommon;
using namespace rkcommon::math;

namespace openvkl {
  namespace cpu_device {

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

  }  // namespace cpu_device
}  // namespace openvkl
