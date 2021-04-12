// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VdbLeafAccessObserver.h"
#include "VdbGrid.h"
#include "VdbSampler.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    VdbLeafAccessObserver<W>::VdbLeafAccessObserver(VdbSampler<W> &target,
                                                    const VdbGrid &grid)
        : Observer<W>(target)
    {
      accessBuffer = allocator.allocate<uint32>(grid.totalNumLeaves);
      size         = grid.totalNumLeaves;
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
