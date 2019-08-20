// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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

inline uint32 GridAccelerator_getCellAddress(
    GridAccelerator *uniform accelerator, const varying vec3i &cellIndex)
{
  const vec3i brickIndex = cellIndex >> BRICK_WIDTH_BITCOUNT;

  const uint32 brickAddress =
      brickIndex.x + accelerator->bricksPerDimension.x *
                         (brickIndex.y + accelerator->bricksPerDimension.y *
                                             (uint32)brickIndex.z);

  const vec3i cellOffset = bitwise_AND(cellIndex, BRICK_WIDTH - 1);

  return brickAddress << (3 * BRICK_WIDTH_BITCOUNT) |
         cellOffset.z << (2 * BRICK_WIDTH_BITCOUNT) |
         cellOffset.y << (BRICK_WIDTH_BITCOUNT) | cellOffset.x;
}

inline void GridAccelerator_getCellValueRange(GridAccelerator *uniform
                                                  accelerator,
                                              const varying vec3i &cellIndex,
                                              varying box1f &valueRange)
{
  const uint32 address = GridAccelerator_getCellAddress(accelerator, cellIndex);
  valueRange           = accelerator->cellValueRanges[address];
}

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
  uniform bool cellEmpty = true;

  foreach (k = 0 ... CELL_WIDTH + 1,
           j = 0 ... CELL_WIDTH + 1,
           i = 0 ... CELL_WIDTH + 1) {
    const vec3i voxelIndex = cellIndex * CELL_WIDTH + make_vec3i(i, j, k);

    float value;
    volume->getVoxel(volume, min(volume->dimensions - 1, voxelIndex), value);

    if (!isnan(value)) {
      valueRange.lower = min(valueRange.lower, reduce_min(value));
      valueRange.upper = max(valueRange.upper, reduce_max(value));
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

inline box3f GridAccelerator_getCellBounds(
    const GridAccelerator *uniform accelerator, const varying vec3i &index)
{
  SharedStructuredVolume *uniform volume = accelerator->volume;

  // coordinates of the lower corner of the cell in object coordinates
  vec3f lower;
  volume->transformLocalToObject(
      volume, to_float(index << CELL_WIDTH_BITCOUNT), lower);

  // coordinates of the upper corner of the cell in object coordinates
  vec3f upper;
  volume->transformLocalToObject(
      volume, to_float(index + 1 << CELL_WIDTH_BITCOUNT), upper);

  return (make_box3f(lower, upper));
}

GridAccelerator *uniform GridAccelerator_Constructor(void *uniform _volume)
{
  SharedStructuredVolume *uniform volume =
      (SharedStructuredVolume * uniform) _volume;

  GridAccelerator *uniform accelerator = uniform new uniform GridAccelerator;

  // cells per dimension after padding out the volume dimensions to the nearest
  // cell
  accelerator->cellsPerDimension =
      (volume->dimensions + CELL_WIDTH - 1) / CELL_WIDTH;

  // bricks per dimension after padding out the cell dimensions to the nearest
  // brick
  accelerator->bricksPerDimension =
      (accelerator->cellsPerDimension + BRICK_WIDTH - 1) / BRICK_WIDTH;

  const uniform size_t cellCount =
      accelerator->bricksPerDimension.x * accelerator->bricksPerDimension.y *
      accelerator->bricksPerDimension.z * BRICK_CELL_COUNT;

  accelerator->cellValueRanges =
      (cellCount > 0) ? uniform new uniform box1f[cellCount] : NULL;

  accelerator->volume = volume;

  return accelerator;
}

void GridAccelerator_Destructor(GridAccelerator *uniform accelerator)
{
  if (accelerator->cellValueRanges)
    delete[] accelerator->cellValueRanges;

  delete accelerator;
}

bool GridAccelerator_nextCell(const GridAccelerator *uniform accelerator,
                              const GridAcceleratorIterator *uniform iterator,
                              varying vec3i &cellIndex,
                              varying Interval &interval)
{
  SharedStructuredVolume *uniform volume = accelerator->volume;

  cif(cellIndex.x == -1)
  {
    // first iteration
    vec3f localCoordinates;
    volume->transformObjectToLocal(
        volume,
        iterator->origin +
            (iterator->boundingBoxTRange.lower) * iterator->direction,
        localCoordinates);

    cellIndex = to_int(localCoordinates) >> CELL_WIDTH_BITCOUNT;
  }

  else
  {
    // subsequent iterations: only moving one cell at a time

    // TODO: see "A Fast Voxel Traversal Algorithm for Ray Tracing", John
    // Amanatides, to see if this can be further simplified

    // transform object-space direction and origin to cell-space
    const vec3f cellDirection =
        iterator->direction * 1.f / volume->gridSpacing * RCP_CELL_WIDTH;

    const vec3f rcpCellDirection = 1.f / cellDirection;

    vec3f cellOrigin;
    volume->transformObjectToLocal(volume, iterator->origin, cellOrigin);
    cellOrigin = cellOrigin * RCP_CELL_WIDTH;

    // sign of direction determines index delta (1 or -1 in each dimension) to
    // far corner cell
    const vec3i cornerDeltaCellIndex =
        make_vec3i(1 - 2 * (intbits(cellDirection.x) >> 31),
                   1 - 2 * (intbits(cellDirection.y) >> 31),
                   1 - 2 * (intbits(cellDirection.z) >> 31));

    // find exit distance within current cell
    const vec3f t0 = (to_float(cellIndex) - cellOrigin) * rcpCellDirection;
    const vec3f t1 = (to_float(cellIndex + 1) - cellOrigin) * rcpCellDirection;
    const vec3f tMax = max(t0, t1);

    const float tExit = reduce_min(tMax);

    // the next cell corresponds to the exit point (which will be a movement in
    // one direction only)
    vec3i deltaCellIndex =
        make_vec3i(tMax.x == tExit ? cornerDeltaCellIndex.x : 0,
                   tMax.y == tExit ? cornerDeltaCellIndex.y : 0,
                   tMax.z == tExit ? cornerDeltaCellIndex.z : 0);

    cellIndex = cellIndex + deltaCellIndex;
  }

  box3f cellBounds = GridAccelerator_getCellBounds(accelerator, cellIndex);

  // clamp next cell bounds to ray iterator bounding range
  box1f cellInterval = intersectBox(iterator->origin,
                                    iterator->direction,
                                    cellBounds,
                                    iterator->boundingBoxTRange);

  if (isempty1f(cellInterval)) {
    resetInterval(interval);
    return false;
  } else {
    interval.tRange = cellInterval;
    return true;
  }
}

export uniform int GridAccelerator_getBricksPerDimension_x(
    void *uniform _accelerator)
{
  GridAccelerator *uniform accelerator =
      (GridAccelerator * uniform) _accelerator;
  return accelerator->bricksPerDimension.x;
}

export uniform int GridAccelerator_getBricksPerDimension_y(
    void *uniform _accelerator)
{
  GridAccelerator *uniform accelerator =
      (GridAccelerator * uniform) _accelerator;
  return accelerator->bricksPerDimension.y;
}

export uniform int GridAccelerator_getBricksPerDimension_z(
    void *uniform _accelerator)
{
  GridAccelerator *uniform accelerator =
      (GridAccelerator * uniform) _accelerator;
  return accelerator->bricksPerDimension.z;
}

export void GridAccelerator_build(void *uniform _accelerator,
                                  const uniform int taskIndex)
{
  GridAccelerator *uniform accelerator =
      (GridAccelerator * uniform) _accelerator;
  GridAccelerator_encodeBrick(accelerator, taskIndex);
}
