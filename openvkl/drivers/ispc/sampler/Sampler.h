// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/ManagedObject.h"
#include "../common/simd.h"
#include "../observer/Observer.h"
#include "openvkl/openvkl.h"
#include "rkcommon/math/vec.h"

using namespace rkcommon;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    class Volume;

    template <int W>
    struct Sampler : public ManagedObject
    {
      virtual ~Sampler();

      // single attribute /////////////////////////////////////////////////////

      // samplers can optionally define a scalar sampling method; if not
      // defined then the default implementation will use computeSampleV()
      virtual void computeSample(const vvec3fn<1> &objectCoordinates,
                                 vfloatn<1> &samples,
                                 unsigned int attributeIndex) const;

      virtual void computeSampleV(const vintn<W> &valid,
                                  const vvec3fn<W> &objectCoordinates,
                                  vfloatn<W> &samples,
                                  unsigned int attributeIndex) const = 0;

      virtual void computeSampleN(unsigned int N,
                                  const vvec3fn<1> *objectCoordinates,
                                  float *samples,
                                  unsigned int attributeIndex) const = 0;

      virtual void computeGradientV(const vintn<W> &valid,
                                    const vvec3fn<W> &objectCoordinates,
                                    vvec3fn<W> &gradients,
                                    unsigned int attributeIndex) const = 0;

      virtual void computeGradientN(unsigned int N,
                                    const vvec3fn<1> *objectCoordinates,
                                    vvec3fn<1> *gradients,
                                    unsigned int attributeIndex) const = 0;

      // multi-attribute //////////////////////////////////////////////////////

      virtual void computeSampleM(const vvec3fn<1> &objectCoordinates,
                                  float *samples,
                                  unsigned int M,
                                  const unsigned int *attributeIndices) const;

      virtual void computeSampleMV(const vintn<W> &valid,
                                   const vvec3fn<W> &objectCoordinates,
                                   float *samples,
                                   unsigned int M,
                                   const unsigned int *attributeIndices) const;

      virtual void computeSampleMN(unsigned int N,
                                   const vvec3fn<1> *objectCoordinates,
                                   float *samples,
                                   unsigned int M,
                                   const unsigned int *attributeIndices) const;

      virtual Observer<W> *newObserver(const char *type) = 0;

      /*
       * Samplers keep references to their underlying volumes!
       */
      virtual Volume<W> &getVolume()             = 0;
      virtual const Volume<W> &getVolume() const = 0;

     protected:
      void *getISPCEquivalent();

     protected:
      void *ispcEquivalent{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline void Sampler<W>::computeSample(const vvec3fn<1> &objectCoordinates,
                                          vfloatn<1> &samples,
                                          unsigned int attributeIndex) const
    {
      // gracefully degrade to use computeSampleV(); see
      // ISPCDriver<W>::computeSampleAnyWidth()

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i == 0 ? 1 : 0;

      ocW.fill_inactive_lanes(validW);

      vfloatn<W> samplesW;

      computeSampleV(validW, ocW, samplesW, attributeIndex);

      samples[0] = samplesW[0];
    }

    template <int W>
    inline void Sampler<W>::computeSampleM(
        const vvec3fn<1> &objectCoordinates,
        float *samples,
        unsigned int M,
        const unsigned int *attributeIndices) const
    {
      for (unsigned int a = 0; a < M; a++) {
        computeSample(
            objectCoordinates, reinterpret_cast<vfloatn<1> &>(samples[a]), a);
      }
    }

    template <int W>
    inline void Sampler<W>::computeSampleMV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        float *samples,
        unsigned int M,
        const unsigned int *attributeIndices) const
    {
      for (unsigned int a = 0; a < M; a++) {
        vfloatn<W> samplesW;

        computeSampleV(valid, objectCoordinates, samplesW, attributeIndices[a]);

        for (int i = 0; i < W; i++)
          samples[a * W + i] = samplesW[i];
      }
    }

    template <int W>
    inline void Sampler<W>::computeSampleMN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        float *samples,
        unsigned int M,
        const unsigned int *attributeIndices) const
    {
      for (unsigned int i = 0; i < N; i++) {
        for (unsigned int a = 0; a < M; a++) {
          computeSample(objectCoordinates[i],
                        reinterpret_cast<vfloatn<1> &>(samples[i * M + a]),
                        attributeIndices[a]);
        }
      }
    }

    ///////////////////////////////////////////////////////////////////////////

    // SamplerBase is the base class for all concrete sampler types.
    // It takes care of keeping a reference to the volume, and provides
    // sensible default implementation where possible.

    template <int W, template <int> class VolumeT>
    struct SamplerBase : public Sampler<W>
    {
      explicit SamplerBase(VolumeT<W> &volume);

      VolumeT<W> &getVolume() override;
      const VolumeT<W> &getVolume() const override;

      Observer<W> *newObserver(const char *type) override;

     protected:
      Ref<VolumeT<W>> volume;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W, template <int> class VolumeT>
    inline SamplerBase<W, VolumeT>::SamplerBase(VolumeT<W> &volume)
        : volume(&volume)
    {
    }

    template <int W, template <int> class VolumeT>
    inline VolumeT<W> &SamplerBase<W, VolumeT>::getVolume()
    {
      return *volume;
    }

    template <int W, template <int> class VolumeT>
    inline const VolumeT<W> &SamplerBase<W, VolumeT>::getVolume() const
    {
      return *volume;
    }

    template <int W, template <int> class VolumeT>
    inline Observer<W> *SamplerBase<W, VolumeT>::newObserver(const char *type)
    {
      /*
       * This is a place to provide potential default observers that
       * work for *all* samplers, if we ever find such a thing.
       */
      return nullptr;
    }

  }  // namespace ispc_driver
}  // namespace openvkl
