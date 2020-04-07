// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"
#include "openvkl/openvkl.h"

namespace openvkl {

  struct OPENVKL_CORE_INTERFACE Observer : public ManagedObject
  {
    virtual ~Observer() override;
    virtual std::string toString() const override;

    virtual const void *map() = 0;
    virtual void unmap() = 0;
    virtual VKLDataType getElementType() const = 0;
    virtual size_t getNumElements() const = 0;
  };

}  // namespace openvkl

