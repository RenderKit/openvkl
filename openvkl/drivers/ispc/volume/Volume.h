// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "../common/ManagedObject.h"
#include "../common/objectFactory.h"
#include "../iterator/DefaultIterator.h"
#include "../value_selector/ValueSelector.h"
#include "openvkl/openvkl.h"
#include "ospcommon/math/box.h"

#define THROW_NOT_IMPLEMENTED                          \
  throw std::runtime_error(std::string(__FUNCTION__) + \
                           " not implemented in this volume!")

using namespace ospcommon;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct Volume : public ManagedObject
    {
      Volume()                   = default;
      virtual ~Volume() override = default;

      static Volume *createInstance(const std::string &type);

      // volumes must provide their own iterator implementations based on
      // their internal acceleration structures.

      // initialize a new iterator for the given input rays (specified by
      // origin, direction and tRange) and optional valueSelector indicating
      // volume sample values of interest. if no valueSelector is provided, all
      // intervals intersecting the volume should be (iteratively) returned by
      // iterateIntervalV(), and no hits should be returned by iterateHitV().
      //
      // the iterator objects can be casted to volume-specific iterator types,
      // and may maintain internal state as desired, e.g. for current state
      // within an acceleration structure, etc.

      virtual void initIntervalIteratorV(const vintn<W> &valid,
                                         vVKLIntervalIteratorN<W> &iterator,
                                         const vvec3fn<W> &origin,
                                         const vvec3fn<W> &direction,
                                         const vrange1fn<W> &tRange,
                                         const ValueSelector<W> *valueSelector);

      // for each active lane / ray (indicated by valid), iterate once for the
      // given iterator and return the next interval (if any) satisfying the
      // iterator's valueSelector in interval. result (0 or 1) should
      // indicate if a new interval was found for each active lane.
      //
      // iterator may be modified to track any internal state as desired.
      virtual void iterateIntervalV(const vintn<W> &valid,
                                    vVKLIntervalIteratorN<W> &iterator,
                                    vVKLIntervalN<W> &interval,
                                    vintn<W> &result);

      virtual void initHitIteratorV(const vintn<W> &valid,
                                    vVKLHitIteratorN<W> &iterator,
                                    const vvec3fn<W> &origin,
                                    const vvec3fn<W> &direction,
                                    const vrange1fn<W> &tRange,
                                    const ValueSelector<W> *valueSelector);

      // for each active lane / ray (indicated by valid), iterate once for the
      // given iterator and return the next hit (if any) satisfying
      // the iterator's valueSelector in hit. result (0 or 1) should
      // indicate if a new hit was found for each active lane.
      //
      // iterator may be modified to track any internal state as desired.
      virtual void iterateHitV(const vintn<W> &valid,
                               vVKLHitIteratorN<W> &iterator,
                               vVKLHitN<W> &hit,
                               vintn<W> &result);

      virtual ValueSelector<W> *newValueSelector();

      // volumes can optionally define a scalar sampling method; if not
      // defined then the default implementation will use computeSampleV()
      virtual void computeSample(const vvec3fn<1> &objectCoordinates,
                                 vfloatn<1> &samples) const;

      virtual void computeSampleV(const vintn<W> &valid,
                                  const vvec3fn<W> &objectCoordinates,
                                  vfloatn<W> &samples) const = 0;

      virtual void computeGradientV(const vintn<W> &valid,
                                    const vvec3fn<W> &objectCoordinates,
                                    vvec3fn<W> &gradients) const;

      virtual box3f getBoundingBox() const = 0;

      virtual range1f getValueRange() const = 0;

      void *getISPCEquivalent() const;

     protected:
      void *ispcEquivalent{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline Volume<W> *Volume<W>::createInstance(const std::string &type)
    {
      return createInstanceHelper<Volume<W>, VKL_VOLUME>(type);
    }

    template <int W>
    inline void Volume<W>::initIntervalIteratorV(
        const vintn<W> &valid,
        vVKLIntervalIteratorN<W> &iterator,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      iterator = toVKLIntervalIterator<W>(DefaultIterator<W>(
          valid, this, origin, direction, tRange, valueSelector));
    }

    template <int W>
    inline void Volume<W>::iterateIntervalV(const vintn<W> &valid,
                                            vVKLIntervalIteratorN<W> &iterator,
                                            vVKLIntervalN<W> &interval,
                                            vintn<W> &result)
    {
      DefaultIterator<W> *i =
          fromVKLIntervalIterator<DefaultIterator<W>>(&iterator);

      i->iterateInterval(valid, result);

      interval =
          *reinterpret_cast<const vVKLIntervalN<W> *>(i->getCurrentInterval());
    }

    template <int W>
    inline void Volume<W>::initHitIteratorV(
        const vintn<W> &valid,
        vVKLHitIteratorN<W> &iterator,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      iterator = toVKLHitIterator<W>(DefaultIterator<W>(
          valid, this, origin, direction, tRange, valueSelector));
    }

    template <int W>
    inline void Volume<W>::iterateHitV(const vintn<W> &valid,
                                       vVKLHitIteratorN<W> &iterator,
                                       vVKLHitN<W> &hit,
                                       vintn<W> &result)
    {
      DefaultIterator<W> *i = fromVKLHitIterator<DefaultIterator<W>>(&iterator);

      i->iterateHit(valid, result);

      hit = *reinterpret_cast<const vVKLHitN<W> *>(i->getCurrentHit());
    }

    template <int W>
    inline ValueSelector<W> *Volume<W>::newValueSelector()
    {
      return new ValueSelector<W>(this);
    }

    template <int W>
    inline void Volume<W>::computeSample(const vvec3fn<1> &objectCoordinates,
                                         vfloatn<1> &sample) const
    {
      // gracefully degrade to use computeSampleV(); see
      // ISPCDriver<W>::computeSampleAnyWidth()

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i == 0 ? 1 : 0;

      ocW.fill_inactive_lanes(validW);

      vfloatn<W> samplesW;

      computeSampleV(validW, ocW, samplesW);

      sample[0] = samplesW[0];
    }

    template <int W>
    inline void Volume<W>::computeGradientV(const vintn<W> &valid,
                                            const vvec3fn<W> &objectCoordinates,
                                            vvec3fn<W> &gradients) const
    {
      THROW_NOT_IMPLEMENTED;
    }

    template <int W>
    inline void *Volume<W>::getISPCEquivalent() const
    {
      return ispcEquivalent;
    }

#define VKL_REGISTER_VOLUME(InternalClass, external_name) \
  VKL_REGISTER_OBJECT(                                    \
      ::openvkl::ManagedObject, volume, InternalClass, external_name)

  }  // namespace ispc_driver
}  // namespace openvkl
