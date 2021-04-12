// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "../iterator/GridAcceleratorIterator.ih"
#include "GridAccelerator.ih"
#include "SharedStructuredVolume.ih"
#include "math/box_utility.ih"

// bit count used to represent the brick width in macrocells
#define BRICK_WIDTH_BITCOUNT (4)

// brick width in macrocells
#define BRICK_WIDTH (1 << BRICK_WIDTH_BITCOUNT)

// brick count in macrocells
#define BRICK_CELL_COUNT (BRICK_WIDTH * BRICK_WIDTH * BRICK_WIDTH)

// bit count used to represent the macrocell width in volume cells
#define CELL_WIDTH_BITCOUNT (4)

// macrocell width in volume cells
#define CELL_WIDTH (1 << CELL_WIDTH_BITCOUNT)

// reciprocal of macrocell width in volume cells
#define RCP_CELL_WIDTH 1.f / CELL_WIDTH

#define template_GridAccelerator_getters(univary)                              \
  inline univary uint32 GridAccelerator_getCellAddress(                        \
      GridAccelerator *uniform accelerator, const univary vec3i &cellIndex)    \
  {                                                                            \
    const univary vec3i brickIndex = cellIndex >> BRICK_WIDTH_BITCOUNT;        \
                                                                               \
    const univary uint32 brickAddress =                                        \
        brickIndex.x + accelerator->bricksPerDimension.x *                     \
                           (brickIndex.y + accelerator->bricksPerDimension.y * \
                                               (uint32)brickIndex.z);          \
                                                                               \
    const univary vec3i cellOffset = bitwise_AND(cellIndex, BRICK_WIDTH - 1);  \
                                                                               \
    return brickAddress << (3 * BRICK_WIDTH_BITCOUNT) |                        \
           cellOffset.z << (2 * BRICK_WIDTH_BITCOUNT) |                        \
           cellOffset.y << (BRICK_WIDTH_BITCOUNT) | cellOffset.x;              \
  }                                                                            \
                                                                               \
  inline void GridAccelerator_getCellValueRange(                               \
      GridAccelerator *uniform accelerator,                                    \
      const univary vec3i &cellIndex,                                          \
      univary box1f &valueRange)                                               \
  {                                                                            \
    const univary uint32 address =                                             \
        GridAccelerator_getCellAddress(accelerator, cellIndex);                \
    valueRange = accelerator->cellValueRanges[address];                        \
  }                                                                            \
                                                                               \
  inline univary box3f GridAccelerator_getCellBounds(                          \
      const GridAccelerator *uniform accelerator, const univary vec3i &index)  \
  {                                                                            \
    SharedStructuredVolume *uniform volume = accelerator->volume;              \
                                                                               \
    /* coordinates of the lower corner of the cell in object coordinates */    \
    univary vec3f lower;                                                       \
    transformLocalToObject_##univary##_dispatch(                               \
        volume, to_float(index << CELL_WIDTH_BITCOUNT), lower);                \
                                                                               \
    /* coordinates of the upper corner of the cell in object coordinates */    \
    univary vec3f upper;                                                       \
    transformLocalToObject_##univary##_dispatch(                               \
        volume, to_float(index + 1 << CELL_WIDTH_BITCOUNT), upper);            \
                                                                               \
    return (make_box3f(lower, upper));                                         \
  }

template_GridAccelerator_getters(uniform);
template_GridAccelerator_getters(varying);
#undef template_GridAccelerator_getters

inline void GridAccelerator_setCellValueRange(GridAccelerator *uniform
                                                  accelerator,
                                              uniform uint32 address,
                                              const uniform box1f &valueRange)
{
  accelerator->cellValueRanges[address] = valueRange;
}

inline void GridAccelerator_computeCellValueRange(
    SharedStructuredVolume *uniform volume,
    const uniform vec3i &cellIndex,
    uniform box1f &valueRange)
{
  // temporarily restrict grid accelerator to first attribute
  const uniform uint32 attributeIndex = 0;

  uniform bool cellEmpty = true;

  foreach (k = 0 ... CELL_WIDTH + 1,
           j = 0 ... CELL_WIDTH + 1,
           i = 0 ... CELL_WIDTH + 1) {
    const vec3i localCoordinates = make_vec3i(cellIndex.x * CELL_WIDTH + i,
                                        cellIndex.y * CELL_WIDTH + j,
                                        cellIndex.z * CELL_WIDTH + k);

    const range1f voxelRange = volume->computeVoxelRange[attributeIndex](
        volume, min(volume->dimensions - 1, localCoordinates), attributeIndex);

    if (!isnan(voxelRange.lower)) {
      extend(valueRange, voxelRange.lower);
      cellEmpty        = false;
    }

    if (!isnan(voxelRange.upper)) {
      extend(valueRange, voxelRange.upper);
      cellEmpty        = false;
    }
  }

  if (cellEmpty) {
    valueRange.lower = valueRange.upper = floatbits(0xffffffff);  // NaN
  }
}

inline void GridAccelerator_encodeBrick(GridAccelerator *uniform accelerator,
                                        const uniform int taskIndex)
{
  // brick index from task index
  const uniform int bx = taskIndex % accelerator->bricksPerDimension.x;
  const uniform int by = (taskIndex / accelerator->bricksPerDimension.x) %
                         accelerator->bricksPerDimension.y;
  const uniform int bz = taskIndex / (accelerator->bricksPerDimension.x *
                                      accelerator->bricksPerDimension.y);
  const uniform vec3i brickIndex = make_vec3i(bx, by, bz);

  uniform uint32 brickAddress =
      brickIndex.x + accelerator->bricksPerDimension.x *
                         (brickIndex.y + accelerator->bricksPerDimension.y *
                                             (uint32)brickIndex.z);

  for (uniform uint32 i = 0; i < BRICK_CELL_COUNT; i++) {
    uniform uint32 z      = i >> (2 * BRICK_WIDTH_BITCOUNT);
    uniform uint32 offset = i & (BRICK_WIDTH * BRICK_WIDTH - 1);
    uniform uint32 y      = offset >> BRICK_WIDTH_BITCOUNT;
    uniform uint32 x      = offset % BRICK_WIDTH;

    uniform vec3i cellIndex = brickIndex * BRICK_WIDTH + make_vec3i(x, y, z);

    uniform box1f valueRange = make_box1f(inf, -inf);
    GridAccelerator_computeCellValueRange(
        accelerator->volume, cellIndex, valueRange);

    uniform uint32 cellAddress = brickAddress << (3 * BRICK_WIDTH_BITCOUNT) | i;
    GridAccelerator_setCellValueRange(accelerator, cellAddress, valueRange);
  }
}

GridAccelerator *uniform GridAccelerator_Constructor(void *uniform _volume)
{
  SharedStructuredVolume *uniform volume =
      (SharedStructuredVolume * uniform) _volume;

  GridAccelerator *uniform accelerator = uniform new uniform GridAccelerator;

  // cells per dimension after padding out the volume dimensions to the nearest
  // cell
  uniform vec3i cellsPerDimension =
      (volume->dimensions + CELL_WIDTH - 1) / CELL_WIDTH;

  // bricks per dimension after padding out the cell dimensions to the nearest
  // brick
  accelerator->bricksPerDimension =
      (cellsPerDimension + BRICK_WIDTH - 1) / BRICK_WIDTH;

  accelerator->cellCount = accelerator->bricksPerDimension.x *
                           accelerator->bricksPerDimension.y *
                           accelerator->bricksPerDimension.z * BRICK_CELL_COUNT;

  accelerator->cellValueRanges =
      (accelerator->cellCount > 0)
          ? uniform new uniform box1f[accelerator->cellCount]
          : NULL;

  accelerator->volume = volume;

  return accelerator;
}

void GridAccelerator_Destructor(GridAccelerator *uniform accelerator)
{
  if (accelerator->cellValueRanges)
    delete[] accelerator->cellValueRanges;

  delete accelerator;
}

#define cif_uniform if
#define cif_varying cif

#define template_GridAccelerator_nextCell(univary)                          \
  univary bool GridAccelerator_nextCell(                                    \
      const GridAccelerator *uniform accelerator,                           \
      const univary GridAcceleratorIterator *uniform iterator,              \
      univary vec3i &cellIndex,                                             \
      univary box1f &cellTRange)                                            \
  {                                                                         \
    SharedStructuredVolume *uniform volume = accelerator->volume;           \
                                                                            \
    const univary bool firstCell = cellIndex.x == -1;                       \
    univary box1f cellInterval;                                             \
    __vkl_concat(cif_, univary)(firstCell)                                  \
    {                                                                       \
      /* first iteration */                                                 \
      univary vec3f localCoordinates;                                       \
      transformObjectToLocal_##univary##_dispatch(                          \
          volume,                                                           \
          iterator->origin +                                                \
              (iterator->boundingBoxTRange.lower) * iterator->direction,    \
          localCoordinates);                                                \
                                                                            \
      cellIndex = to_int(localCoordinates) >> CELL_WIDTH_BITCOUNT;          \
      univary box3f cellBounds =                                            \
          GridAccelerator_getCellBounds(accelerator, cellIndex);            \
                                                                            \
      /* clamp next cell bounds to ray iterator bounding range */           \
      cellInterval = intersectBox(iterator->origin,                         \
                                  iterator->direction,                      \
                                  cellBounds,                               \
                                  iterator->boundingBoxTRange);             \
    }                                                                       \
                                                                            \
    if (!firstCell || isempty1f(cellInterval)) {                            \
      /* subsequent iterations: only moving one cell at a time */           \
                                                                            \
      /* TODO: see "A Fast Voxel Traversal Algorithm for Ray Tracing", John \
         Amanatides, to see if this can be further simplified */            \
                                                                            \
      /* transform object-space direction and origin to cell-space */       \
      const univary vec3f cellDirection =                                   \
          iterator->direction * 1.f / volume->gridSpacing * RCP_CELL_WIDTH; \
                                                                            \
      const univary vec3f rcpCellDirection = rcp_safe(cellDirection);       \
                                                                            \
      univary vec3f cellOrigin;                                             \
      transformObjectToLocal_##univary##_dispatch(                          \
          volume, iterator->origin, cellOrigin);                            \
      cellOrigin = cellOrigin * RCP_CELL_WIDTH;                             \
                                                                            \
      /* sign of direction determines index delta (1 or -1 in each          \
         dimension) to far corner cell */                                   \
      const univary vec3i cornerDeltaCellIndex =                            \
          make_vec3i(1 - 2 * (intbits(cellDirection.x) >> 31),              \
                     1 - 2 * (intbits(cellDirection.y) >> 31),              \
                     1 - 2 * (intbits(cellDirection.z) >> 31));             \
                                                                            \
      /* find exit distance within current cell */                          \
      const univary vec3f t0 =                                              \
          (to_float(cellIndex) - cellOrigin) * rcpCellDirection;            \
      const univary vec3f t1 =                                              \
          (to_float(cellIndex + 1) - cellOrigin) * rcpCellDirection;        \
      const univary vec3f tMax = max(t0, t1);                               \
                                                                            \
      const univary float tExit = reduce_min(tMax);                         \
                                                                            \
      /* the next cell corresponds to the exit point (which will be a       \
         movement in one direction only) */                                 \
      univary vec3i deltaCellIndex =                                        \
          make_vec3i(tMax.x == tExit ? cornerDeltaCellIndex.x : 0,          \
                     tMax.y == tExit ? cornerDeltaCellIndex.y : 0,          \
                     tMax.z == tExit ? cornerDeltaCellIndex.z : 0);         \
                                                                            \
      cellIndex = cellIndex + deltaCellIndex;                               \
                                                                            \
      univary box3f cellBounds =                                            \
          GridAccelerator_getCellBounds(accelerator, cellIndex);            \
                                                                            \
      /* clamp next cell bounds to ray iterator bounding range */           \
      cellInterval = intersectBox(iterator->origin,                         \
                                  iterator->direction,                      \
                                  cellBounds,                               \
                                  iterator->boundingBoxTRange);             \
    }                                                                       \
                                                                            \
    if (isempty1f(cellInterval)) {                                          \
      cellTRange = make_box1f(inf, -inf);                                   \
      return false;                                                         \
    } else {                                                                \
      cellTRange = cellInterval;                                            \
      return true;                                                          \
    }                                                                       \
  }

template_GridAccelerator_nextCell(uniform);
template_GridAccelerator_nextCell(varying);
#undef template_GridAccelerator_nextCell

export uniform int EXPORT_UNIQUE(GridAccelerator_getBricksPerDimension_x,
                                 void *uniform _accelerator)
{
  GridAccelerator *uniform accelerator =
      (GridAccelerator * uniform) _accelerator;
  return accelerator->bricksPerDimension.x;
}

export uniform int EXPORT_UNIQUE(GridAccelerator_getBricksPerDimension_y,
                                 void *uniform _accelerator)
{
  GridAccelerator *uniform accelerator =
      (GridAccelerator * uniform) _accelerator;
  return accelerator->bricksPerDimension.y;
}

export uniform int EXPORT_UNIQUE(GridAccelerator_getBricksPerDimension_z,
                                 void *uniform _accelerator)
{
  GridAccelerator *uniform accelerator =
      (GridAccelerator * uniform) _accelerator;
  return accelerator->bricksPerDimension.z;
}

export void EXPORT_UNIQUE(GridAccelerator_build,
                          void *uniform _accelerator,
                          const uniform int taskIndex)
{
  GridAccelerator *uniform accelerator =
      (GridAccelerator * uniform) _accelerator;
  GridAccelerator_encodeBrick(accelerator, taskIndex);
}

export void EXPORT_UNIQUE(GridAccelerator_computeValueRange,
                          void *uniform _accelerator,
                          uniform float &lower,
                          uniform float &upper)
{
  GridAccelerator *uniform accelerator =
      (GridAccelerator * uniform) _accelerator;

  uniform box1f valueRange = make_box1f(pos_inf, neg_inf);

  for (uniform size_t i = 0; i < accelerator->cellCount; i++) {
    valueRange = box_extend(valueRange, accelerator->cellValueRanges[i]);
  }

  lower = valueRange.lower;
  upper = valueRange.upper;
}
