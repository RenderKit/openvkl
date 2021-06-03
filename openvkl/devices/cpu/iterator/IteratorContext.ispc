// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "IteratorContext.ih"

///////////////////////////////////////////////////////////////////////////////
// Interval iterator context //////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void *uniform EXPORT_UNIQUE(IntervalIteratorContext_Constructor,
                                   void *uniform sampler,
                                   const uniform uint32 attributeIndex,
                                   const uniform int numValueRanges,
                                   const box1f *uniform valueRanges)
{
  uniform IntervalIteratorContext *uniform self =
      uniform new uniform IntervalIteratorContext;

  self->sampler        = (const Sampler *uniform)sampler;
  self->attributeIndex = attributeIndex;

  ValueRanges_Constructor(self->valueRanges, numValueRanges, valueRanges);

  return self;
}

export void EXPORT_UNIQUE(IntervalIteratorContext_Destructor,
                          void *uniform _self)
{
  uniform IntervalIteratorContext *uniform self =
      (uniform IntervalIteratorContext * uniform) _self;

  ValueRanges_Destructor(self->valueRanges);

  delete self;
}

///////////////////////////////////////////////////////////////////////////////
// Hit iterator context ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void *uniform EXPORT_UNIQUE(HitIteratorContext_Constructor,
                                   void *uniform sampler,
                                   const uniform uint32 attributeIndex,
                                   const uniform int numValues,
                                   const float *uniform values)
{
  uniform HitIteratorContext *uniform self =
      uniform new uniform HitIteratorContext;

  self->numValues = numValues;
  self->values    = uniform new uniform float[numValues];

  foreach (i = 0 ... numValues) {
    self->values[i] = values[i];
  }

  // superclass parameters
  self->super.sampler        = (const Sampler *uniform)sampler;
  self->super.attributeIndex = attributeIndex;

  ValueRanges_Constructor(self->super.valueRanges, numValues, values);

  return self;
}

export void EXPORT_UNIQUE(HitIteratorContext_Destructor, void *uniform _self)
{
  uniform HitIteratorContext *uniform self =
      (uniform HitIteratorContext * uniform) _self;

  delete[] self->values;

  ValueRanges_Destructor(self->super.valueRanges);

  delete self;
}
