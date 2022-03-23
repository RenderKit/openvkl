// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/ManagedObject.h"
#include "../common/ObjectFactory.h"
#include "../common/export_util.h"
#include "../iterator/Iterator.h"
#include "../sampler/Sampler.h"
#include "Volume_ispc.h"
#include "openvkl/openvkl.h"
#include "rkcommon/math/box.h"
#include "openvkl/common/StructShared.h"
#include "VolumeShared.h"

#define THROW_NOT_IMPLEMENTED                          \
  throw std::runtime_error(std::string(__FUNCTION__) + \
                           " not implemented in this volume!")

using namespace rkcommon;

namespace openvkl {
  namespace cpu_device {

    // Helpers ////////////////////////////////////////////////////////////////

    // this helper may be used on both signed and unsigned int types
    template <int W, typename INT_TYPE>
    inline void throwOnIllegalAttributeIndex(const Volume<W> *volume,
                                             INT_TYPE attributeIndex)
    {
      if (attributeIndex < 0 || attributeIndex >= volume->getNumAttributes()) {
        throw std::runtime_error("illegal attributeIndex requested on volume");
      }
    }

    // Volume /////////////////////////////////////////////////////////////////

    template <int W>
    struct Volume : public AddStructShared<ManagedObject, ispc::VolumeShared>
    {
      Volume() {}  // not = default, due to ICC 19 compiler bug
      virtual ~Volume() override = default;

      static Volume *createInstance(Device *device, const std::string &type);
      static void registerType(const std::string &type,
                               FactoryFcn<Volume<W>> f);

      virtual Sampler<W> *newSampler() = 0;

      virtual box3f getBoundingBox() const = 0;

      virtual unsigned int getNumAttributes() const = 0;

      virtual range1f getValueRange(unsigned int attributeIndex) const = 0;

      virtual Observer<W> *newObserver(const char *type)
      {
        return nullptr;
      }

     protected:
      static ObjectFactory<Volume, VKL_VOLUME> volumeFactory;

      bool SharedStructInitialized = false;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline Volume<W> *Volume<W>::createInstance(Device *device,
                                                const std::string &type)
    {
      return createInstanceHelper<Volume<W>, VKL_VOLUME>(
          device, type, volumeFactory);
    }

    template <int W>
    inline void Volume<W>::registerType(const std::string &type,
                                        FactoryFcn<Volume<W>> f)
    {
      volumeFactory.registerType(type, f);
    }

    template <int W>
    ObjectFactory<Volume<W>, VKL_VOLUME> Volume<W>::volumeFactory;

#define VKL_REGISTER_VOLUME(InternalClass, external_name) \
  VKL_REGISTER_OBJECT(                                    \
      ::openvkl::ManagedObject, volume, InternalClass, external_name)

  }  // namespace cpu_device
}  // namespace openvkl
