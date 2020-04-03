// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Data.h"
#include "ospcommon/memory/malloc.h"

namespace openvkl {

  Data::Data(size_t numItems,
             VKLDataType dataType,
             const void *source,
             VKLDataCreationFlags dataCreationFlags)
      : numItems(numItems),
        numBytes(numItems * sizeOf(dataType)),
        dataType(dataType),
        dataCreationFlags(dataCreationFlags)
  {
    if (dataCreationFlags & VKL_DATA_SHARED_BUFFER) {
      if (source == nullptr)
        throw std::runtime_error("shared buffer is NULL");
      data = source;
    } else {
      void *buffer = ospcommon::memory::alignedMalloc(numBytes + 16);
      if (buffer == nullptr)
        throw std::runtime_error("data is NULL");
      data = buffer;
      if (source)
        memcpy(buffer, source, numBytes);
      else if (dataType == VKL_OBJECT)
        memset(buffer, 0, numBytes);
    }

    managedObjectType = VKL_DATA;
    if (isManagedObject(dataType)) {
      ManagedObject **child = (ManagedObject **)data;
      for (uint32_t i = 0; i < numItems; i++) {
        if (child[i])
          child[i]->refInc();
      }
    }
  }

  Data::~Data()
  {
    if (isManagedObject(dataType)) {
      Data **child = (Data **)data;
      for (uint32_t i = 0; i < numItems; i++) {
        if (child[i])
          child[i]->refDec();
      }
    }

    if (!(dataCreationFlags & VKL_DATA_SHARED_BUFFER)) {
      // We know we allocated this buffer, so the const cast is in fact
      // reasonable.
      ospcommon::memory::alignedFree(const_cast<void *>(data));
    }
  }

  std::string Data::toString() const
  {
    return "openvkl::Data";
  }

  size_t openvkl::Data::size() const
  {
    return numItems;
  }

}  // namespace openvkl
