// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <type_traits>
#include "../common/IteratorBase.h"
#include "../common/align.h"
#include "../common/simd.h"
#include "IteratorContext.h"
#include "openvkl/openvkl.h"

using namespace rkcommon;

#define __varying_ispc_type(TypeName) \
  ispc::__vkl_concat(__vkl_concat(v, VKL_TARGET_WIDTH), _varying_##TypeName)

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct Sampler;

    /*
     * Base class for all iterators.
     */
    template <int W>
    struct Iterator : public IteratorBase
    {
      /*
       * Disallow all kinds of copying to be on the safe side.
       */
      Iterator(const Iterator &) = delete;
      Iterator &operator=(const Iterator &) = delete;
      Iterator(Iterator &&)                 = delete;
      Iterator &operator=(Iterator &&) = delete;

      // WORKAROUND ICC 15: This destructor must be public!
      virtual ~Iterator() = default;

     protected:
      explicit Iterator(const IteratorContext<W> &context) : context{&context}
      {
      }

      // Not a Ref<>! Destructors will not run.
      IteratorContext<W> const *context{nullptr};
    };

    ///////////////////////////////////////////////////////////////////////////
    // Interval iterator //////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    struct IntervalIterator : public Iterator<W>
    {
      using Iterator<W>::Iterator;

      explicit IntervalIterator(const IntervalIteratorContext<W> &context)
          : Iterator<W>(context)
      {
      }

      /*
       * Uniform path.
       * Implementing this can have substantial performance benefits, however,
       * note that we fall back to the varying code path by default.
       */
      virtual void initializeIntervalU(const vvec3fn<1> &origin,
                                       const vvec3fn<1> &direction,
                                       const vrange1fn<1> &tRange,
                                       float time)
      {
        vintn<W> validW;
        for (int i = 0; i < W; i++)
          validW[i] = i == 0 ? -1 : 0;

        vvec3fn<W> originW    = static_cast<vvec3fn<W>>(origin);
        vvec3fn<W> directionW = static_cast<vvec3fn<W>>(direction);
        vrange1fn<W> tRangeW  = static_cast<vrange1fn<W>>(tRange);
        vfloatn<W> timesW(&time, 1);

        initializeIntervalV(validW, originW, directionW, tRangeW, timesW);
      }

      virtual void iterateIntervalU(vVKLIntervalN<1> &interval,
                                    vintn<1> &result)
      {
        vintn<W> validW;
        for (int i = 0; i < W; i++)
          validW[i] = i == 0 ? -1 : 0;

        vVKLIntervalN<W> intervalW;

        vintn<W> resultW;

        iterateIntervalV(validW, intervalW, resultW);

        interval.tRange.lower[0]     = intervalW.tRange.lower[0];
        interval.tRange.upper[0]     = intervalW.tRange.upper[0];
        interval.valueRange.lower[0] = intervalW.valueRange.lower[0];
        interval.valueRange.upper[0] = intervalW.valueRange.upper[0];
        interval.nominalDeltaT[0]    = intervalW.nominalDeltaT[0];

        result[0] = resultW[0];
      }

      /*
       * Varying code path.
       */
      virtual void initializeIntervalV(const vintn<W> &valid,
                                       const vvec3fn<W> &origin,
                                       const vvec3fn<W> &direction,
                                       const vrange1fn<W> &tRange,
                                       const vfloatn<W> &times) = 0;

      virtual void iterateIntervalV(const vintn<W> &valid,
                                    vVKLIntervalN<W> &interval,
                                    vintn<W> &result) = 0;

      /*
       * This interface is used by the default hit iterator.
       */
      virtual void *getIspcStorage() = 0;
    };

/*
 * Verify max interval iterator size macros. Use this in your implementation to
 * catch inconsistencies early.
 */
#if defined(VKL_TARGET_WIDTH)

#define __vkl_verify_max_interval_iterator_size_impl(T, W)                  \
  static_assert(sizeof(T) <= alignedSize<T>() &&                            \
                    alignedSize<T>() <= VKL_MAX_INTERVAL_ITERATOR_SIZE_##W, \
                "sizeof(" #T                                                \
                ") is greater than VKL_MAX_INTERVAL_ITERATOR_SIZE_" #W);

#if (VKL_TARGET_WIDTH == 4)
#define __vkl_verify_max_interval_iterator_size(T) \
  __vkl_verify_max_interval_iterator_size_impl(T, 4)
#elif (VKL_TARGET_WIDTH == 8)
#define __vkl_verify_max_interval_iterator_size(T) \
  __vkl_verify_max_interval_iterator_size_impl(T, 8)
#elif (VKL_TARGET_WIDTH == 16)
#define __vkl_verify_max_interval_iterator_size(T) \
  __vkl_verify_max_interval_iterator_size_impl(T, 16)
#endif

#endif  // defined(VKL_TARGET_WIDTH)

    ///////////////////////////////////////////////////////////////////////////
    // Hit iterator ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    struct HitIterator : public Iterator<W>
    {
      using Iterator<W>::Iterator;

      explicit HitIterator(const HitIteratorContext<W> &context)
          : Iterator<W>(context)
      {
      }

      /*
       * Uniform path.
       * Implementing this can have substantial performance benefits, however,
       * note that we fall back to the varying code path by default.
       */

      virtual void initializeHitU(const vvec3fn<1> &origin,
                                  const vvec3fn<1> &direction,
                                  const vrange1fn<1> &tRange,
                                  float time)
      {
        vintn<W> validW;
        for (int i = 0; i < W; i++)
          validW[i] = i == 0 ? -1 : 0;

        vvec3fn<W> originW    = static_cast<vvec3fn<W>>(origin);
        vvec3fn<W> directionW = static_cast<vvec3fn<W>>(direction);
        vrange1fn<W> tRangeW  = static_cast<vrange1fn<W>>(tRange);
        vfloatn<W> timesW(&time, 1);

        initializeHitV(validW, originW, directionW, tRangeW, timesW);
      }

      virtual void iterateHitU(vVKLHitN<1> &hit, vintn<1> &result)
      {
        vintn<W> validW;
        for (int i = 0; i < W; i++)
          validW[i] = i == 0 ? -1 : 0;

        vVKLHitN<W> hitW;
        vintn<W> resultW;

        iterateHitV(validW, hitW, resultW);

        hit.t[0]       = hitW.t[0];
        hit.sample[0]  = hitW.sample[0];
        hit.epsilon[0] = hitW.epsilon[0];
        result[0]      = resultW[0];
      }

      /*
       * Varying code path.
       */

      virtual void initializeHitV(const vintn<W> &valid,
                                  const vvec3fn<W> &origin,
                                  const vvec3fn<W> &direction,
                                  const vrange1fn<W> &tRange,
                                  const vfloatn<W> &times) = 0;

      virtual void iterateHitV(const vintn<W> &valid,
                               vVKLHitN<W> &hit,
                               vintn<W> &result) = 0;
    };

/*
 * Verify max hit iterator size macros. Use this in your implementation to
 * catch inconsistencies early.
 */
#if defined(VKL_TARGET_WIDTH)

#define __vkl_verify_max_hit_iterator_size_impl(T, W)                  \
  static_assert(sizeof(T) <= alignedSize<T>() &&                       \
                    alignedSize<T>() <= VKL_MAX_HIT_ITERATOR_SIZE_##W, \
                "sizeof(" #T                                           \
                ") is greater than VKL_MAX_HIT_ITERATOR_SIZE_" #W);

#if (VKL_TARGET_WIDTH == 4)
#define __vkl_verify_max_hit_iterator_size(T) \
  __vkl_verify_max_hit_iterator_size_impl(T, 4)
#elif (VKL_TARGET_WIDTH == 8)
#define __vkl_verify_max_hit_iterator_size(T) \
  __vkl_verify_max_hit_iterator_size_impl(T, 8)
#elif (VKL_TARGET_WIDTH == 16)
#define __vkl_verify_max_hit_iterator_size(T) \
  __vkl_verify_max_hit_iterator_size_impl(T, 16)
#endif

#endif  // defined(VKL_TARGET_WIDTH)

    ///////////////////////////////////////////////////////////////////////////
    // Iterator factory ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    /*
     * Interface for iterator factories.
     * Note that the uniform interfaces attempt to fall back to the varying
     * implementation to reduce implementation burden. However, samplers should
     * consider implementing both varying and uniform code paths for maximum
     * performance.
     */
    template <int W,
              template <int>
              class IteratorT,
              template <int>
              class ContextT>
    struct IteratorFactory
    {
      virtual ~IteratorFactory() = default;

      /*
       * Constructs a new context.
       */
      virtual ContextT<W> *newContext(const Sampler<W> &sampler) const = 0;

      /*
       * Construct a new varying iterator into the provided buffer.
       */
      virtual IteratorT<W> *constructV(const ContextT<W> &context,
                                       void *buffer) const = 0;

      /*
       * Retrieve the size, in bytes, required for a buffer to be able to
       * store a varying iterator.
       *
       * This includes all padding that may be needed for alignment, and both
       * the storeage required for the iterator itself and its internal state.
       */
      virtual size_t sizeV() const = 0;

      /*
       * Construct a new uniform iterator into the provided buffer.
       */
      virtual IteratorT<W> *constructU(const ContextT<W> &context,
                                       void *buffer) const = 0;

      /*
       * Retrieve the size, in bytes, required for a buffer to be able to
       * store a uniform iterator.
       *
       * This includes all padding that may be needed for alignment, and both
       * the storeage required for the iterator itself and its internal state.
       */
      virtual size_t sizeU() const = 0;
    };

    /*
     * Concrete iterator factory. Derive from this to implement a sampler's
     * iterator factory.
     */
    template <int W,
              template <int>
              class IteratorBaseT,
              template <int>
              class IteratorT,
              template <int>
              class ContextBaseT,
              template <int>
              class ContextT>
    struct ConcreteIteratorFactory
        : public IteratorFactory<W, IteratorBaseT, ContextT>
    {
      static_assert(std::is_base_of<IteratorBaseT<W>, IteratorT<W>>::value &&
                        std::is_base_of<ContextBaseT<W>, ContextT<W>>::value,
                    "ConcreteIteratorFactory used with incompatible types.");

      ContextT<W> *newContext(const Sampler<W> &sampler) const override final
      {
        return new ContextT<W>(sampler);
      }

      IteratorBaseT<W> *constructV(const ContextBaseT<W> &context,
                                   void *buffer) const override final
      {
        return new (align<IteratorT<W>>(buffer)) IteratorT<W>(context);
      }

      size_t sizeV() const override final
      {
        return alignedSize<IteratorT<W>>();
      }

      IteratorBaseT<W> *constructU(const ContextBaseT<W> &context,
                                   void *buffer) const override final
      {
        return new (align<IteratorT<W>>(buffer)) IteratorT<W>(context);
      }

      size_t sizeU() const override final
      {
        return alignedSize<IteratorT<W>>();
      }
    };

  }  // namespace cpu_device
}  // namespace openvkl
