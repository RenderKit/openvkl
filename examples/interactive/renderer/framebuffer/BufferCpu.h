// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <rkcommon/containers/AlignedVector.h>
#include <rkcommon/math/vec.h>

#include "../Scheduler.h"
#include "Buffer.h"
#include "BufferCpu.h"
#include "BufferDisplay.h"
#include "Stats.h"

namespace openvkl {
  namespace examples {
    class BufferCpu : public Buffer
    {
     public:
      vec4f *getRgba() override final
      {
        return bufRgba.data();
      }
      const vec4f *getRgba() const override final
      {
        return bufRgba.data();
      }

      float *getWeight()
      {
        return bufWeight.data();
      }
      const float *getWeight() const
      {
        return bufWeight.data();
      }

      void resize(size_t _w, size_t _h) override final;

      void tonemap(BufferDisplay &outputBuffer);

     private:
      rkcommon::containers::AlignedVector<vec4f> bufRgba;
      rkcommon::containers::AlignedVector<float> bufWeight;
    };
  }  // namespace examples
}  // namespace openvkl