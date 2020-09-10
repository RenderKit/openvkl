// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/openvkl.h>
#include <stdio.h>

#if defined(_MSC_VER)
#include <malloc.h> // _malloca
#endif

void demoScalarAPI(VKLVolume volume)
{
  printf("demo of 1-wide API\n");

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  // bounding box
  vkl_box3f bbox = vklGetBoundingBox(volume);
  printf("\tbounding box\n");
  printf("\t\tlower = %f %f %f\n", bbox.lower.x, bbox.lower.y, bbox.lower.z);
  printf("\t\tupper = %f %f %f\n\n", bbox.upper.x, bbox.upper.y, bbox.upper.z);

  // number of attributes
  unsigned int numAttributes = vklGetNumAttributes(volume);
  printf("\tnum attributes = %d\n\n", numAttributes);

  // value range (first attribute)
  vkl_range1f valueRange = vklGetValueRange(volume);
  printf("\tvalue range (first attribute) = (%f %f)\n\n",
         valueRange.lower,
         valueRange.upper);

  // coordinate for sampling / gradients
  vkl_vec3f coord = {1.f, 2.f, 3.f};
  printf("\tcoord = %f %f %f\n\n", coord.x, coord.y, coord.z);

  // sample, gradient (first attribute)
  unsigned int attributeIndex = 0;
  float sample   = vklComputeSample(sampler, &coord, attributeIndex);
  vkl_vec3f grad = vklComputeGradient(sampler, &coord, attributeIndex);
  printf("\tsampling and gradient computation (first attribute)\n");
  printf("\t\tsample = %f\n", sample);
  printf("\t\tgrad   = %f %f %f\n\n", grad.x, grad.y, grad.z);

  // sample (multiple attributes)
  unsigned int M                  = 3;
  unsigned int attributeIndices[] = {0, 1, 2};
  float samples[3];
  vklComputeSampleM(sampler, &coord, samples, M, attributeIndices);
  printf("\tsampling (multiple attributes)\n");
  printf("\t\tsamples = %f %f %f\n\n", samples[0], samples[1], samples[2]);

  // value selector setup (note the commit at the end)
  vkl_range1f ranges[2]     = {{10, 20}, {50, 75}};
  int num_ranges            = 2;
  float values[2]           = {32, 96};
  int num_values            = 2;
  VKLValueSelector selector = vklNewValueSelector(volume);
  vklValueSelectorSetRanges(selector, num_ranges, ranges);
  vklValueSelectorSetValues(selector, num_values, values);
  vklCommit(selector);

  // ray definition for iterators
  vkl_vec3f rayOrigin    = {0, 1, 1};
  vkl_vec3f rayDirection = {1, 0, 0};
  vkl_range1f rayTRange  = {0, 200};
  printf("\trayOrigin = %f %f %f\n", rayOrigin.x, rayOrigin.y, rayOrigin.z);
  printf("\trayDirection = %f %f %f\n",
         rayDirection.x,
         rayDirection.y,
         rayDirection.z);
  printf("\trayTRange = %f %f\n", rayTRange.lower, rayTRange.upper);

  // interval iteration. This is scoped
  {
    // Note: buffer will cease to exist at the end of this scope.
#if defined(_MSC_VER)
    // MSVC does not support variable length arrays, but provides a
    // safer version of alloca.
    char *buffer = _malloca(vklGetIntervalIteratorSize(volume));
#else
    char buffer[vklGetIntervalIteratorSize(volume)];
#endif
    VKLIntervalIterator intervalIterator = vklInitIntervalIterator(
        volume, &rayOrigin, &rayDirection, &rayTRange, selector, buffer);

    printf("\n\tinterval iterator for value ranges {%f %f} {%f %f}\n",
           ranges[0].lower,
           ranges[0].upper,
           ranges[1].lower,
           ranges[1].upper);

    for (;;) {
      VKLInterval interval;
      int result = vklIterateInterval(intervalIterator, &interval);
      if (!result)
        break;
      printf(
          "\t\ttRange (%f %f)\n\t\tvalueRange (%f %f)\n\t\tnominalDeltaT "
          "%f\n\n",
          interval.tRange.lower,
          interval.tRange.upper,
          interval.valueRange.lower,
          interval.valueRange.upper,
          interval.nominalDeltaT);
    }
  }

  // hit iteration
  {
#if defined(_MSC_VER)
    // MSVC does not support variable length arrays, but provides a
    // safer version of alloca.
    char *buffer = _malloca(vklGetHitIteratorSize(volume));
#else
    char buffer[vklGetHitIteratorSize(volume)];
#endif
    VKLHitIterator hitIterator = vklInitHitIterator(
        volume, &rayOrigin, &rayDirection, &rayTRange, selector, buffer);

    printf("\thit iterator for values %f %f\n", values[0], values[1]);

    for (;;) {
      VKLHit hit;
      int result = vklIterateHit(hitIterator, &hit);
      if (!result)
        break;
      printf("\t\tt %f\n\t\tsample %f\n\t\tepsilon %f\n\n",
             hit.t,
             hit.sample,
             hit.epsilon);
    }
  }

  vklRelease(selector);
  vklRelease(sampler);
}

void demoVectorAPI(VKLVolume volume)
{
  printf("demo of 4-wide API (8- and 16- follow the same pattern)\n");

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  // structure-of-array layout
  vkl_vvec3f4 coord4;
  int valid[4];
  for (int i = 0; i < 4; i++) {
    coord4.x[i] = i * 3 + 0;
    coord4.y[i] = i * 3 + 1;
    coord4.z[i] = i * 3 + 2;
    valid[i]    = -1;  // valid mask: 0 = not valid, -1 = valid
  }

  for (int i = 0; i < 4; i++) {
    printf(
        "\tcoord[%d] = %f %f %f\n", i, coord4.x[i], coord4.y[i], coord4.z[i]);
  }

  // sample, gradient (first attribute)
  unsigned int attributeIndex = 0;
  float sample4[4];
  vkl_vvec3f4 grad4;
  vklComputeSample4(valid, sampler, &coord4, sample4, attributeIndex);
  vklComputeGradient4(valid, sampler, &coord4, &grad4, attributeIndex);

  printf("\n\tsampling and gradient computation (first attribute)\n");

  for (int i = 0; i < 4; i++) {
    printf("\t\tsample[%d] = %f\n", i, sample4[i]);
    printf(
        "\t\tgrad[%d]   = %f %f %f\n", i, grad4.x[i], grad4.y[i], grad4.z[i]);
  }

  // sample (multiple attributes)
  unsigned int M                  = 3;
  unsigned int attributeIndices[] = {0, 1, 2};
  float samples[3 * 4];
  vklComputeSampleM4(valid, sampler, &coord4, samples, M, attributeIndices);

  printf("\n\tsampling (multiple attributes)\n");

  printf("\t\tsamples = ");

  for (unsigned int j = 0; j < M; j++) {
    printf("%f %f %f %f\n",
           samples[j * 4 + 0],
           samples[j * 4 + 1],
           samples[j * 4 + 2],
           samples[j * 4 + 3]);
    printf("\t\t          ");
  }

  printf("\n");

  vklRelease(sampler);
}

void demoStreamAPI(VKLVolume volume)
{
  printf("demo of stream API\n");

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  // array-of-structure layout; arbitrary stream lengths are supported
  vkl_vec3f coord[5];

  for (int i = 0; i < 5; i++) {
    coord[i].x = i * 3 + 0;
    coord[i].y = i * 3 + 1;
    coord[i].z = i * 3 + 2;
  }

  for (int i = 0; i < 5; i++) {
    printf("\tcoord[%d] = %f %f %f\n", i, coord[i].x, coord[i].y, coord[i].z);
  }

  // sample, gradient (first attribute)
  printf("\n\tsampling and gradient computation (first attribute)\n");
  unsigned int attributeIndex = 0;
  float sample[5];
  vkl_vec3f grad[5];
  vklComputeSampleN(sampler, 5, coord, sample, attributeIndex);
  vklComputeGradientN(sampler, 5, coord, grad, attributeIndex);

  for (int i = 0; i < 5; i++) {
    printf("\t\tsample[%d] = %f\n", i, sample[i]);
    printf("\t\tgrad[%d]   = %f %f %f\n", i, grad[i].x, grad[i].y, grad[i].z);
  }

  // sample (multiple attributes)
  unsigned int M                  = 3;
  unsigned int attributeIndices[] = {0, 1, 2};
  float samples[3 * 5];
  vklComputeSampleMN(sampler, 5, coord, samples, M, attributeIndices);

  printf("\n\tsampling (multiple attributes)\n");

  printf("\t\tsamples = ");

  for (int i = 0; i < 5; i++) {
    for (unsigned int j = 0; j < M; j++) {
      printf("%f ", samples[i * M + j]);
    }
    printf("\n\t\t          ");
  }

  printf("\n");

  vklRelease(sampler);
}

int main()
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  const int dimensions[] = {128, 128, 128};

  const int numVoxels = dimensions[0] * dimensions[1] * dimensions[2];

  const int numAttributes = 3;

  VKLVolume volume = vklNewVolume("structuredRegular");
  vklSetVec3i(
      volume, "dimensions", dimensions[0], dimensions[1], dimensions[2]);
  vklSetVec3f(volume, "gridOrigin", 0, 0, 0);
  vklSetVec3f(volume, "gridSpacing", 1, 1, 1);

  float *voxels = malloc(numVoxels * sizeof(float));

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

  VKLData data0 = vklNewData(numVoxels, VKL_FLOAT, voxels, VKL_DATA_DEFAULT, 0);

  // volume attribute 1: y-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)j;

  VKLData data1 = vklNewData(numVoxels, VKL_FLOAT, voxels, VKL_DATA_DEFAULT, 0);

  // volume attribute 2: z-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)k;

  VKLData data2 = vklNewData(numVoxels, VKL_FLOAT, voxels, VKL_DATA_DEFAULT, 0);

  VKLData attributes[] = {data0, data1, data2};

  VKLData attributesData =
      vklNewData(numAttributes, VKL_DATA, attributes, VKL_DATA_DEFAULT, 0);

  vklRelease(data0);
  vklRelease(data1);
  vklRelease(data2);

  vklSetData(volume, "data", attributesData);
  vklRelease(attributesData);

  vklCommit(volume);

  demoScalarAPI(volume);
  demoVectorAPI(volume);
  demoStreamAPI(volume);

  vklRelease(volume);

  vklShutdown();

  free(voxels);

  return 0;
}
