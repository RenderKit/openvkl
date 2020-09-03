// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/ManagedObject.h"
#include "openvkl/openvkl.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct Observer : public ManagedObject
    {
      explicit Observer(ManagedObject &target);

      Observer() = delete;
      ~Observer() override;

      std::string toString() const override;

      virtual const void *map()                  = 0;
      virtual void unmap()                       = 0;
      virtual VKLDataType getElementType() const = 0;
      virtual size_t getNumElements() const      = 0;

     protected:
      Ref<ManagedObject> target;
    };

  }  // namespace ispc_driver
}  // namespace openvkl
