// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <vector>
#include "utility.h"

/*
 * Benchmark wrappers for interval iterator functions.
 */

namespace api {

  template <class VolumeWrapper>
  struct IntervalIteratorConstruction
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "scalarIntervalIteratorConstruction";
      if (!VolumeWrapper::name().empty())
        os << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      static std::unique_ptr<VolumeWrapper> wrapper;
      static VKLVolume vklVolume;
      static vkl_vec3f origin;
      static const vkl_vec3f direction{0.f, 0.f, 1.f};
      static const vkl_range1f tRange{0.f, 1000.f};
      static size_t intervalIteratorSize { 0 };
      static std::vector<char> buffers;

      // This is safe: Google benchmark guarantees that threads do not
      // start running until the start of the loop below.
      if (state.thread_index == 0)
      {
        wrapper = rkcommon::make_unique<VolumeWrapper>();
        vklVolume            = wrapper->getVolume();
        const vkl_box3f bbox = vklGetBoundingBox(vklVolume);

        std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
        std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

        std::random_device rd;
        std::mt19937 eng(rd());
        origin = vkl_vec3f{distX(eng), distY(eng), -1.f};

        intervalIteratorSize = vklGetIntervalIteratorSize(vklVolume);
        buffers.resize(intervalIteratorSize * state.threads);
      }

      for (auto _ : state) {
        void *buffer = buffers.data() + intervalIteratorSize * state.thread_index;
        VKLIntervalIterator iterator = vklInitIntervalIterator(
            vklVolume, &origin, &direction, &tRange, nullptr, buffer);

        benchmark::DoNotOptimize(iterator);
      }

      if (state.thread_index == 0)
        wrapper.reset();

      // enables rates in report output
      state.SetItemsProcessed(state.iterations());
    }
  };

  template <class VolumeWrapper>
  struct IntervalIteratorIterateFirst
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "scalarIntervalIteratorIterateFirst";
      if (!VolumeWrapper::name().empty())
        os << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      static std::unique_ptr<VolumeWrapper> wrapper;
      static VKLVolume vklVolume;
      static vkl_vec3f origin;
      static const vkl_vec3f direction{0.f, 0.f, 1.f};
      static const vkl_range1f tRange{0.f, 1000.f};
      static size_t intervalIteratorSize { 0 };
      static std::vector<char> buffers;

      if (state.thread_index == 0)
      {
        wrapper              = rkcommon::make_unique<VolumeWrapper>();
        vklVolume            = wrapper->getVolume();
        const vkl_box3f bbox = vklGetBoundingBox(vklVolume);

        std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
        std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

        std::random_device rd;
        std::mt19937 eng(rd());

        origin = vkl_vec3f{distX(eng), distY(eng), -1.f};
        vkl_vec3f direction{0.f, 0.f, 1.f};
        vkl_range1f tRange{0.f, 1000.f};

        intervalIteratorSize = vklGetIntervalIteratorSize(vklVolume);
        buffers.resize(intervalIteratorSize * state.threads);
      }

      VKLInterval interval;
      std::vector<char> buffer;

      for (auto _ : state) {
        void *buffer = buffers.data() + intervalIteratorSize * state.thread_index;
        VKLIntervalIterator iterator = vklInitIntervalIterator(
            vklVolume, &origin, &direction, &tRange, nullptr, buffer);

        bool success = vklIterateInterval(iterator, &interval);

        if (!success) {
          throw std::runtime_error("vklIterateInterval() returned false");
        }

        benchmark::DoNotOptimize(interval);
      }

      // global teardown only in first thread
      if (state.thread_index == 0)
        wrapper.reset();

      // enables rates in report output
      state.SetItemsProcessed(state.iterations());
    }
  };

  template <class VolumeWrapper>
  struct IntervalIteratorIterateSecond
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "scalarIntervalIteratorIterateSecond";
      if (!VolumeWrapper::name().empty())
        os << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      static std::unique_ptr<VolumeWrapper> wrapper;
      static VKLVolume vklVolume;
      static vkl_vec3f origin;
      static const vkl_vec3f direction{0.f, 0.f, 1.f};
      static const vkl_range1f tRange{0.f, 1000.f};
      static size_t intervalIteratorSize { 0 };
      static std::vector<char> buffers;

      if (state.thread_index == 0)
      {
        wrapper = rkcommon::make_unique<VolumeWrapper>();
        vklVolume            = wrapper->getVolume();
        const vkl_box3f bbox = vklGetBoundingBox(vklVolume);

        std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
        std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

        std::random_device rd;
        std::mt19937 eng(rd());
        origin = vkl_vec3f{distX(eng), distY(eng), -1.f};

        intervalIteratorSize = vklGetIntervalIteratorSize(vklVolume);
        buffers.resize(intervalIteratorSize * state.threads);
      }

      VKLInterval interval;
      for (auto _ : state) {
        void *buffer = buffers.data() + intervalIteratorSize * state.thread_index;
        VKLIntervalIterator iterator = vklInitIntervalIterator(
            vklVolume, &origin, &direction, &tRange, nullptr, buffer);

        vklIterateInterval(iterator, &interval);

        bool success = vklIterateInterval(iterator, &interval);

        if (!success) {
          throw std::runtime_error("vklIterateInterval() returned false");
        }

        benchmark::DoNotOptimize(interval);
      }

      if (state.thread_index == 0)
        wrapper.reset();

      // enables rates in report output
      state.SetItemsProcessed(state.iterations());
    }
  };

}  // namespace api

/*
 * Register interval iterator tests.
 */
template <class VolumeWrapper>
inline void registerIntervalIterators()
{
  using Construction = api::IntervalIteratorConstruction<VolumeWrapper>;
  registerBenchmark<Construction>()->UseRealTime();
  registerBenchmark<Construction>()->Threads(2)->UseRealTime();
  registerBenchmark<Construction>()->Threads(4)->UseRealTime();
  registerBenchmark<Construction>()->Threads(6)->UseRealTime();
  registerBenchmark<Construction>()->Threads(12)->UseRealTime();
  registerBenchmark<Construction>()->Threads(36)->UseRealTime();
  registerBenchmark<Construction>()->Threads(72)->UseRealTime();

  using IterateFirst = api::IntervalIteratorIterateFirst<VolumeWrapper>;
  registerBenchmark<IterateFirst>()->UseRealTime();
  registerBenchmark<IterateFirst>()->Threads(2)->UseRealTime();
  registerBenchmark<IterateFirst>()->Threads(4)->UseRealTime();
  registerBenchmark<IterateFirst>()->Threads(6)->UseRealTime();
  registerBenchmark<IterateFirst>()->Threads(12)->UseRealTime();
  registerBenchmark<IterateFirst>()->Threads(36)->UseRealTime();
  registerBenchmark<IterateFirst>()->Threads(72)->UseRealTime();

  using IterateSecond = api::IntervalIteratorIterateSecond<VolumeWrapper>;
  registerBenchmark<IterateSecond>()->UseRealTime();
  registerBenchmark<IterateSecond>()->Threads(2)->UseRealTime();
  registerBenchmark<IterateSecond>()->Threads(4)->UseRealTime();
  registerBenchmark<IterateSecond>()->Threads(6)->UseRealTime();
  registerBenchmark<IterateSecond>()->Threads(12)->UseRealTime();
  registerBenchmark<IterateSecond>()->Threads(36)->UseRealTime();
  registerBenchmark<IterateSecond>()->Threads(72)->UseRealTime();
}

