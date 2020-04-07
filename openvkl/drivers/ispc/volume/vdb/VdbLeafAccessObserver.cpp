// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VdbLeafAccessObserver.h"

namespace openvkl {
  namespace ispc_driver {

    VdbLeafAccessObserver::VdbLeafAccessObserver(ManagedObject &target,
                                                 size_t size,
                                                 const vkl_uint32 *accessBuffer)
        : target(&target), size(size), accessBuffer(accessBuffer)
    {
      this->target->refInc();
    }

    VdbLeafAccessObserver::~VdbLeafAccessObserver()
    {
      target->refDec();
    }

    const void *VdbLeafAccessObserver::map()
    {
      return accessBuffer;
    }

    void VdbLeafAccessObserver::unmap() {}

    size_t VdbLeafAccessObserver::getNumElements() const
    {
      return size;
    }

    VKLDataType VdbLeafAccessObserver::getElementType() const
    {
      return VKL_UINT;
    }

  }  // namespace ispc_driver
}  // namespace openvkl
