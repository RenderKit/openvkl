// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
using namespace rkcommon;
using namespace rkcommon::math;

#include "VdbGrid.h"
#include "VdbLeafAccessObserver.h"
#include "VdbSampler.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    VdbLeafAccessObserver<W>::VdbLeafAccessObserver(VdbSampler<W> &target,
                                                    const VdbGrid &grid)
        : Observer<W>(target)
    {
      accessBuffer = allocator.allocate<uint32>(grid.numLeaves);
      size         = grid.numLeaves;
      getRegistry().add(accessBuffer);
    }

    template <int W>
    VdbLeafAccessObserver<W>::~VdbLeafAccessObserver()
    {
      getRegistry().remove(accessBuffer);
      allocator.deallocate(accessBuffer);
    }

    template <int W>
    const void *VdbLeafAccessObserver<W>::map()
    {
      return accessBuffer;
    }

    template <int W>
    void VdbLeafAccessObserver<W>::unmap()
    {
    }

    template <int W>
    size_t VdbLeafAccessObserver<W>::getNumElements() const
    {
      return size;
    }

    template <int W>
    VKLDataType VdbLeafAccessObserver<W>::getElementType() const
    {
      return VKL_UINT;
    }

    template <int W>
    size_t VdbLeafAccessObserver<W>::getElementSize() const
    {
      return sizeof(uint32_t);
    }

    template <int W>
    ObserverRegistry<W> &VdbLeafAccessObserver<W>::getRegistry()
    {
      return dynamic_cast<VdbSampler<W> &>(*this->target)
          .getLeafAccessObserverRegistry();
    }

    template struct VdbLeafAccessObserver<VKL_TARGET_WIDTH>;

  }  // namespace cpu_device
}  // namespace openvkl
