// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../../api/Device.h"
#include "../common/device_simd.h"
#include "../include/openvkl/device/openvkl.h"

namespace openvkl {
  namespace cpu_device {

    struct AddDeviceAPIs : public api::Device
    {
      /////////////////////////////////////////////////////////////////////////
      // Sampler //////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

#define __define_computeSampleN(WIDTH)                                       \
  virtual void computeSample##WIDTH(const int *valid,                        \
                                    const VKLSampler *sampler,               \
                                    const vvec3fn<WIDTH> &objectCoordinates, \
                                    float *samples,                          \
                                    unsigned int attributeIndex,             \
                                    const float *times) = 0;

      __define_computeSampleN(1);
      __define_computeSampleN(4);
      __define_computeSampleN(8);
      __define_computeSampleN(16);

#undef __define_computeSampleN

      virtual void computeSampleN(const VKLSampler *sampler,
                                  unsigned int N,
                                  const vvec3fn<1> *objectCoordinates,
                                  float *samples,
                                  unsigned int attributeIndex,
                                  const float *times) = 0;

#define __define_computeSampleMN(WIDTH)                                       \
  virtual void computeSampleM##WIDTH(const int *valid,                        \
                                     const VKLSampler *sampler,               \
                                     const vvec3fn<WIDTH> &objectCoordinates, \
                                     float *samples,                          \
                                     unsigned int M,                          \
                                     const unsigned int *attributeIndices,    \
                                     const float *times) = 0;

      __define_computeSampleMN(1);
      __define_computeSampleMN(4);
      __define_computeSampleMN(8);
      __define_computeSampleMN(16);

#undef __define_computeSampleMN

      virtual void computeSampleMN(const VKLSampler *sampler,
                                   unsigned int N,
                                   const vvec3fn<1> *objectCoordinates,
                                   float *samples,
                                   unsigned int M,
                                   const unsigned int *attributeIndices,
                                   const float *times) = 0;

#define __define_computeGradientN(WIDTH)                                       \
  virtual void computeGradient##WIDTH(const int *valid,                        \
                                      const VKLSampler *sampler,               \
                                      const vvec3fn<WIDTH> &objectCoordinates, \
                                      vvec3fn<WIDTH> &gradients,               \
                                      unsigned int attributeIndex,             \
                                      const float *times) = 0;

      __define_computeGradientN(1);
      __define_computeGradientN(4);
      __define_computeGradientN(8);
      __define_computeGradientN(16);

#undef __define_computeGradientN

      virtual void computeGradientN(const VKLSampler *sampler,
                                    unsigned int N,
                                    const vvec3fn<1> *objectCoordinates,
                                    vvec3fn<1> *gradients,
                                    unsigned int attributeIndex,
                                    const float *times) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Interval iterator ////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual size_t getIntervalIteratorSize1(
          const VKLIntervalIteratorContext *context) const = 0;

#define __define_getIntervalIteratorSizeN(WIDTH) \
  virtual size_t getIntervalIteratorSize##WIDTH( \
      const VKLIntervalIteratorContext *context) const = 0;

      __define_getIntervalIteratorSizeN(4);
      __define_getIntervalIteratorSizeN(8);
      __define_getIntervalIteratorSizeN(16);

#undef __define_getIntervalIteratorSizeN

      virtual VKLIntervalIterator initIntervalIterator1(
          const VKLIntervalIteratorContext *context,
          const vvec3fn<1> &origin,
          const vvec3fn<1> &direction,
          const vrange1fn<1> &tRange,
          float time,
          void *buffer) const = 0;

#define __define_initIntervalIteratorN(WIDTH)                     \
  virtual VKLIntervalIterator##WIDTH initIntervalIterator##WIDTH( \
      const int *valid,                                           \
      const VKLIntervalIteratorContext *context,                  \
      const vvec3fn<WIDTH> &origin,                               \
      const vvec3fn<WIDTH> &direction,                            \
      const vrange1fn<WIDTH> &tRange,                             \
      const float *times,                                         \
      void *buffer) const = 0;

      __define_initIntervalIteratorN(4);
      __define_initIntervalIteratorN(8);
      __define_initIntervalIteratorN(16);

#undef __define_initIntervalIteratorN

      virtual void iterateInterval1(VKLIntervalIterator iterator,
                                    vVKLIntervalN<1> &interval,
                                    int *result) const = 0;

#define __define_iterateIntervalN(WIDTH)                                   \
  virtual void iterateInterval##WIDTH(const int *valid,                    \
                                      VKLIntervalIterator##WIDTH iterator, \
                                      vVKLIntervalN<WIDTH> &interval,      \
                                      int *result) const = 0;

      __define_iterateIntervalN(4);
      __define_iterateIntervalN(8);
      __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

      /////////////////////////////////////////////////////////////////////////
      // Hit iterator /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual size_t getHitIteratorSize1(
          const VKLHitIteratorContext *context) const = 0;

#define __define_getHitIteratorSizeN(WIDTH) \
  virtual size_t getHitIteratorSize##WIDTH( \
      const VKLHitIteratorContext *context) const = 0;

      __define_getHitIteratorSizeN(4);
      __define_getHitIteratorSizeN(8);
      __define_getHitIteratorSizeN(16);

#undef __define_getHitIteratorSizeN

      virtual VKLHitIterator initHitIterator1(
          const VKLHitIteratorContext *context,
          const vvec3fn<1> &origin,
          const vvec3fn<1> &direction,
          const vrange1fn<1> &tRange,
          float time,
          void *buffer) const = 0;

#define __define_initHitIteratorN(WIDTH)                \
  virtual VKLHitIterator##WIDTH initHitIterator##WIDTH( \
      const int *valid,                                 \
      const VKLHitIteratorContext *context,             \
      const vvec3fn<WIDTH> &origin,                     \
      const vvec3fn<WIDTH> &direction,                  \
      const vrange1fn<WIDTH> &tRange,                   \
      const float *times,                               \
      void *buffer) const = 0;

      __define_initHitIteratorN(4);
      __define_initHitIteratorN(8);
      __define_initHitIteratorN(16);

#undef __define_initHitIteratorN

      virtual void iterateHit1(VKLHitIterator iterator,
                               vVKLHitN<1> &hit,
                               int *result) const = 0;

#define __define_iterateHitN(WIDTH)                              \
  virtual void iterateHit##WIDTH(const int *valid,               \
                                 VKLHitIterator##WIDTH iterator, \
                                 vVKLHitN<WIDTH> &hit,           \
                                 int *result) const = 0;

      __define_iterateHitN(4);
      __define_iterateHitN(8);
      __define_iterateHitN(16);

#undef __define_iterateHitN
    };

  }  // namespace cpu_device
}  // namespace openvkl
