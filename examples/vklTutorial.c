// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#include <openvkl/openvkl.h>
#include <stdio.h>

void demoScalarAPI(VKLVolume volume)
{
  printf("demo of 1-wide API\n");

  // sample, gradient
  vkl_vec3f coord = {1.f, 1.f, 1.f};
  float sample    = vklComputeSample(volume, &coord);
  vkl_vec3f grad  = vklComputeGradient(volume, &coord);
  printf("\tcoord = %f %f %f\n", coord.x, coord.y, coord.z);
  printf("\t\tsample = %f\n", sample);
  printf("\t\tgrad   = %f %f %f\n\n", grad.x, grad.y, grad.z);

  // iterator setup (note the commit at the end)
  vkl_range1f ranges[2]     = {{10, 20}, {50, 75}};
  int num_ranges            = 2;
  float isosurfaces[2]      = {32, 96};
  int num_isosurfaces       = 2;
  VKLValueSelector selector = vklNewValueSelector(volume);
  vklValueSelectorSetRanges(selector, num_ranges, ranges);
  vklValueSelectorSetValues(selector, num_isosurfaces, isosurfaces);
  vklCommit(selector);

  vkl_vec3f rayOrigin    = {0, 0, 0};
  vkl_vec3f rayDirection = {1, 0, 0};
  vkl_range1f rayTRange  = {0, 200};
  printf("\trayOrigin = %f %f %f\n", rayOrigin.x, rayOrigin.y, rayOrigin.z);
  printf("\trayDirection = %f %f %f\n",
         rayDirection.x,
         rayDirection.y,
         rayDirection.z);
  printf("\trayTRange = %f %f\n", rayTRange.lower, rayTRange.upper);

  struct VKLIntervalIterator intervalIterator;
  vklInitIntervalIterator(&intervalIterator,
                          volume,
                          &rayOrigin,
                          &rayDirection,
                          &rayTRange,
                          selector);

  printf("\n\tranges {%f %f} {%f %f}\n",
         ranges[0].lower,
         ranges[0].upper,
         ranges[1].lower,
         ranges[1].upper);

  for (;;) {
    struct VKLInterval interval;
    int result = vklIterateInterval(&intervalIterator, &interval);
    if (!result)
      break;
    printf(
        "\t\ttRange (%f %f)\n\t\tvalueRange (%f %f)\n\t\tnominalDeltaT %f\n\n",
        interval.tRange.lower,
        interval.tRange.upper,
        interval.valueRange.lower,
        interval.valueRange.upper,
        interval.nominalDeltaT);
  }

  struct VKLHitIterator hitIterator;
  vklInitHitIterator(
      &hitIterator, volume, &rayOrigin, &rayDirection, &rayTRange, selector);

  printf("\n\tisosurfaces %f %f\n", isosurfaces[0], isosurfaces[1]);

  for (;;) {
    struct VKLHit hit;
    int result = vklIterateHit(&hitIterator, &hit);
    if (!result)
      break;
    printf("\t\tt %f\n\t\tsample %f\n\n", hit.t, hit.sample);
  }
}

void demoVectorAPI(VKLVolume volume)
{
  printf("demo of 4-wide API (8- and 16- follow the same pattern)\n");
  vkl_vvec3f4 coord4;  // structure-of-array layout
  int valid[4];
  for (int i = 0; i < 4; i++) {
    coord4.x[i] = coord4.y[i] = coord4.z[i] = i;
    valid[i] = -1;  // valid mask: 0 = not valid, -1 = valid
  }
  float sample4[4];
  vkl_vvec3f4 grad4;
  vklComputeSample4(valid, volume, &coord4, sample4);
  vklComputeGradient4(valid, volume, &coord4, &grad4);
  for (int i = 0; i < 4; i++) {
    printf(
        "\tcoord[%d] = %f %f %f\n", i, coord4.x[i], coord4.y[i], coord4.z[i]);
    printf("\t\tsample[%d] = %f\n", i, sample4[i]);
    printf("\t\tgrad[%d]   = %f %f %f\n", i, grad4.x[i], grad4.y[i], grad4.z[i]);
  }
}

int main()
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  int dimensions[] = {128, 128, 128};

  const int numVoxels = dimensions[0] * dimensions[1] * dimensions[2];

  VKLVolume volume = vklNewVolume("structured_regular");
  vklSetVec3i(
      volume, "dimensions", dimensions[0], dimensions[1], dimensions[2]);
  vklSetVec3f(volume, "gridOrigin", 0, 0, 0);
  vklSetVec3f(volume, "gridSpacing", 1, 1, 1);

  float *voxels = malloc(numVoxels * sizeof(float));

  if (!voxels) {
    printf("failed to allocate voxel memory!\n");
    return 1;
  }

  // x-grad sample volume
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)i;

  VKLData voxelData = vklNewData(numVoxels, VKL_FLOAT, voxels, 0);
  vklSetData(volume, "voxelData", voxelData);
  vklRelease(voxelData);

  vklCommit(volume);

  demoScalarAPI(volume);
  demoVectorAPI(volume);

  vklShutdown();

  free(voxels);

  return 0;
}
