// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "IteratorContext.h"
#include "../common/Data.h"
#include "../common/export_util.h"
#include "../sampler/Sampler.h"
#include "../volume/Volume.h"
#include "../volume/vdb/VdbVolume.h"
#include "IteratorContext_ispc.h"
#include "rkcommon/math/range.h"

namespace openvkl {
  namespace cpu_device {

    ///////////////////////////////////////////////////////////////////////////
    // Helpers ////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    inline int getMaxIteratorDepthParameter(IteratorContext<W> &iteratorContext)
    {
      // default for maxIteratorDepth depends on volume type; later this
      // parameter will be replaced with some more generic, with a default valid
      // for all volume types
      int maxIteratorDepthDefault = 6;

      if (dynamic_cast<const VdbVolume<W> *>(
              &iteratorContext.getSampler().getVolume())) {
        maxIteratorDepthDefault = VKL_VDB_NUM_LEVELS - 2;
      }

      const int maxIteratorDepth =
          std::max(iteratorContext.template getParam<int>(
                       "maxIteratorDepth", maxIteratorDepthDefault),
                   0);

      return maxIteratorDepth;
    }

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
      // attribute index
      this->attributeIndex = this->template getParam<int>("attributeIndex", 0);

      throwOnIllegalAttributeIndex(&this->getSampler().getVolume(),
                                   this->attributeIndex);

      // value ranges
      Ref<const DataT<box1f>> valueRangesData =
          this->template getParamDataT<box1f>("valueRanges", nullptr);

      std::vector<range1f> valueRanges;

      if (valueRangesData) {
        for (const auto &r : *valueRangesData) {
          valueRanges.push_back(r);
        }
      }

      // other parameters

      const int maxIteratorDepth = getMaxIteratorDepthParameter(*this);

      const bool elementaryCellIteration =
          this->template getParam<bool>("elementaryCellIteration", false);

      if (this->ispcEquivalent) {
        CALL_ISPC(IntervalIteratorContext_Destructor, this->ispcEquivalent);
      }

      this->ispcEquivalent = CALL_ISPC(IntervalIteratorContext_Constructor,
                                       this->getSampler().getISPCEquivalent(),
                                       this->attributeIndex,
                                       valueRanges.size(),
                                       (const ispc::box1f *)valueRanges.data(),
                                       maxIteratorDepth,
                                       elementaryCellIteration);
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

      std::vector<float> values;

      if (valuesData) {
        for (const auto &r : *valuesData) {
          values.push_back(r);
        }
      }

      const int maxIteratorDepth = getMaxIteratorDepthParameter(*this);

      if (this->ispcEquivalent) {
        CALL_ISPC(HitIteratorContext_Destructor, this->ispcEquivalent);
      }

      this->ispcEquivalent = CALL_ISPC(HitIteratorContext_Constructor,
                                       this->getSampler().getISPCEquivalent(),
                                       this->attributeIndex,
                                       values.size(),
                                       (const float *)values.data(),
                                       maxIteratorDepth);
    }

    template struct HitIteratorContext<VKL_TARGET_WIDTH>;

  }  // namespace cpu_device
}  // namespace openvkl
