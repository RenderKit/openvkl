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
             size_t _byteStride,
             bool ownSharedBuffer)
      : ManagedObject(d),
        numItems(numItems),
        dataType(dataType),
        dataCreationFlags(dataCreationFlags),
        byteStride(_byteStride),
        ownSharedBuffer(ownSharedBuffer)
  {
    if (numItems == 0) {
      throw std::out_of_range("VKLData: numItems must be positive");
    }

    if (!source) {
      throw std::runtime_error("VKLData: source cannot be NULL");
    }

    if (ownSharedBuffer && dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
      throw std::runtime_error(
          "VKLData: ownSharedBuffer is only compatible with shared buffers");
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

      sharedPtr =
          this->device->allocateSharedMemory(numBytes, alignOf(dataType));
      if (sharedPtr == nullptr) {
        throw std::bad_alloc();
      }
      void *buffer = sharedPtr;

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
      // sharedPtr is not needed for shared buffers
      sharedPtr = nullptr;
      addr     = (char *)source;

      // we must validate that shared buffers for GPU devices were allocated
      // properly. however, bypass these checks for "owned" buffers.
      if (!ownSharedBuffer &&
          device->getDeviceType() == OPENVKL_DEVICE_TYPE_GPU) {
        AllocType allocationType = device->getAllocationType(source);

        switch (allocationType) {
        case OPENVKL_ALLOC_TYPE_SHARED:
          // all good.
          break;

        case OPENVKL_ALLOC_TYPE_HOST:
          throw std::runtime_error(
              "VKLData: host buffer provided, but need USM buffer for GPU "
              "support");
          break;

        case OPENVKL_ALLOC_TYPE_DEVICE:
          LogMessageStream(d, VKL_LOG_DEBUG)
              << "VKLData: shared data buffer provided with device-only memory";
          break;

        case OPENVKL_ALLOC_TYPE_UNKNOWN: {
          static bool warnedOnce = false;
          if (!warnedOnce) {
            LogMessageStream(d, VKL_LOG_WARNING)
                << "VKLData: could not verify allocation type for shared data "
                   "buffer on GPU-based device";
            warnedOnce = true;
          }
          break;
        }
        }
      } else if (ownSharedBuffer) {
        LogMessageStream(d, VKL_LOG_DEBUG)
            << "VKLData: got owned shared buffer -- not performing checks";
      }

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

    sharedPtr = this->device->allocateSharedMemory(numBytes, alignOf(dataType));
    if (sharedPtr == nullptr) {
      throw std::bad_alloc();
    }
    addr = (char *)sharedPtr;

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
      this->device->freeSharedMemory(sharedPtr);
      sharedPtr = nullptr;
    } else if ((dataCreationFlags & VKL_DATA_SHARED_BUFFER) &&
               ownSharedBuffer) {
      LogMessageStream(this->device.ptr, VKL_LOG_DEBUG)
          << "VKLData: deleting ownSharedBuffer";
      delete[] addr;
    }
  }

  rkcommon::memory::Ref<const Data> Data::hostAccessible() const
  {
    if (device->getDeviceType() == OPENVKL_DEVICE_TYPE_GPU &&
        device->getAllocationType(addr) == OPENVKL_ALLOC_TYPE_DEVICE) {
      // create new data object, with host-only copy of data
      // this will be a new refcounted object, which will be destructed and have
      // its memory freed automatically

      LogMessageStream(this->device.ptr, VKL_LOG_DEBUG)
          << "VKLData: copying data to host-accessible Data object";

      char *hostBuffer = this->device->copyDeviceBufferToHost(
          numItems, dataType, addr, byteStride);

      // create new Data object, which will _own_ the hostBuffer above (and
      // delete it on destruction)
      Data *d = new Data(this->device.ptr,
                         numItems,
                         dataType,
                         hostBuffer,
                         VKL_DATA_SHARED_BUFFER,
                         sizeOf(dataType),
                         true);

      rkcommon::memory::Ref<const Data> ref(d);

      // since there is no app handle
      ref->refDec();

      return ref;

    } else {
      return this;
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
