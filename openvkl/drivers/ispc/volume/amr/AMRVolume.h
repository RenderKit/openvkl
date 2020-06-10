// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Volume.h"
#include "../../iterator/Iterator.h"
#include "../../iterator/DefaultIterator.h"
#include "AMRAccel.h"
#include "rkcommon/memory/RefCount.h"

using namespace rkcommon::memory;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    using AmrIntervalIteratorFactory =
        ConcreteIteratorFactory<W,
                                IntervalIterator,
                                DefaultIntervalIterator>;

    template <int W>
    using AmrHitIterator = DefaultHitIterator<W, DefaultIntervalIterator<W>>;

    template <int W>
    using AmrHitIteratorFactory =
        ConcreteIteratorFactory<W, HitIterator, AmrHitIterator>;


    template <int W>
    struct AMRVolume : public Volume<W>
    {
      AMRVolume();
      ~AMRVolume() override;

      std::string toString() const override;

      void commit() override;

      Sampler<W> *newSampler() override;

      box3f getBoundingBox() const override;
      unsigned int getNumAttributes() const override;
      range1f getValueRange() const override;

      std::unique_ptr<amr::AMRData> data;
      std::unique_ptr<amr::AMRAccel> accel;

      Ref<const DataT<Data *>> blockDataData;
      Ref<const DataT<box3i>> blockBoundsData;
      Ref<const DataT<int>> refinementLevelsData;
      Ref<const DataT<float>> cellWidthsData;
      VKLDataType voxelType;
      range1f valueRange{empty};
      box3f bounds;

      VKLAMRMethod amrMethod;

      const IteratorFactory<W, IntervalIterator>
          &getIntervalIteratorFactory() const override final
      {
        return intervalIteratorFactory;
      }

      const IteratorFactory<W, HitIterator> &getHitIteratorFactory()
          const override final
      {
        return hitIteratorFactory;
      }

     private:
      AmrIntervalIteratorFactory<W> intervalIteratorFactory;
      AmrHitIteratorFactory<W> hitIteratorFactory;
    };

  }  // namespace ispc_driver
}  // namespace openvkl
