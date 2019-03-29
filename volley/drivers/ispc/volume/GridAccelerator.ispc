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

#include "GridAccelerator.ih"
#include "SharedStructuredVolume.ih"
#include "math/box_utility.ih"

// Bit count used to represent the brick width.
#define BRICK_WIDTH_BITCOUNT (4)

// Brick width in grid cells.
#define BRICK_WIDTH (1 << BRICK_WIDTH_BITCOUNT)

// Brick size in cells.
#define BRICK_CELL_COUNT (BRICK_WIDTH * BRICK_WIDTH * BRICK_WIDTH)

// Bit count used to represent the grid cell width.
#define CELL_WIDTH_BITCOUNT (4)

// Grid cell width in volumetric elements.
#define CELL_WIDTH (1 << CELL_WIDTH_BITCOUNT)

// Compute the 1D address of a cell in the grid.
uint32 GridAccelerator_getCellAddress(GridAccelerator *uniform accelerator,
                                      const varying vec3i &index);

// Get the volumetric value range of a cell.
inline void GridAccelerator_getCellRange(GridAccelerator *uniform accelerator,
                                         const varying vec3i &index,
                                         varying box1f &value)
{
  const uint32 address = GridAccelerator_getCellAddress(accelerator, index);
  value                = accelerator->cellRange[address];
}

// Set the volumetric value range of a cell.
inline void GridAccelerator_setCellRange(GridAccelerator *uniform accelerator,
                                         uniform uint32 address,
                                         uniform box1f &value)
{
  accelerator->cellRange[address] = value;
}

GridAccelerator *uniform GridAccelerator_Constructor(void *uniform _volume)
{
  SharedStructuredVolume *uniform volume =
      (SharedStructuredVolume * uniform) _volume;

  GridAccelerator *uniform accelerator = uniform new uniform GridAccelerator;

  // Grid size in cells per dimension after padding out the volume dimensions to
  // the nearest cell.
  accelerator->gridDimensions =
      (volume->dimensions + CELL_WIDTH - 1) / CELL_WIDTH;

  // Grid size in bricks per dimension after padding out the grid dimensions to
  // the nearest brick.
  accelerator->brickCount =
      (accelerator->gridDimensions + BRICK_WIDTH - 1) / BRICK_WIDTH;

  // Grid cell count with padding.
  const uniform size_t cellCount = accelerator->brickCount.x *
                                   accelerator->brickCount.y *
                                   accelerator->brickCount.z * BRICK_CELL_COUNT;

  accelerator->cellRange =
      (cellCount > 0) ? uniform new uniform box1f[cellCount] : NULL;

  accelerator->volume = volume;

  return accelerator;
}

void GridAccelerator_Destructor(GridAccelerator *uniform accelerator)
{
  if (accelerator->cellRange)
    delete[] accelerator->cellRange;

  delete accelerator;
}

inline void GridAccelerator_encodeBrickCell(GridAccelerator *uniform accel,
                                            SharedStructuredVolume *uniform
                                                volume,
                                            const uniform vec3i &cellIndex,
                                            uniform box1f &cellRange)
{
  uniform bool cellEmpty = true;

  // Loop over voxels in the current cell.
  foreach (k = 0 ... CELL_WIDTH, j = 0 ... CELL_WIDTH, i = 0 ... CELL_WIDTH) {
    // The 3D index of the voxel in the volume.
    const vec3i voxelIndex = cellIndex * CELL_WIDTH + make_vec3i(i, j, k);

    // The voxel value.
    float value;
    volume->getVoxel(volume, min(volume->dimensions - 1, voxelIndex), value);

    // Update the volumetric value range of the current cell, ignoring any NaN
    // values.
    if (!isnan(value)) {
      cellRange.lower = min(cellRange.lower, reduce_min(value));
      cellRange.upper = max(cellRange.upper, reduce_max(value));
      cellEmpty   = false;
    }
  }

  if (cellEmpty) {
    cellRange.lower = cellRange.upper = floatbits(0xffffffff); /* NaN */
  }
}

// Compute the value range of voxels contained in each cell of a brick in the
// grid.
inline void GridAccelerator_encodeVolumeBrick(
    GridAccelerator *uniform accelerator,
    SharedStructuredVolume *uniform volume,
    const uniform int taskIndex)
{
  // Brick index from task index.
  const uniform int bx = taskIndex % accelerator->brickCount.x;
  const uniform int by =
      (taskIndex / accelerator->brickCount.x) % accelerator->brickCount.y;
  const uniform int bz =
      taskIndex / (accelerator->brickCount.x * accelerator->brickCount.y);
  const uniform vec3i brickIndex = make_vec3i(bx, by, bz);

  // The 1D address of the brick in the grid.
  uniform uint32 brickAddress =
      brickIndex.x +
      accelerator->brickCount.x *
          (brickIndex.y + accelerator->brickCount.y * (uint32)brickIndex.z);

  // Loop over cells in the current brick.
  for (uniform uint32 i = 0; i < BRICK_CELL_COUNT; i++) {
    // The 1D address of the cell in the grid.
    uniform uint32 cellAddress = brickAddress << (3 * BRICK_WIDTH_BITCOUNT) | i;

    // The index of the slice containing the current cell.
    uniform uint32 z      = i >> (2 * BRICK_WIDTH_BITCOUNT);
    uniform uint32 offset = i & (BRICK_WIDTH * BRICK_WIDTH - 1);

    // The indices of the cell within the slice.
    uniform uint32 y = offset >> BRICK_WIDTH_BITCOUNT;
    uniform uint32 x = offset % BRICK_WIDTH;

    // The 3D index of the cell in the grid.
    uniform vec3i cellIndex = brickIndex * BRICK_WIDTH + make_vec3i(x, y, z);

    // The minimum and maximum volumetric values contained in the cell.
    uniform box1f cellRange = make_box1f(99999.0f, -99999.0f);

    // Compute the value range over the voxels in the cell.
    GridAccelerator_encodeBrickCell(accelerator, volume, cellIndex, cellRange);

    // Store the value range.
    GridAccelerator_setCellRange(accelerator, cellAddress, cellRange);
  }
}

inline uint32 GridAccelerator_getCellAddress(
    GridAccelerator *uniform accelerator, const varying vec3i &index)
{
  // Compute the 3D index of the grid brick containing the cell.
  const vec3i brickIndex = index >> BRICK_WIDTH_BITCOUNT;

  // Compute the 1D address of the grid brick containing the cell.
  const uint32 brickAddress =
      brickIndex.x +
      accelerator->brickCount.x *
          (brickIndex.y + accelerator->brickCount.y * (uint32)brickIndex.z);

  // Compute the 3D offset of the cell in the grid brick.
  const vec3i cellOffset = bitwise_AND(index, BRICK_WIDTH - 1);

  // Compute the 1D address of the cell.
  return brickAddress << (3 * BRICK_WIDTH_BITCOUNT) |
         cellOffset.z << (2 * BRICK_WIDTH_BITCOUNT) |
         cellOffset.y << (BRICK_WIDTH_BITCOUNT) | cellOffset.x;
}

inline box3f GridAccelerator_getCellBounds(GridAccelerator *uniform accelerator,
                                           const varying vec3i &index)
{
  SharedStructuredVolume *uniform volume =
      (SharedStructuredVolume * uniform) accelerator->volume;

  // Coordinates of the lower corner of the cell in object coordinates.
  vec3f lower;
  volume->transformLocalToObject(
      volume, to_float(index << CELL_WIDTH_BITCOUNT), lower);

  // Coordinates of the upper corner of the cell in object coordinates.
  vec3f upper;
  volume->transformLocalToObject(
      volume, to_float(index + 1 << CELL_WIDTH_BITCOUNT), upper);

  // The bounding box in object coordinates.
  return (make_box3f(lower, upper));
}

bool GridAccelerator_nextCell(GridAccelerator *uniform accelerator,
                              GridAcceleratorRayIterator *uniform rayIterator)
{
  SharedStructuredVolume *uniform volume =
      (SharedStructuredVolume * uniform) accelerator->volume;

  cif(rayIterator->currentCellIndex.x == -1)
  {
    // first iteration
    vec3f localCoordinates;
    volume->transformObjectToLocal(
        volume,
        rayIterator->origin +
            (rayIterator->boundingBoxTRange.lower) * rayIterator->direction,
        localCoordinates);

    rayIterator->currentCellIndex =
        to_int(localCoordinates) >> CELL_WIDTH_BITCOUNT;
  }
  else
  {
    // subsequent iterations: only moving one cell at a time

    // transform object-space direction and origin to cell-space
    const vec3f cellDirection =
        rayIterator->direction * rcp_safe(volume->gridSpacing) * rcp(16.f);

    const vec3f rcpCellDirection = rcp_safe(cellDirection);

    vec3f cellOrigin;
    volume->transformObjectToLocal(volume, rayIterator->origin, cellOrigin);
    cellOrigin = cellOrigin * rcp(16.f);

    // sign of direction determines index delta (1 or -1 in each dimension) to
    // far corner cell
    const vec3i cornerDeltaCellIndex =
        make_vec3i(1 - 2 * (intbits(cellDirection.x) >> 31),
                   1 - 2 * (intbits(cellDirection.y) >> 31),
                   1 - 2 * (intbits(cellDirection.z) >> 31));

    // find exit distance within current cell
    const vec3f t0 = (to_float(rayIterator->currentCellIndex) - cellOrigin) *
                     rcpCellDirection;
    const vec3f t1 =
        (to_float(rayIterator->currentCellIndex + 1) - cellOrigin) *
        rcpCellDirection;
    const vec3f tMax = max(t0, t1);

    const float tExit = reduce_min(tMax);

    // the next cell corresponds to the exit point (which will be a movement in
    // one direction only)
    vec3i deltaCellIndex =
        make_vec3i(tMax.x == tExit ? cornerDeltaCellIndex.x : 0,
                   tMax.y == tExit ? cornerDeltaCellIndex.y : 0,
                   tMax.z == tExit ? cornerDeltaCellIndex.z : 0);

    rayIterator->currentCellIndex =
        rayIterator->currentCellIndex + deltaCellIndex;
  }

  box3f cellBounds =
      GridAccelerator_getCellBounds(accelerator, rayIterator->currentCellIndex);

  // clamp next cell bounds to ray iterator bounding range
  box1f cellInterval = intersectBox(rayIterator->origin,
                                    rayIterator->direction,
                                    cellBounds,
                                    rayIterator->boundingBoxTRange);

  if (isempty1f(cellInterval)) {
    resetRayInterval(rayIterator->currentRayInterval);
    return false;
  } else {
    rayIterator->currentRayInterval.tRange = cellInterval;
    return true;
  }
}

export uniform int GridAccelerator_getBrickCount_x(void *uniform _accel)
{
  GridAccelerator *uniform accelerator = (GridAccelerator * uniform) _accel;
  return accelerator->brickCount.x;
}

export uniform int GridAccelerator_getBrickCount_y(void *uniform _accel)
{
  GridAccelerator *uniform accelerator = (GridAccelerator * uniform) _accel;
  return accelerator->brickCount.y;
}

export uniform int GridAccelerator_getBrickCount_z(void *uniform _accel)
{
  GridAccelerator *uniform accelerator = (GridAccelerator * uniform) _accel;
  return accelerator->brickCount.z;
}

export void GridAccelerator_buildAccelerator(void *uniform _volume,
                                             const uniform int taskIndex)
{
  SharedStructuredVolume *uniform volume =
      (SharedStructuredVolume * uniform) _volume;

  // Compute the volumetric value range per cell.
  GridAccelerator_encodeVolumeBrick(volume->accelerator, volume, taskIndex);
}
