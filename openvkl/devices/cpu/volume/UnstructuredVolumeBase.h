// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Data.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "Volume.h"
#include "UnstructuredVolumeBaseShared.h"
#include "openvkl/devices/common/StructShared.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct UnstructuredVolumeBase
        : public AddStructShared<Volume<W>, ispc::VKLUnstructuredBase>
    {
      std::string toString() const override;

      UnstructuredVolumeBase(Device *device) : AddStructShared<Volume<W>, ispc::VKLUnstructuredBase>(device) {};  // not = default, due to ICC 19 compiler bug
      ~UnstructuredVolumeBase(){};

      virtual void commit() override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline std::string UnstructuredVolumeBase<W>::toString() const
    {
      return "openvkl::UnstructuredVolumeBase";
    }

    template <int W>
    void UnstructuredVolumeBase<W>::commit()
    {
      Volume<W>::commit();
    }

  }  // namespace cpu_device
}  // namespace openvkl
