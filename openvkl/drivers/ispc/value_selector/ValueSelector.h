// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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
