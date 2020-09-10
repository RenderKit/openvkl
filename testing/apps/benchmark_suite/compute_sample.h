// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "utility.h"

/*
 * Benchmark wrappers for vklComputeSample* (single attribute) functions.
 */

namespace api {

  using openvkl::vfloatn;
  using openvkl::vintn;
  using openvkl::vvec3fn;

  template <class ProgrammingModel,
            class VolumeWrapper,
            class CoordinateGenerator>
  struct VklComputeSample;

  template <class VolumeWrapper, class CoordinateGenerator>
  struct VklComputeSample<programming_model::Scalar,
                          VolumeWrapper,
                          CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "scalar" << CoordinateGenerator::name() << "Sample";
      if (!VolumeWrapper::name().empty())
         os << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      VolumeWrapper wrapper;
      VKLSampler sampler = wrapper.getSampler();
      CoordinateGenerator gen(vklGetBoundingBox(wrapper.getVolume()));

      vkl_vec3f objectCoordinates{0.f, 0.f, 0.f};

      for (auto _ : state) {
        gen.template getNextN<1>(&objectCoordinates);
        benchmark::DoNotOptimize(vklComputeSample(sampler, &objectCoordinates));
      }

      // enables rates in report output
      state.SetItemsProcessed(state.iterations());
    }
  };

  namespace impl {
    template <int W, class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSample;

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSample<4, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<4> &valid,
                              VKLSampler sampler,
                              const vvec3fn<4> &coord,
                              vfloatn<4> &samples)
      {
        vklComputeSample4(reinterpret_cast<const int *>(&valid),
                          sampler,
                          reinterpret_cast<const vkl_vvec3f4 *>(&coord),
                          reinterpret_cast<float *>(&samples));
      }
    };

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSample<8, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<8> &valid,
                              VKLSampler sampler,
                              const vvec3fn<8> &coord,
                              vfloatn<8> &samples)
      {
        vklComputeSample8(reinterpret_cast<const int *>(&valid),
                          sampler,
                          reinterpret_cast<const vkl_vvec3f8 *>(&coord),
                          reinterpret_cast<float *>(&samples));
      }
    };

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSample<16, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<16> &valid,
                              VKLSampler sampler,
                              const vvec3fn<16> &coord,
                              vfloatn<16> &samples)
      {
        vklComputeSample16(reinterpret_cast<const int *>(&valid),
                           sampler,
                           reinterpret_cast<const vkl_vvec3f16 *>(&coord),
                           reinterpret_cast<float *>(&samples));
      }
    };
  }  // namespace impl

  template <int W, class VolumeWrapper, class CoordinateGenerator>
  struct VklComputeSample<programming_model::Vector<W>,
                          VolumeWrapper,
                          CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "vector" << CoordinateGenerator::name() << "Sample"
         << "<" << W;
      if (!VolumeWrapper::name().empty())
         os << ", " << VolumeWrapper::name();
      os << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      VolumeWrapper wrapper;
      VKLSampler sampler = wrapper.getSampler();
      CoordinateGenerator gen(vklGetBoundingBox(wrapper.getVolume()));

      vintn<W> valid;
      vvec3fn<W> objectCoordinates;
      vfloatn<W> samples;

      for (int i = 0; i < W; i++) {
        valid[i] = 1;
      }

      for (auto _ : state) {
        gen.template getNextV<W>(&objectCoordinates);
        impl::VklComputeSample<W, VolumeWrapper, CoordinateGenerator>::call(
            valid, sampler, objectCoordinates, samples);
      }

      // enables rates in report output
      state.SetItemsProcessed(state.iterations() * W);
    }
  };

  template <unsigned int N, class VolumeWrapper, class CoordinateGenerator>
  struct VklComputeSample<programming_model::Stream<N>,
                          VolumeWrapper,
                          CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "stream" << CoordinateGenerator::name() << "Sample"
         << "<" << N;
      if (!VolumeWrapper::name().empty())
         os << ", " << VolumeWrapper::name();
      os << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      VolumeWrapper wrapper;
      VKLSampler sampler = wrapper.getSampler();
      CoordinateGenerator gen(vklGetBoundingBox(wrapper.getVolume()));

      std::vector<vkl_vec3f> objectCoordinates(N);
      std::vector<float> samples(N);

      for (auto _ : state) {
        gen.template getNextN<N>(objectCoordinates.data());
        vklComputeSampleN(sampler, N, objectCoordinates.data(), samples.data());
      }

      // enables rates in report output
      state.SetItemsProcessed(state.iterations() * N);
    }
  };

}  // namespace api

/*
 * Register benchmarks related to vklComputeGradient*
 */
template <class VolumeWrapper, class CoordinateGenerator>
inline void registerComputeSample()
{
  using programming_model::Scalar;
  using programming_model::Stream;
  using programming_model::Vector;

  registerBenchmark<
      api::VklComputeSample<Scalar, VolumeWrapper, CoordinateGenerator>>();

  registerBenchmark<
      api::VklComputeSample<Vector<4>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<
      api::VklComputeSample<Vector<8>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<
      api::VklComputeSample<Vector<16>, VolumeWrapper, CoordinateGenerator>>();

  registerBenchmark<
      api::VklComputeSample<Stream<1>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<
      api::VklComputeSample<Stream<4>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<
      api::VklComputeSample<Stream<8>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<
      api::VklComputeSample<Stream<16>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<
      api::VklComputeSample<Stream<32>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<
      api::VklComputeSample<Stream<64>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<
      api::VklComputeSample<Stream<128>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<
      api::VklComputeSample<Stream<256>, VolumeWrapper, CoordinateGenerator>>();
}

