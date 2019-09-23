# makeAMR documentation

Converts structured grid data to an AMR representation.  In short, this method
works by building the AMR representation from the finest grid level to the
coarsest. The structured grid data provided is considered to be the finest
data. Data regions with enough variation remain at this level. Regions without
sufficient variation are coarsened by averaging the values in the region.

```
void makeAMR(const std::vector<float> &voxels,
             const vec3i inGridDims,
             const int numLevels,
             const int blockSize,
             const int refinementLevel,
             const float threshold,
             std::vector<box3i> &blockBounds,
             std::vector<int> &refinementLevels,
             std::vector<float> &cellWidths,
             std::vector<std::vector<float>> &brickData)
```

## Input parameters

* `voxels` - input structured grid voxel data
* `inGridDims` - input structured grid dimensions (can be non-cube)
* `numLevels` - the number of refinement levels to create
* `blockSize` - the size of blocks (in units of the next finest refinement
  level)
* `refinementLevel` - the scale by which each level grows/shrinks
* `threshold` - how much data must vary to prevent being coarsened

## Output parameters

* `blockBounds` - bounding boxes for all data blocks in the AMR data
* `refinementLevels` - the refinement level for each block in the above vector
* `cellWidths` - the cell width for each _refinement level_ (not per block)
* `blockData` - a vector of vectors containing the data values for each block
