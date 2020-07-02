Examples
========

Open VKL ships with simple tutorial applications demonstrating the basic
usage of the API, as well as full renderers showing recommended usage.

Tutorials
---------

Simple tutorials can be found in the `examples/` directory. These are:

* `vklTutorial.c` : usage of the C API
* `vklTutorialISPC.[cpp,ispc]` : combined usage of the C and ISPC APIs

For quick reference, the contents of `vklTutorial.c` are shown below.

``` cpp

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

  // value range
  vkl_range1f valueRange = vklGetValueRange(volume);
  printf("\tvalue range = (%f %f)\n\n", valueRange.lower, valueRange.upper);

  // sample, gradient
  vkl_vec3f coord = {1.f, 1.f, 1.f};
  float sample    = vklComputeSample(sampler, &coord);
  vkl_vec3f grad  = vklComputeGradient(sampler, &coord);
  printf("\tcoord = %f %f %f\n", coord.x, coord.y, coord.z);
  printf("\t\tsample = %f\n", sample);
  printf("\t\tgrad   = %f %f %f\n\n", grad.x, grad.y, grad.z);

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

  vkl_vvec3f4 coord4;  // structure-of-array layout
  int valid[4];
  for (int i = 0; i < 4; i++) {
    coord4.x[i] = coord4.y[i] = coord4.z[i] = i;
    valid[i] = -1;  // valid mask: 0 = not valid, -1 = valid
  }

  float sample4[4];
  vkl_vvec3f4 grad4;
  vklComputeSample4(valid, sampler, &coord4, sample4);
  vklComputeGradient4(valid, sampler, &coord4, &grad4);

  for (int i = 0; i < 4; i++) {
    printf(
        "\tcoord[%d] = %f %f %f\n", i, coord4.x[i], coord4.y[i], coord4.z[i]);
    printf("\t\tsample[%d] = %f\n", i, sample4[i]);
    printf(
        "\t\tgrad[%d]   = %f %f %f\n", i, grad4.x[i], grad4.y[i], grad4.z[i]);
  }

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
    coord[i].x = coord[i].y = coord[i].z = i;
  }

  float sample[5];
  vkl_vec3f grad[5];
  vklComputeSampleN(sampler, 5, coord, sample);
  vklComputeGradientN(sampler, 5, coord, grad);

  for (int i = 0; i < 5; i++) {
    printf("\tcoord[%d] = %f %f %f\n", i, coord[i].x, coord[i].y, coord[i].z);
    printf("\t\tsample[%d] = %f\n", i, sample[i]);
    printf("\t\tgrad[%d]   = %f %f %f\n", i, grad[i].x, grad[i].y, grad[i].z);
  }

  vklRelease(sampler);
}

int main()
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  int dimensions[] = {128, 128, 128};

  const int numVoxels = dimensions[0] * dimensions[1] * dimensions[2];

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

  // x-grad sample volume
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)i;

  VKLData data = vklNewData(numVoxels, VKL_FLOAT, voxels, VKL_DATA_DEFAULT, 0);
  vklSetData(volume, "data", data);
  vklRelease(data);

  vklCommit(volume);

  demoScalarAPI(volume);
  demoVectorAPI(volume);
  demoStreamAPI(volume);

  vklRelease(volume);

  vklShutdown();

  free(voxels);

  return 0;
}
```

Interactive examples
--------------------

Open VKL also ships with an interactive example application, `vklExamples`. This
interactive viewer demonstrates multiple example renderers including a path
tracer, isosurface renderer (using hit iterators), and ray marcher. The viewer
UI supports switching between renderers interactively.

Each renderer has both a C++ and ISPC implementation showing recommended API
usage. These implementations are available in the
`examples/interactive/renderers/` directory.
