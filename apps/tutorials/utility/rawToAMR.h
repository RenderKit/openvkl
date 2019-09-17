#include <vector>
#include "openvkl/common.h"
#include "ospcommon/math/box.h"
#include "ospcommon/math/vec.h"

using namespace ospcommon::math;

void makeAMR(const std::vector<float> &in,
             const vec3i inGridDims,
             const int numLevels,
             const int blockSize,
             const int refinementLevel,
             const float threshold,
             std::vector<box3i> &blockBounds,
             std::vector<int> &refinementLevels,
             std::vector<float> &cellWidths,
             std::vector<std::vector<float>> &brickData);
