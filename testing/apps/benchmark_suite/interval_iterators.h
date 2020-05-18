// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#pragma once

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
      os << "scalarIntervalIteratorConstruction"
         << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      static std::unique_ptr<VolumeWrapper> wrapper;
      static VKLVolume vklVolume;
      static VKLIntervalIterator iterator;
      static vkl_vec3f origin;

      if (state.thread_index == 0) {
        wrapper = ospcommon::make_unique<VolumeWrapper>();

        vklVolume            = wrapper->getVolume();
        const vkl_box3f bbox = vklGetBoundingBox(vklVolume);

        std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
        std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

        std::random_device rd;
        std::mt19937 eng(rd());
        origin = vkl_vec3f{distX(eng), distY(eng), -1.f};
      }

      vkl_vec3f direction{0.f, 0.f, 1.f};
      vkl_range1f tRange{0.f, 1000.f};

      for (auto _ : state) {
        VKLIntervalIterator iterator;
        vklInitIntervalIterator(
            &iterator, vklVolume, &origin, &direction, &tRange, nullptr);

        benchmark::DoNotOptimize(iterator);
      }

      // global teardown only in first thread
      if (state.thread_index == 0) {
        wrapper.reset();
      }

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
      os << "scalarIntervalIteratorIterateFirst"
         << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      static std::unique_ptr<VolumeWrapper> wrapper;
      static VKLVolume vklVolume;
      static VKLIntervalIterator iterator;

      if (state.thread_index == 0) {
        wrapper = ospcommon::make_unique<VolumeWrapper>();

        vklVolume            = wrapper->getVolume();
        const vkl_box3f bbox = vklGetBoundingBox(vklVolume);

        std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
        std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

        std::random_device rd;
        std::mt19937 eng(rd());

        vkl_vec3f origin{distX(eng), distY(eng), -1.f};
        vkl_vec3f direction{0.f, 0.f, 1.f};
        vkl_range1f tRange{0.f, 1000.f};

        vklInitIntervalIterator(
            &iterator, vklVolume, &origin, &direction, &tRange, nullptr);
      }

      VKLInterval interval;

      for (auto _ : state) {
        VKLIntervalIterator iteratorTemp = iterator;

        bool success = vklIterateInterval(&iteratorTemp, &interval);

        if (!success) {
          throw std::runtime_error("vklIterateInterval() returned false");
        }

        benchmark::DoNotOptimize(interval);
      }

      // global teardown only in first thread
      if (state.thread_index == 0) {
        wrapper.reset();
      }

      // enables rates in report output
      state.SetItemsProcessed(state.iterations());
    }
  };

} // namespace api

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
}

