// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "Volume.ih"

export void EXPORT_UNIQUE(Volume_sample_N_export,
                          void *uniform _self,
                          const uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          float *uniform samples)
{
  Volume *uniform self = (Volume * uniform) _self;

  foreach (i = 0 ... N) {
    varying vec3f oc = objectCoordinates[i];
    samples[i]       = self->computeSample_varying(self, oc);
  }
}

export void EXPORT_UNIQUE(Volume_gradient_N_export,
                          void *uniform _self,
                          const uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          vec3f *uniform gradients)
{
  Volume *uniform self = (Volume * uniform) _self;

  foreach (i = 0 ... N) {
    varying vec3f oc = objectCoordinates[i];
    gradients[i]     = self->computeGradient_varying(self, oc);
  }
}
