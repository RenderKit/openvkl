// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "ObserverRegistry.ih"

export void *uniform EXPORT_UNIQUE(ObserverRegistry_create)
{
  ObserverRegistry *uniform self = uniform new uniform ObserverRegistry;
  self->size                     = 0;
  self->capacity                 = 0;
  self->data                     = NULL;
  return self;
}

export void EXPORT_UNIQUE(ObserverRegistry_destroy, void *uniform _self)
{
  assert(_self);
  ObserverRegistry *uniform self = ((ObserverRegistry * uniform) _self);
  if (self->data)
    delete self->data;
  delete self;
}

export void EXPORT_UNIQUE(ObserverRegistry_add,
                          void *uniform _self,
                          void *uniform ptr)
{
  assert(_self);
  ObserverRegistry *uniform self = ((ObserverRegistry * uniform) _self);
  for (uniform size_t i = 0; i < self->size; ++i) {
    // Do not add observer data multiple times.
    if (self->data[i] == ptr)
      return;
  }

  assert(self->size <= self->capacity);
  if (self->size >= self->capacity) {
    void *uniform *uniform oldData   = self->data;
    const uniform size_t newCapacity = self->capacity + 1;
    // ISPC seems to not be able to parse
    //  uniform new void *uniform [...]
    // directly.
    typedef void *uniform VoidPtr;
    self->data     = uniform new uniform VoidPtr[newCapacity];
    self->capacity = newCapacity;
    if (oldData) {
      if (self->size > 0)
        memcpy(self->data, oldData, sizeof(void *uniform) * self->size);
      delete oldData;
    } else
      assert(self->capacity == 1);
  }

  self->data[self->size] = ptr;
  self->size             = self->size + 1;
}

export void EXPORT_UNIQUE(ObserverRegistry_remove,
                          void *uniform _self,
                          void *uniform ptr)
{
  assert(_self);
  ObserverRegistry *uniform self = ((ObserverRegistry * uniform) _self);
  for (uniform size_t i = 0; i < self->size; ++i) {
    if (self->data[i] == ptr) {
      self->data[i] = self->data[self->size];
      self->size    = self->size - 1;
      // Like std::vector, we never decrease capacity.
    }
  }
}
