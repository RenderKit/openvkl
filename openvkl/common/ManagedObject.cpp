// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ManagedObject.h"

#include <algorithm>

namespace openvkl {

  ManagedObject::~ManagedObject()
  {
    // make sure all ManagedObject parameter refs are decremented
    std::for_each(params_begin(), params_end(), [&](std::shared_ptr<Param> &p) {
      auto &param = *p;
      if (param.data.is<VKL_PTR>()) {
        auto *obj = param.data.get<VKL_PTR>();
        if (obj != nullptr)
          obj->refDec();
      }
    });
  }

  std::string ManagedObject::toString() const
  {
    return "openvkl::ManagedObject";
  }

}  // namespace openvkl
