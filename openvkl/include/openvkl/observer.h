// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <cstdlib>
#else
#include <stdint.h>
#include <stdlib.h>
#endif

#include "common.h"
#include "sampler.h"
#include "volume.h"

// Observers provide an interface for receiving data back from OpenVKL.

#ifdef __cplusplus
struct VKLObserver : public VKLObject
{
};
#else
typedef VKLObject VKLObserver;
#endif

#ifdef __cplusplus
extern "C" {
#endif

NOWARN_C_LINKAGE_PUSH

// Create a new observer of the given type.
// Triggers the error handler and returns NULL on error.
OPENVKL_INTERFACE
VKLObserver vklNewVolumeObserver(VKLVolume volume, const char *type);

OPENVKL_INTERFACE
VKLObserver vklNewSamplerObserver(VKLSampler volume, const char *type);

NOWARN_C_LINKAGE_POP

// Map an observer.
// Returns a pointer to the observer data buffer.
// Triggers the error handler and returns NULL on failure.
// Must not be called on an observer that is mapped already.
// The returned pointer can be cast to a pointer of type
// const T *, where T is the underlying data type.
OPENVKL_INTERFACE
const void * vklMapObserver(VKLObserver observer);

// Unmap an observer.
// Triggers the error handler if observer is NULL.
// Triggers the error handler on invalid parameters.
OPENVKL_INTERFACE
void vklUnmapObserver(VKLObserver observer);

// Query the element data type of the observer data buffer.
// Triggers the error handler and returns VKL_UNKNOWN on error.
// May fail if the observer is not mapped.
OPENVKL_INTERFACE
VKLDataType vklGetObserverElementType(VKLObserver observer);

// Query the size, in bytes, of a single element.
// Triggers the error handler and returns 0 on error.
// May fail if the observer is not mapped.
OPENVKL_INTERFACE
size_t vklGetObserverElementSize(VKLObserver observer);

// Query the number of elements contained in the observer data buffer.
// Triggers the error handler and returns 0 on error.
// May fail if the observer is not mapped.
OPENVKL_INTERFACE
size_t vklGetObserverNumElements(VKLObserver observer);


#ifdef __cplusplus
}  // extern "C"
#endif

