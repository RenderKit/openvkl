// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/ManagedObject.h"
#include "../common/export_util.h"
#include "../common/objectFactory.h"
#include "../iterator/Iterator.h"
#include "../sampler/Sampler.h"
#include "../value_selector/ValueSelector.h"
#include "Volume_ispc.h"
#include "openvkl/openvkl.h"
#include "rkcommon/math/box.h"

#define THROW_NOT_IMPLEMENTED                          \
  throw std::runtime_error(std::string(__FUNCTION__) + \
                           " not implemented in this volume!")

using namespace rkcommon;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct Volume : public ManagedObject
    {
      Volume()                   = default;
      virtual ~Volume() override = default;

      static Volume *createInstance(const std::string &type);

      /*
       * Return the iterator factory for this volume.
       */
      virtual const IteratorFactory<W, IntervalIterator>
          &getIntervalIteratorFactory() const = 0;

      virtual const IteratorFactory<W, HitIterator> &getHitIteratorFactory()
          const = 0;

      virtual ValueSelector<W> *newValueSelector();

      virtual Sampler<W> *newSampler() = 0;

      virtual box3f getBoundingBox() const = 0;

      virtual unsigned int getNumAttributes() const = 0;

      virtual range1f getValueRange() const = 0;

      void *getISPCEquivalent() const;

      virtual Observer<W> *newObserver(const char *type)
      {
        return nullptr;
      }

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
    inline ValueSelector<W> *Volume<W>::newValueSelector()
    {
      return new ValueSelector<W>(this);
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
