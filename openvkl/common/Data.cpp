// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
using namespace rkcommon;
using namespace rkcommon::math;

#include "../devices/common/BufferShared.h"
#include "Data.h"
#include "rkcommon/memory/malloc.h"

namespace openvkl {

  ispc::Data1D Data::emptyData1D;

  Data::Data(Device *d,
             size_t numItems,
             VKLDataType dataType,
             const void *source,
             VKLDataCreationFlags dataCreationFlags,
             size_t _byteStride)
      : ManagedObject(d),
        numItems(numItems),
        dataType(dataType),
        dataCreationFlags(dataCreationFlags),
        byteStride(_byteStride)
  {
    if (numItems == 0) {
      throw std::out_of_range("VKLData: numItems must be positive");
    }

    if (!source) {
      throw std::runtime_error("VKLData: source cannot be NULL");
    }

    // compute stride if requested
    if (byteStride == 0) {
      if (isManagedObject(dataType)) {
        // sizeOf(dataType) represents the _stored_ byte stride (we store only
        // ManagedObject *), but the source byte stride is for the full
        // user-provided VKLObject(s)
        byteStride = sizeof(VKLObject);
      } else {
        byteStride = sizeOf(dataType);
      }
    }

    if (dataCreationFlags == VKL_DATA_DEFAULT) {
      // copy source data into naturally-strided (compact) array
      const size_t naturalByteStride = sizeOf(dataType);
      const size_t numBytes          = numItems * naturalByteStride;

      openvkl::api::memstate *m = this->device->allocateBytes(numBytes + 16);
      if (m->allocatedBuffer == nullptr) {
        throw std::bad_alloc();
      }
      view         = m;
      void *buffer = m->allocatedBuffer;

      if (isManagedObject(dataType)) {
        // the user provided an array of VKLObjects, but we'll only populate
        // with the host-side ManagedObject * (the host pointer)
        for (size_t i = 0; i < numItems; i++) {
          const char *src = (const char *)source + i * byteStride;
          char *dst       = (char *)buffer + i * naturalByteStride;
          memcpy(dst, src, sizeOf(dataType));
        }
      } else {
        if (byteStride == naturalByteStride) {
          memcpy(buffer, source, numBytes);
        } else {
          for (size_t i = 0; i < numItems; i++) {
            const char *src = (const char *)source + i * byteStride;
            char *dst       = (char *)buffer + i * naturalByteStride;
            memcpy(dst, src, sizeOf(dataType));
          }
        }
      }

      addr       = (char *)buffer;
      byteStride = naturalByteStride;
    } else if (dataCreationFlags == VKL_DATA_SHARED_BUFFER) {
      // view is not needed for shared buffers
      // TODO: add code to verify provided buffer is in USM
      view = nullptr;
      addr = (char *)source;

      if (byteStride % alignOf(dataType) != 0) {
        LogMessageStream(d, VKL_LOG_WARNING)
            << "VKLData: byteStride for shared buffer will require unaligned "
               "accesses";
      }
    } else {
      throw std::runtime_error("VKLData: unknown data creation flags provided");
    }

    managedObjectType = VKL_DATA;
    if (isManagedObject(dataType)) {
      ManagedObject **child = (ManagedObject **)addr;
      for (uint32_t i = 0; i < numItems; i++) {
        if (child[i])
          child[i]->refInc();
      }
    }

    // set ISPC-side proxy
    ispc.addr       = reinterpret_cast<decltype(ispc.addr)>(addr);
    ispc.byteStride = byteStride;
    ispc.numItems   = numItems;
    ispc.dataType   = dataType;
    ispc.compact    = compact();
  }

  Data::Data(Device *d, size_t numItems, VKLDataType dataType)
      : ManagedObject(d),
        numItems(numItems),
        dataType(dataType),
        dataCreationFlags(VKL_DATA_DEFAULT),
        byteStride(sizeOf(dataType))
  {
    if (numItems == 0) {
      throw std::out_of_range("VKLData: numItems must be positive");
    }

    if (isManagedObject(dataType)) {
      throw std::runtime_error(
          "VKLData: constructor not allowed on managed objects");
    }

    const size_t numBytes = numItems * byteStride;

    openvkl::api::memstate *m = this->device->allocateBytes(numBytes + 16);
    if (m->allocatedBuffer == nullptr) {
      throw std::bad_alloc();
    }
    view = m;
    addr = (char *)m->allocatedBuffer;

    managedObjectType = VKL_DATA;

    // set ISPC-side proxy
    ispc.addr       = reinterpret_cast<decltype(ispc.addr)>(addr);
    ispc.byteStride = byteStride;
    ispc.numItems   = numItems;
    ispc.dataType   = dataType;
    ispc.compact    = compact();
  }

  Data::~Data()
  {
    if (isManagedObject(dataType)) {
      Data **child = (Data **)addr;
      for (uint32_t i = 0; i < numItems; i++) {
        if (child[i])
          child[i]->refDec();
      }
    }

    if (!(dataCreationFlags & VKL_DATA_SHARED_BUFFER)) {
      this->device->freeMemState(view);
    }
  }

  std::string Data::toString() const
  {
    return "openvkl::Data";
  }

  size_t Data::size() const
  {
    return numItems;
  }

  bool Data::compact() const
  {
    return byteStride == sizeOf(dataType);
  }

}  // namespace openvkl
