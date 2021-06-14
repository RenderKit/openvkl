// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "IteratorContext.h"
#include "../common/Data.h"
#include "../common/export_util.h"
#include "../sampler/Sampler.h"
#include "../volume/Volume.h"
#include "IteratorContext_ispc.h"

namespace openvkl {
  namespace cpu_device {

    ///////////////////////////////////////////////////////////////////////////
    // Interval iterator context //////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    IntervalIteratorContext<W>::~IntervalIteratorContext()
    {
      if (this->ispcEquivalent) {
        CALL_ISPC(IntervalIteratorContext_Destructor, this->ispcEquivalent);
        this->ispcEquivalent = nullptr;
      }
    }

    template <int W>
    void IntervalIteratorContext<W>::commit()
    {
      this->attributeIndex = this->template getParam<int>("attributeIndex", 0);

      throwOnIllegalAttributeIndex(&this->getSampler().getVolume(),
                                   this->attributeIndex);

      Ref<const DataT<box1f>> valueRangesData =
          this->template getParamDataT<box1f>("valueRanges", nullptr);

      valueRanges.clear();

      if (valueRangesData) {
        for (const auto &r : *valueRangesData) {
          valueRanges.push_back(r);
        }
      }

      if (this->ispcEquivalent) {
        CALL_ISPC(IntervalIteratorContext_Destructor, this->ispcEquivalent);
      }

      this->ispcEquivalent = CALL_ISPC(IntervalIteratorContext_Constructor,
                                       this->getSampler().getISPCEquivalent(),
                                       this->attributeIndex,
                                       valueRanges.size(),
                                       (const ispc::box1f *)valueRanges.data());
    }

    template struct IntervalIteratorContext<VKL_TARGET_WIDTH>;

    ///////////////////////////////////////////////////////////////////////////
    // Hit iterator context ///////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    HitIteratorContext<W>::~HitIteratorContext()
    {
      if (this->ispcEquivalent) {
        CALL_ISPC(HitIteratorContext_Destructor, this->ispcEquivalent);
        this->ispcEquivalent = nullptr;
      }
    }

    template <int W>
    void HitIteratorContext<W>::commit()
    {
      this->attributeIndex = this->template getParam<int>("attributeIndex", 0);

      throwOnIllegalAttributeIndex(&this->getSampler().getVolume(),
                                   this->attributeIndex);

      Ref<const DataT<float>> valuesData =
          this->template getParamDataT<float>("values", nullptr);

      values.clear();

      if (valuesData) {
        for (const auto &r : *valuesData) {
          values.push_back(r);
        }
      }

      if (this->ispcEquivalent) {
        CALL_ISPC(HitIteratorContext_Destructor, this->ispcEquivalent);
      }

      this->ispcEquivalent = CALL_ISPC(HitIteratorContext_Constructor,
                                       this->getSampler().getISPCEquivalent(),
                                       this->attributeIndex,
                                       values.size(),
                                       (const float *)values.data());
    }

    template struct HitIteratorContext<VKL_TARGET_WIDTH>;

  }  // namespace cpu_device
}  // namespace openvkl
