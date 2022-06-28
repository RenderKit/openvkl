// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"

#include "../common/ValueRanges.ih"
#define __ISPC_STRUCT_box1f__
#define __ISPC_STRUCT_ValueRanges__
#include "IteratorContextShared.h"

///////////////////////////////////////////////////////////////////////////////
// Interval iterator context //////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void EXPORT_UNIQUE(IntervalIteratorContext_Constructor,
                          void *uniform sampler,
                          const uniform uint32 attributeIndex,
                          const uniform int numValueRanges,
                          const box1f *uniform valueRanges,
                          const uniform uint32 maxIteratorDepth,
                          const uniform bool elementaryCellIteration,
                          void *uniform _self)
{
  uniform IntervalIteratorContext *uniform self =
      (uniform IntervalIteratorContext * uniform) _self;

  self->super.sampler        = (const SamplerShared *uniform)sampler;
  self->super.attributeIndex = attributeIndex;

  ValueRanges_Constructor(self->super.valueRanges, numValueRanges, valueRanges);

  self->super.maxIteratorDepth        = maxIteratorDepth;
  self->super.elementaryCellIteration = elementaryCellIteration;
}

export void EXPORT_UNIQUE(IntervalIteratorContext_Destructor,
                          void *uniform _self)
{
  uniform IntervalIteratorContext *uniform self =
      (uniform IntervalIteratorContext * uniform) _self;

  ValueRanges_Destructor(self->super.valueRanges);
}

///////////////////////////////////////////////////////////////////////////////
// Hit iterator context ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void EXPORT_UNIQUE(HitIteratorContext_Constructor,
                          void *uniform sampler,
                          const uniform uint32 attributeIndex,
                          const uniform int numValues,
                          const float *uniform values,
                          const uniform uint32 maxIteratorDepth,
                          void *uniform _self)
{
  uniform HitIteratorContext *uniform self =
      (uniform HitIteratorContext * uniform) _self;

  self->numValues = numValues;

  // superclass parameters
  self->super.super.sampler        = (const SamplerShared *uniform)sampler;
  self->super.super.attributeIndex = attributeIndex;

  ValueRanges_Constructor(self->super.super.valueRanges, numValues, values);

  self->super.super.maxIteratorDepth = maxIteratorDepth;

  // always attempt to use elementary cell iteration for hit iteration
  self->super.super.elementaryCellIteration = true;
}

export void EXPORT_UNIQUE(HitIteratorContext_Destructor, void *uniform _self)
{
  uniform HitIteratorContext *uniform self =
      (uniform HitIteratorContext * uniform) _self;

  ValueRanges_Destructor(self->super.super.valueRanges);
}
