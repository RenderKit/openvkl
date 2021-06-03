// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "../math/box_utility.ih"
#include "../common/ValueRanges.ih"
#include "../volume/GridAccelerator.ih"
#include "../volume/SharedStructuredVolume.ih"
#include "../iterator/IteratorContext.ih"
#include "GridAcceleratorIterator.ih"

export void EXPORT_UNIQUE(GridAcceleratorIterator_declare_types_dummy,
                          uniform vec3f &dummy_vec3f)
{
}

inline void printGridAcceleratorIterator(
    varying GridAcceleratorIterator *uniform self)
{
  print("origin:\n  %\n  %\n  %\n",
        self->origin.x,
        self->origin.y,
        self->origin.z);
  print("direction:\n  %\n  %\n  %\n",
        self->direction.x,
        self->direction.y,
        self->direction.z);
  print("tRange\n  %\n  %\n", self->tRange.lower, self->tRange.upper);
  print("boundingBoxTRange:\n  %\n  %\n",
        self->boundingBoxTRange.lower,
        self->boundingBoxTRange.upper);
}

export uniform int EXPORT_UNIQUE(GridAcceleratorIteratorU_sizeOf)
{
  return sizeof(uniform GridAcceleratorIterator);
}

export uniform int EXPORT_UNIQUE(GridAcceleratorIteratorV_sizeOf)
{
  return sizeof(varying GridAcceleratorIterator);
}

#define template_GridAcceleratorIterator_Initialize_internal(univary)          \
  univary GridAcceleratorIterator *uniform self =                              \
      (univary GridAcceleratorIterator * uniform) _self;                       \
                                                                               \
  self->context   = (const IntervalIteratorContext *uniform)_context;          \
  self->origin    = *((univary vec3f * uniform) _origin);                      \
  self->direction = *((univary vec3f * uniform) _direction);                   \
  self->tRange    = *((univary box1f * uniform) _tRange);                      \
  self->time      = *((univary float *uniform)_time);                          \
                                                                               \
  const SharedStructuredVolume *uniform volume =                               \
      (const SharedStructuredVolume *uniform)self->context->sampler->volume;   \
                                                                               \
  self->boundingBoxTRange = intersectBox(                                      \
      self->origin, self->direction, volume->boundingBox, self->tRange);       \
                                                                               \
  /* if using ISPC fast-math and approximate rcp() functions, an epsilon needs \
   to be added to the bounding box intersection to prevent artifacts. this is  \
   because the inverse coordinate transformation may land outside a grid cell. \
   the artifacts may look like speckles, or cross-hairs in orthographic camera \
   views. */                                                                   \
  /*const uniform float epsilon = reduce_min(volume->gridSpacing) *            \
   * 0.125f;                                                                   \
  self->boundingBoxTRange.lower += epsilon;                                    \
  */                                                                           \
                                                                               \
  self->intervalState.currentCellIndex = make_vec3i(-1);                       \
                                                                               \
  self->intervalState.nominalDeltaT =                                          \
      reduce_min(volume->gridSpacing *                                         \
                 rcp_safe(absf(self->direction))); /* in ray space */          \
                                                                               \
  self->hitState.currentCellIndex  = make_vec3i(-1);                           \
  self->hitState.currentCellTRange = make_box1f(inf, -inf);

export void EXPORT_UNIQUE(GridAcceleratorIteratorU_Initialize,
                          void *uniform _self,
                          void *uniform _context,
                          void *uniform _origin,
                          void *uniform _direction,
                          void *uniform _tRange,
                          void *uniform _time)
{
  template_GridAcceleratorIterator_Initialize_internal(uniform);
}

export void EXPORT_UNIQUE(GridAcceleratorIteratorV_Initialize,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _context,
                          void *uniform _origin,
                          void *uniform _direction,
                          void *uniform _tRange,
                          void *uniform _time)
{
  if (!imask[programIndex]) {
    return;
  }

  template_GridAcceleratorIterator_Initialize_internal(varying);
}
#undef template_GridAcceleratorIteratorU_Initialize_internal

#define template_GridAcceleratorIterator_iterateInterval_internal(univary)     \
  univary GridAcceleratorIterator *uniform self =                              \
      (univary GridAcceleratorIterator * uniform) _self;                       \
                                                                               \
  univary Interval *uniform interval = (univary Interval * uniform) _interval; \
                                                                               \
  univary int *uniform result = (univary int *uniform)_result;                 \
                                                                               \
  if (isempty1f(self->boundingBoxTRange)) {                                    \
    *result = false;                                                           \
    return;                                                                    \
  }                                                                            \
                                                                               \
  const SharedStructuredVolume *uniform volume =                               \
      (const SharedStructuredVolume *uniform)self->context->sampler->volume;   \
                                                                               \
  while (GridAccelerator_nextCell(volume->accelerator,                         \
                                  self,                                        \
                                  self->intervalState.currentCellIndex,        \
                                  interval->tRange)) {                         \
    univary box1f cellValueRange;                                              \
    GridAccelerator_getCellValueRange(volume->accelerator,                     \
                                      self->intervalState.currentCellIndex,    \
                                      0, /* first attribute */                 \
                                      cellValueRange);                         \
                                                                               \
    univary bool returnInterval = false;                                       \
                                                                               \
    if (valueRangesOverlap(self->context->valueRanges, cellValueRange)) {      \
      returnInterval = true;                                                   \
    }                                                                          \
                                                                               \
    if (returnInterval) {                                                      \
      interval->valueRange    = cellValueRange;                                \
      interval->nominalDeltaT = self->intervalState.nominalDeltaT;             \
                                                                               \
      *result = true;                                                          \
      return;                                                                  \
    }                                                                          \
  }                                                                            \
                                                                               \
  *result = false;

export void EXPORT_UNIQUE(GridAcceleratorIteratorU_iterateInterval,
                          void *uniform _self,
                          void *uniform _interval,
                          uniform int *uniform _result)
{
  template_GridAcceleratorIterator_iterateInterval_internal(uniform);
}

export void EXPORT_UNIQUE(GridAcceleratorIteratorV_iterateInterval,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _interval,
                          uniform int *uniform _result)
{
  if (!imask[programIndex]) {
    return;
  }

  template_GridAcceleratorIterator_iterateInterval_internal(varying);
}
#undef template_GridAcceleratorIterator_iterateInterval_internal

#define cif_uniform if
#define cif_varying cif

#define template_GridAcceleratorIterator_iterateHit_internal(univary)          \
  univary GridAcceleratorIterator *uniform self =                              \
      (univary GridAcceleratorIterator * uniform) _self;                       \
                                                                               \
  const HitIteratorContext *uniform hitContext =                               \
      (const HitIteratorContext *uniform) self->context;                       \
                                                                               \
  univary Hit *uniform hit = (univary Hit * uniform) _hit;                     \
                                                                               \
  univary int *uniform result = (univary int *uniform)_result;                 \
                                                                               \
  if (isempty1f(self->boundingBoxTRange)) {                                    \
    *result = false;                                                           \
    return;                                                                    \
  }                                                                            \
                                                                               \
  if (hitContext->numValues == 0) {                                            \
    *result = false;                                                           \
    return;                                                                    \
  }                                                                            \
                                                                               \
  const SharedStructuredVolume *uniform volume =                               \
      (const SharedStructuredVolume *uniform)self->context->sampler->volume;   \
                                                                               \
  /* first iteration */                                                        \
  __vkl_concat(cif_, univary)(self->hitState.currentCellIndex.x == -1)         \
  {                                                                            \
    self->hitState.activeCell =                                                \
        GridAccelerator_nextCell(volume->accelerator,                          \
                                 self,                                         \
                                 self->hitState.currentCellIndex,              \
                                 self->hitState.currentCellTRange);            \
  }                                                                            \
                                                                               \
  const uniform float step = reduce_min(volume->gridSpacing);                  \
                                                                               \
  while (self->hitState.activeCell) {                                          \
    univary box1f cellValueRange;                                              \
    GridAccelerator_getCellValueRange(volume->accelerator,                     \
                                      self->hitState.currentCellIndex,         \
                                      0, /* first attribute */                 \
                                      cellValueRange);                         \
                                                                               \
    univary bool cellValueRangeOverlap =                                       \
        valueRangesOverlap(self->context->valueRanges, cellValueRange);        \
                                                                               \
    if (cellValueRangeOverlap) {                                               \
      univary bool foundHit =                                                  \
          intersectSurfacesNewton(self->context->sampler,                      \
                                  self->origin,                                \
                                  self->direction,                             \
                                  self->hitState.currentCellTRange,            \
                                  self->time,                                  \
                                  0.5f * step,                                 \
                                  hitContext->numValues,                       \
                                  hitContext->values,                          \
                                  *hit);                                       \
                                                                               \
      if (foundHit) {                                                          \
        *result                                = true;                         \
        self->hitState.currentCellTRange.lower = hit->t + hit->epsilon;        \
                                                                               \
        /* move to next cell if next t passes the cell boundary */             \
        if (isempty1f(self->hitState.currentCellTRange)) {                     \
          self->hitState.activeCell =                                          \
              GridAccelerator_nextCell(volume->accelerator,                    \
                                       self,                                   \
                                       self->hitState.currentCellIndex,        \
                                       self->hitState.currentCellTRange);      \
                                                                               \
          /* continue where we left off */                                     \
          self->hitState.currentCellTRange.lower = hit->t + hit->epsilon;      \
        }                                                                      \
                                                                               \
        return;                                                                \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* if no hits are found, move to the next cell; if a hit is found we'll    \
     stay in the cell to pursue other hits */                                  \
    self->hitState.activeCell =                                                \
        GridAccelerator_nextCell(volume->accelerator,                          \
                                 self,                                         \
                                 self->hitState.currentCellIndex,              \
                                 self->hitState.currentCellTRange);            \
  }                                                                            \
                                                                               \
  *result = false;

export void EXPORT_UNIQUE(GridAcceleratorIteratorU_iterateHit,
                          void *uniform _self,
                          void *uniform _hit,
                          uniform int *uniform _result)
{
  template_GridAcceleratorIterator_iterateHit_internal(uniform);
}

export void EXPORT_UNIQUE(GridAcceleratorIteratorV_iterateHit,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _hit,
                          uniform int *uniform _result)
{
  if (!imask[programIndex]) {
    return;
  }

  template_GridAcceleratorIterator_iterateHit_internal(varying);
}
#undef template_GridAcceleratorIterator_iterateHit_internal
