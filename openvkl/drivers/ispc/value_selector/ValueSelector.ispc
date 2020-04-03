// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "ValueSelector.ih"

export void *uniform EXPORT_UNIQUE(ValueSelector_Constructor,
                                   void *uniform volume,
                                   const uniform int &numRanges,
                                   const box1f *uniform ranges,
                                   const uniform int &numValues,
                                   const float *uniform values)
{
  uniform ValueSelector *uniform self = uniform new uniform ValueSelector;

  self->volume = volume;

  self->numRanges = numRanges;
  self->ranges    = uniform new uniform box1f[numRanges];

  foreach (i = 0 ... numRanges) {
    self->ranges[i] = ranges[i];
  }

  self->rangesMinMax = make_box1f(inf, -inf);

  foreach (i = 0... numRanges) {
    self->rangesMinMax.lower =
        min(self->rangesMinMax.lower, reduce_min(ranges[i].lower));
    self->rangesMinMax.upper =
        max(self->rangesMinMax.upper, reduce_max(ranges[i].upper));
  }

  self->numValues = numValues;
  self->values    = uniform new uniform float[numValues];

  foreach (i = 0 ... numValues) {
    self->values[i] = values[i];
  }

  self->valuesMinMax = make_box1f(inf, -inf);

  foreach (i = 0... numValues) {
    self->valuesMinMax.lower =
        min(self->valuesMinMax.lower, reduce_min(values[i]));
    self->valuesMinMax.upper =
        max(self->valuesMinMax.upper, reduce_max(values[i]));
  }

  return self;
}

export void *uniform EXPORT_UNIQUE(ValueSelector_Destructor,
                                   void *uniform _self)
{
  uniform ValueSelector *uniform self = (uniform ValueSelector * uniform) _self;
  delete[] self->ranges;
  delete[] self->values;
  delete self;
}
