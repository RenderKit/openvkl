// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/openvkl.h>
#include <openvkl/device/openvkl.h>
#include <stdio.h>

#if defined(_MSC_VER)
#include <malloc.h>   // _malloca
#include <windows.h>  // Sleep
#endif

void demoScalarAPI(VKLDevice device, VKLVolume volume)
{
  printf("demo of 1-wide API\n");

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit2(sampler);

  // bounding box
  vkl_box3f bbox = vklGetBoundingBox(volume);
  printf("\tbounding box\n");
  printf("\t\tlower = %f %f %f\n", bbox.lower.x, bbox.lower.y, bbox.lower.z);
  printf("\t\tupper = %f %f %f\n\n", bbox.upper.x, bbox.upper.y, bbox.upper.z);

  // number of attributes
  unsigned int numAttributes = vklGetNumAttributes(volume);
  printf("\tnum attributes = %d\n\n", numAttributes);

  // value range for all attributes
  for (unsigned int i = 0; i < numAttributes; i++) {
    vkl_range1f valueRange = vklGetValueRange(volume, i);
    printf("\tvalue range (attribute %u) = (%f %f)\n",
           i,
           valueRange.lower,
           valueRange.upper);
  }

  // coordinate for sampling / gradients
  vkl_vec3f coord = {1.f, 2.f, 3.f};
  printf("\n\tcoord = %f %f %f\n\n", coord.x, coord.y, coord.z);

  // sample, gradient (first attribute)
  unsigned int attributeIndex = 0;
  float time                  = 0.f;
  float sample   = vklComputeSample(&sampler, &coord, attributeIndex, time);
  vkl_vec3f grad = vklComputeGradient(&sampler, &coord, attributeIndex, time);
  printf("\tsampling and gradient computation (first attribute)\n");
  printf("\t\tsample = %f\n", sample);
  printf("\t\tgrad   = %f %f %f\n\n", grad.x, grad.y, grad.z);

  // sample (multiple attributes)
  unsigned int M                  = 3;
  unsigned int attributeIndices[] = {0, 1, 2};
  float samples[3];
  vklComputeSampleM(&sampler, &coord, samples, M, attributeIndices, time);
  printf("\tsampling (multiple attributes)\n");
  printf("\t\tsamples = %f %f %f\n\n", samples[0], samples[1], samples[2]);

  vklRelease2(sampler);
}

int main()
{
  vklLoadModule("gpu_device");

  VKLDevice device = vklNewDevice("gpu_4");
  vklCommitDevice(device);

  const int dimensions[] = {128, 128, 128};

  const int numVoxels = dimensions[0] * dimensions[1] * dimensions[2];

  const int numAttributes = 3;

  VKLVolume volume = vklNewVolume(device, "structuredRegular");
  vklSetVec3i(
      volume, "dimensions", dimensions[0], dimensions[1], dimensions[2]);
  vklSetVec3f(volume, "gridOrigin", 0, 0, 0);
  vklSetVec3f(volume, "gridSpacing", 1, 1, 1);

  float *voxels = static_cast<float *>(malloc(numVoxels * sizeof(float)));

  if (!voxels) {
    printf("failed to allocate voxel memory!\n");
    return 1;
  }

  // volume attribute 0: x-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)i;

  VKLData data0 =
      vklNewData(device, numVoxels, VKL_FLOAT, voxels, VKL_DATA_DEFAULT, 0);

  // volume attribute 1: y-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)j;

  VKLData data1 =
      vklNewData(device, numVoxels, VKL_FLOAT, voxels, VKL_DATA_DEFAULT, 0);

  // volume attribute 2: z-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)k;

  VKLData data2 =
      vklNewData(device, numVoxels, VKL_FLOAT, voxels, VKL_DATA_DEFAULT, 0);

  VKLData attributes[] = {data0, data1, data2};

  VKLData attributesData = vklNewData(
      device, numAttributes, VKL_DATA, attributes, VKL_DATA_DEFAULT, 0);

  vklRelease(data0);
  vklRelease(data1);
  vklRelease(data2);

  vklSetData(volume, "data", attributesData);
  vklRelease(attributesData);

  vklCommit(volume);

  demoScalarAPI(device, volume);

  vklRelease(volume);

  vklReleaseDevice(device);

  free(voxels);

  printf("complete.\n");

#if defined(_MSC_VER)
  // On Windows, sleep for a few seconds so the terminal window doesn't close
  // immediately.
  Sleep(3000);
#endif

  return 0;
}
