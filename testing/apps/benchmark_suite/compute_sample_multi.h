// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "utility.h"

/*
 * Benchmark wrappers for vklComputeSampleM* (multi-attribute) functions.
 */

namespace api {

  using openvkl::vintn;
  using openvkl::vvec3fn;

  using openvkl::testing::getAttributeIndices;

  template <class ProgrammingModel,
            class VolumeWrapper,
            class CoordinateGenerator>
  struct VklComputeSampleM;

  template <class ProgrammingModel,
            class VolumeWrapper,
            class CoordinateGenerator>
  struct VklComputeSampleSeqM;

  // Scalar ///////////////////////////////////////////////////////////////////

  template <unsigned int M, class VolumeWrapper, class CoordinateGenerator>
  struct VklComputeSampleM<programming_model::ScalarM<M>,
                           VolumeWrapper,
                           CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "scalar" << CoordinateGenerator::name() << "SampleM";
      os << "<" << M;
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

      vkl_vec3f objectCoordinates{0.f, 0.f, 0.f};

      std::vector<float> samples(M);

      std::vector<unsigned int> attributeIndices = getAttributeIndices(M);

      for (auto _ : state) {
        gen.template getNextN<1>(&objectCoordinates);
        vklComputeSampleM(sampler,
                          &objectCoordinates,
                          samples.data(),
                          M,
                          attributeIndices.data());
      }

      // enables rates in report output
      state.SetItemsProcessed(M * state.iterations());
    }
  };

  // Scalar (sequential per attribute) ////////////////////////////////////////

  template <unsigned int M, class VolumeWrapper, class CoordinateGenerator>
  struct VklComputeSampleSeqM<programming_model::ScalarM<M>,
                              VolumeWrapper,
                              CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "scalar" << CoordinateGenerator::name() << "SampleSeqM";
      os << "<" << M;
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

      vkl_vec3f objectCoordinates{0.f, 0.f, 0.f};

      std::vector<float> samples(M);

      std::vector<unsigned int> attributeIndices = getAttributeIndices(M);

      for (auto _ : state) {
        gen.template getNextN<1>(&objectCoordinates);
        for (unsigned int a = 0; a < M; a++) {
          samples[a] = vklComputeSample(
              sampler, &objectCoordinates, attributeIndices[a]);
        }
      }

      // enables rates in report output
      state.SetItemsProcessed(M * state.iterations());
    }
  };

  // Vector ///////////////////////////////////////////////////////////////////

  namespace impl {
    template <int W, class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSampleM;

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSampleM<4, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<4> &valid,
                              VKLSampler sampler,
                              const vvec3fn<4> &coord,
                              std::vector<float> &samples,
                              unsigned int M,
                              const std::vector<unsigned int> &attributeIndices)
      {
        vklComputeSampleM4(reinterpret_cast<const int *>(&valid),
                           sampler,
                           reinterpret_cast<const vkl_vvec3f4 *>(&coord),
                           samples.data(),
                           M,
                           attributeIndices.data());
      }
    };

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSampleM<8, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<8> &valid,
                              VKLSampler sampler,
                              const vvec3fn<8> &coord,
                              std::vector<float> &samples,
                              unsigned int M,
                              const std::vector<unsigned int> &attributeIndices)
      {
        vklComputeSampleM8(reinterpret_cast<const int *>(&valid),
                           sampler,
                           reinterpret_cast<const vkl_vvec3f8 *>(&coord),
                           samples.data(),
                           M,
                           attributeIndices.data());
      }
    };

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSampleM<16, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<16> &valid,
                              VKLSampler sampler,
                              const vvec3fn<16> &coord,
                              std::vector<float> &samples,
                              unsigned int M,
                              const std::vector<unsigned int> &attributeIndices)
      {
        vklComputeSampleM16(reinterpret_cast<const int *>(&valid),
                            sampler,
                            reinterpret_cast<const vkl_vvec3f16 *>(&coord),
                            samples.data(),
                            M,
                            attributeIndices.data());
      }
    };
  }  // namespace impl

  template <unsigned int M,
            int W,
            class VolumeWrapper,
            class CoordinateGenerator>
  struct VklComputeSampleM<programming_model::VectorM<M, W>,
                           VolumeWrapper,
                           CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "vector" << CoordinateGenerator::name() << "SampleM"
         << "<" << M << ", " << W;
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

      std::vector<float> samples(M * W);

      const std::vector<unsigned int> attributeIndices = getAttributeIndices(M);

      for (int i = 0; i < W; i++) {
        valid[i] = 1;
      }

      for (auto _ : state) {
        gen.template getNextV<W>(&objectCoordinates);
        impl::VklComputeSampleM<W, VolumeWrapper, CoordinateGenerator>::call(
            valid, sampler, objectCoordinates, samples, M, attributeIndices);
      }

      // enables rates in report output
      state.SetItemsProcessed(M * state.iterations() * W);
    }
  };

  // Vector (sequential per attribute) ////////////////////////////////////////

  namespace impl {
    template <int W, class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSampleSeqM;

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSampleSeqM<4, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<4> &valid,
                              VKLSampler sampler,
                              const vvec3fn<4> &coord,
                              std::vector<float> &samples,
                              unsigned int M,
                              const std::vector<unsigned int> &attributeIndices)
      {
        for (unsigned int a = 0; a < M; a++) {
          vklComputeSample4(reinterpret_cast<const int *>(&valid),
                            sampler,
                            reinterpret_cast<const vkl_vvec3f4 *>(&coord),
                            samples.data() + a * 4,
                            attributeIndices[a]);
        }
      }
    };

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSampleSeqM<8, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<8> &valid,
                              VKLSampler sampler,
                              const vvec3fn<8> &coord,
                              std::vector<float> &samples,
                              unsigned int M,
                              const std::vector<unsigned int> &attributeIndices)
      {
        for (unsigned int a = 0; a < M; a++) {
          vklComputeSample8(reinterpret_cast<const int *>(&valid),
                            sampler,
                            reinterpret_cast<const vkl_vvec3f8 *>(&coord),
                            samples.data() + a * 8,
                            attributeIndices[a]);
        }
      }
    };

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSampleSeqM<16, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<16> &valid,
                              VKLSampler sampler,
                              const vvec3fn<16> &coord,
                              std::vector<float> &samples,
                              unsigned int M,
                              const std::vector<unsigned int> &attributeIndices)
      {
        for (unsigned int a = 0; a < M; a++) {
          vklComputeSample16(reinterpret_cast<const int *>(&valid),
                             sampler,
                             reinterpret_cast<const vkl_vvec3f16 *>(&coord),
                             samples.data() + a * 16,
                             attributeIndices[a]);
        }
      }
    };
  }  // namespace impl

  template <unsigned int M,
            int W,
            class VolumeWrapper,
            class CoordinateGenerator>
  struct VklComputeSampleSeqM<programming_model::VectorM<M, W>,
                              VolumeWrapper,
                              CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "vector" << CoordinateGenerator::name() << "SampleSeqM"
         << "<" << M << ", " << W;
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

      std::vector<float> samples(M * W);

      const std::vector<unsigned int> attributeIndices = getAttributeIndices(M);

      for (int i = 0; i < W; i++) {
        valid[i] = 1;
      }

      for (auto _ : state) {
        gen.template getNextV<W>(&objectCoordinates);
        impl::VklComputeSampleSeqM<W, VolumeWrapper, CoordinateGenerator>::call(
            valid, sampler, objectCoordinates, samples, M, attributeIndices);
      }

      // enables rates in report output
      state.SetItemsProcessed(M * state.iterations() * W);
    }
  };

  // Stream ///////////////////////////////////////////////////////////////////

  template <unsigned int M,
            unsigned int N,
            class VolumeWrapper,
            class CoordinateGenerator>
  struct VklComputeSampleM<programming_model::StreamM<M, N>,
                           VolumeWrapper,
                           CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "stream" << CoordinateGenerator::name() << "SampleM"
         << "<" << M << ", " << N;
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
      std::vector<float> samples(M * N);

      const std::vector<unsigned int> attributeIndices = getAttributeIndices(M);

      for (auto _ : state) {
        gen.template getNextN<N>(objectCoordinates.data());
        vklComputeSampleMN(sampler,
                           N,
                           objectCoordinates.data(),
                           samples.data(),
                           M,
                           attributeIndices.data());
      }

      // enables rates in report output
      state.SetItemsProcessed(M * state.iterations() * N);
    }
  };

  // Stream (sequential per attribute) ////////////////////////////////////////

  template <unsigned int M,
            unsigned int N,
            class VolumeWrapper,
            class CoordinateGenerator>
  struct VklComputeSampleSeqM<programming_model::StreamM<M, N>,
                              VolumeWrapper,
                              CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "stream" << CoordinateGenerator::name() << "SampleSeqM"
         << "<" << M << ", " << N;
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
      std::vector<float> samples(M * N);

      const std::vector<unsigned int> attributeIndices = getAttributeIndices(M);

      for (auto _ : state) {
        gen.template getNextN<N>(objectCoordinates.data());
        for (unsigned int a = 0; a < M; a++) {
          vklComputeSampleN(sampler,
                            N,
                            objectCoordinates.data(),
                            samples.data() + a * N,
                            attributeIndices[a]);
        }
      }

      // enables rates in report output
      state.SetItemsProcessed(M * state.iterations() * N);
    }
  };

}  // namespace api

/*
 * Register benchmarks related to vklComputeGradient*
 */
template <class VolumeWrapper, class CoordinateGenerator>
inline void registerComputeSampleMulti()
{
  using programming_model::ScalarM;
  using programming_model::StreamM;
  using programming_model::VectorM;

  constexpr unsigned int M = VolumeWrapper::getNumAttributes();

  // scalar (sequential)
  registerBenchmark<api::VklComputeSampleSeqM<ScalarM<M>,
                                              VolumeWrapper,
                                              CoordinateGenerator>>();

  // scalar (multi)
  registerBenchmark<
      api::VklComputeSampleM<ScalarM<M>, VolumeWrapper, CoordinateGenerator>>();

  // vector (sequential)
  registerBenchmark<api::VklComputeSampleSeqM<VectorM<M, 4>,
                                              VolumeWrapper,
                                              CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleSeqM<VectorM<M, 8>,
                                              VolumeWrapper,
                                              CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleSeqM<VectorM<M, 16>,
                                              VolumeWrapper,
                                              CoordinateGenerator>>();

  // vector (multi)
  registerBenchmark<api::VklComputeSampleM<VectorM<M, 4>,
                                           VolumeWrapper,
                                           CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleM<VectorM<M, 8>,
                                           VolumeWrapper,
                                           CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleM<VectorM<M, 16>,
                                           VolumeWrapper,
                                           CoordinateGenerator>>();

  // stream (sequential)
  registerBenchmark<api::VklComputeSampleSeqM<StreamM<M, 1>,
                                              VolumeWrapper,
                                              CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleSeqM<StreamM<M, 4>,
                                              VolumeWrapper,
                                              CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleSeqM<StreamM<M, 8>,
                                              VolumeWrapper,
                                              CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleSeqM<StreamM<M, 16>,
                                              VolumeWrapper,
                                              CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleSeqM<StreamM<M, 32>,
                                              VolumeWrapper,
                                              CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleSeqM<StreamM<M, 64>,
                                              VolumeWrapper,
                                              CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleSeqM<StreamM<M, 128>,
                                              VolumeWrapper,
                                              CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleSeqM<StreamM<M, 256>,
                                              VolumeWrapper,
                                              CoordinateGenerator>>();

  // stream (multi)
  registerBenchmark<api::VklComputeSampleM<StreamM<M, 1>,
                                           VolumeWrapper,
                                           CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleM<StreamM<M, 4>,
                                           VolumeWrapper,
                                           CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleM<StreamM<M, 8>,
                                           VolumeWrapper,
                                           CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleM<StreamM<M, 16>,
                                           VolumeWrapper,
                                           CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleM<StreamM<M, 32>,
                                           VolumeWrapper,
                                           CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleM<StreamM<M, 64>,
                                           VolumeWrapper,
                                           CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleM<StreamM<M, 128>,
                                           VolumeWrapper,
                                           CoordinateGenerator>>();
  registerBenchmark<api::VklComputeSampleM<StreamM<M, 256>,
                                           VolumeWrapper,
                                           CoordinateGenerator>>();
}
