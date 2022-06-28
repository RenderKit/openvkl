// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Data.h"
#include "BufferShared.h"
#include "rkcommon/memory/malloc.h"

namespace openvkl {

  ispcrt::Context *g_ispcrtContext = nullptr;

  ispc::Data1D Data::emptyData1D;

  Data::Data(size_t numItems,
             VKLDataType dataType,
             const void *source,
             VKLDataCreationFlags dataCreationFlags,
             size_t _byteStride)
      : numItems(numItems),
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
      byteStride = sizeOf(dataType);
    }

    if (dataCreationFlags == VKL_DATA_DEFAULT) {
      // copy source data into naturally-strided (compact) array
      const size_t naturalByteStride = sizeOf(dataType);
      const size_t numBytes          = numItems * naturalByteStride;

      view = BufferSharedCreate(numBytes + 16);
      void *buffer = ispcrtSharedPtr(view);

      if (buffer == nullptr) {
        throw std::bad_alloc();
      }

      if (byteStride == naturalByteStride) {
        memcpy(buffer, source, numBytes);
      } else {
        for (size_t i = 0; i < numItems; i++) {
          const char *src = (const char *)source + i * byteStride;
          char *dst       = (char *)buffer + i * naturalByteStride;
          memcpy(dst, src, sizeOf(dataType));
        }
      }

      addr = (char *)buffer;
      byteStride = naturalByteStride;
    } else if (dataCreationFlags == VKL_DATA_SHARED_BUFFER) {
      view = nullptr; //TODO add code to inspect and reuse and copy as required here
      addr = (char *)source;
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

  Data::Data(size_t numItems, VKLDataType dataType)
      : numItems(numItems),
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

    view = BufferSharedCreate(numBytes + 16);
    void *buffer = (char *)ispcrtSharedPtr(view);

    if (buffer == nullptr) {
      throw std::bad_alloc();
    }

    addr = (char *)ispcrtSharedPtr(view);

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
      BufferSharedDelete(view);
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
