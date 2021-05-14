// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "ValueSelector.ih"

inline void ValueRanges_computeMinMax(uniform ValueRanges &valueRanges)
{
  valueRanges.rangesMinMax = make_box1f(inf, -inf);

  foreach (i = 0 ... valueRanges.numRanges) {
    valueRanges.rangesMinMax.lower =
        min(valueRanges.rangesMinMax.lower,
            reduce_min(valueRanges.ranges[i].lower));
    valueRanges.rangesMinMax.upper =
        max(valueRanges.rangesMinMax.upper,
            reduce_max(valueRanges.ranges[i].upper));
  }
}

inline void ValueRanges_Constructor(uniform ValueRanges &valueRanges,
                                    uniform int numRanges,
                                    const box1f *uniform ranges)
{
  valueRanges.numRanges = numRanges;
  valueRanges.ranges    = uniform new uniform box1f[numRanges];

  foreach (i = 0 ... numRanges) {
    valueRanges.ranges[i] = ranges[i];
  }

  ValueRanges_computeMinMax(valueRanges);
}

inline void ValueRanges_Constructor(uniform ValueRanges &valueRanges,
                                    uniform int numValues,
                                    const float *uniform values)
{
  valueRanges.numRanges = numValues;
  valueRanges.ranges    = uniform new uniform box1f[numValues];

  foreach (i = 0 ... numValues) {
    valueRanges.ranges[i].lower = values[i];
    valueRanges.ranges[i].upper = values[i];
  }

  ValueRanges_computeMinMax(valueRanges);
}

inline void ValueRanges_Destructor(uniform ValueRanges &valueRanges)
{
  delete[] valueRanges.ranges;
  valueRanges.ranges = NULL;
}

export void *uniform EXPORT_UNIQUE(ValueSelector_Constructor,
                                   void *uniform volume,
                                   const uniform int &numRanges,
                                   const box1f *uniform ranges,
                                   const uniform int &numValues,
                                   const float *uniform values)
{
  uniform ValueSelector *uniform self = uniform new uniform ValueSelector;

  self->volume = volume;

  // ranges
  ValueRanges_Constructor(self->ranges, numRanges, ranges);

  // values
  self->numValues = numValues;
  self->values    = uniform new uniform float[numValues];

  foreach (i = 0 ... numValues) {
    self->values[i] = values[i];
  }

  ValueRanges_Constructor(self->valuesRanges, numValues, values);

  return self;
}

export void *uniform EXPORT_UNIQUE(ValueSelector_Destructor,
                                   void *uniform _self)
{
  uniform ValueSelector *uniform self = (uniform ValueSelector * uniform) _self;

  ValueRanges_Destructor(self->ranges);

  delete[] self->values;
  ValueRanges_Destructor(self->valuesRanges);

  delete self;
}
