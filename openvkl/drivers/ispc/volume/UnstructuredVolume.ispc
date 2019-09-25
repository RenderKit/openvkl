// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#include "UnstructuredVolume.ih"

struct LinearSpace3f
{
  vec3f vx;
  vec3f vy;
  vec3f vz;
};

inline varying LinearSpace3f
make_LinearSpace3f(const varying vec3f x, const varying vec3f y, const varying vec3f z) {
  varying LinearSpace3f l; l.vx = x; l.vy = y; l.vz = z; return l;
}

inline varying float det(const varying LinearSpace3f l) { return dot(l.vx,cross(l.vy,l.vz)); }

// Read 32/64-bit integer value from given array
static inline uniform uint64 readInteger(const uint32* uniform array,
                                         const uniform bool is32Bit,
                                         const uniform uint64 id)
{
  const uniform uint64 value =
    *((const uint64* uniform)(array + (is32Bit ? id : id << 1)));
  return value & (is32Bit ? 0x00000000ffffffffull : 0xffffffffffffffffull);
}

// Get cell offset (location) in index array
static inline uniform uint64 getCellOffset(const VKLUnstructuredVolume* uniform self,
                                           const uniform uint64 id)
{
  return readInteger(self->cell, self->cell32Bit, id) + self->cellSkipIds;
}

// Get vertex index from index array
static inline uniform uint64 getVertexId(const VKLUnstructuredVolume* uniform self,
                                         const uniform uint64 id)
{
  return readInteger(self->index, self->index32Bit, id);
}

static inline uniform vec3f calcPlaneNormal(const VKLUnstructuredVolume* uniform self,
                                            const uniform uint64 id,
                                            const uniform uint32 plane[3])
{
  // Retrieve cell offset first
  const uniform uint64 cOffset = getCellOffset(self, id);
  const vec3f* const uniform vtx = self->vertex;

  // Get 3 vertices for normal calculation
  const uniform vec3f& v0 = vtx[getVertexId(self, cOffset + plane[0])];
  const uniform vec3f& v1 = vtx[getVertexId(self, cOffset + plane[1])];
  const uniform vec3f& v2 = vtx[getVertexId(self, cOffset + plane[2])];

  // Calculate normal
  return normalize(cross(v0 - v1, v2 - v1));
}

static inline uniform vec3f tetrahedronNormal(const VKLUnstructuredVolume* uniform self,
                                              const uniform uint64 id,
                                              const uniform int planeID)
{
  // Get precomputed normal if available
  if (self->faceNormals)
    return self->faceNormals[(id * 6) + planeID];

  // Prepare vertex offset bys plane
  const uniform uint32 planes[4][3] =
    {{2, 0, 1}, {3, 1, 0}, {3, 2, 1}, {2, 3, 0}};
  return calcPlaneNormal(self, id, planes[planeID]);
}

static inline uniform vec3f hexahedronNormal(const VKLUnstructuredVolume *uniform self,
                                             const uniform uint64 id,
                                             const uniform int planeID)
{
  // Get precomputed normal if available
  if (self->faceNormals)
    return self->faceNormals[(id * 6) + planeID];

  // Prepare vertex offsets by plane
  const uniform uint32 planes[6][3] =
    {{3, 0, 1}, {5, 1, 0}, {6, 2, 1}, {7, 3, 2}, {7, 4, 0}, {6, 5, 4}};
  return calcPlaneNormal(self, id, planes[planeID]);
}

static inline uniform vec3f wedgeNormal(const VKLUnstructuredVolume *uniform self,
                                        const uniform uint64 id,
                                        const uniform int planeID)
{
  // Get precomputed normal if available
  if (self->faceNormals)
    return self->faceNormals[(id * 6) + planeID];

  // Prepare vertex offsets by plane
  const uniform uint32 planes[5][3] =
    {{2, 0, 1}, {4, 1, 0}, {5, 2, 1}, {5, 3, 0}, {5, 4, 3}};
  return calcPlaneNormal(self, id, planes[planeID]);
}

static inline uniform vec3f pyramidNormal(const VKLUnstructuredVolume *uniform self,
                                          const uniform uint64 id,
                                          const uniform int planeID)
{
  // Get precomputed normal if available
  if (self->faceNormals)
    return self->faceNormals[(id * 6) + planeID];

  // Prepare vertex offsets by plane
  const uniform uint32 planes[5][3] =
    {{3, 0, 1}, {4, 1, 0}, {4, 2, 1}, {4, 3, 2}, {3, 4, 0}};
  return calcPlaneNormal(self, id, planes[planeID]);
}

static bool intersectAndSampleTet(const void *uniform userData,
                                  uniform uint64 id,
                                  uniform bool assumeInside,
                                  float &result,
                                  vec3f samplePos)
{
  const VKLUnstructuredVolume* uniform self = (const VKLUnstructuredVolume* uniform) userData;

  // Get cell offset in index buffer
  const uniform uint64 cOffset = getCellOffset(self, id);

  const vec3f* uniform vtx = self->vertex;
  const uniform vec3f p0 = vtx[getVertexId(self, cOffset + 0)];
  const uniform vec3f p1 = vtx[getVertexId(self, cOffset + 1)];
  const uniform vec3f p2 = vtx[getVertexId(self, cOffset + 2)];
  const uniform vec3f p3 = vtx[getVertexId(self, cOffset + 3)];

  const uniform vec3f norm0 = tetrahedronNormal(self, id, 0);
  const uniform vec3f norm1 = tetrahedronNormal(self, id, 1);
  const uniform vec3f norm2 = tetrahedronNormal(self, id, 2);
  const uniform vec3f norm3 = tetrahedronNormal(self, id, 3);

  // Distance from the world point to the faces.
  const float d0 = dot(norm0, p0 - samplePos);
  const float d1 = dot(norm1, p1 - samplePos);
  const float d2 = dot(norm2, p2 - samplePos);
  const float d3 = dot(norm3, p3 - samplePos);

  // Exit if samplePos is outside the cell
  if (!assumeInside && !(d0 > 0 && d1 > 0 && d2 > 0 && d3 > 0))
    return false;

  // Skip interpolation if values are defined per cell
  if (self->cellValue) {
    result = self->cellValue[id];
    return true;
  }

  // Distance of tetrahedron corners to their opposite faces.
  const uniform float h0 = dot(norm0, p0 - p3);
  const uniform float h1 = dot(norm1, p1 - p2);
  const uniform float h2 = dot(norm2, p2 - p0);
  const uniform float h3 = dot(norm3, p3 - p1);

  // Local coordinates = ratio of distances.
  const float z0 = d0 / h0;
  const float z1 = d1 / h1;
  const float z2 = d2 / h2;
  const float z3 = d3 / h3;

  // Field/attribute values at the tetrahedron corners.
  const float* const uniform vv = self->vertexValue;
  const uniform float v0 = vv[getVertexId(self, cOffset + 0)];
  const uniform float v1 = vv[getVertexId(self, cOffset + 1)];
  const uniform float v2 = vv[getVertexId(self, cOffset + 2)];
  const uniform float v3 = vv[getVertexId(self, cOffset + 3)];

  // Interpolated field/attribute value at the world position.
  result = z0 * v3 + z1 * v2 + z2 * v0 + z3 * v1;
  return true;
}

//----------------------------------------------------------------------------
// Compute iso-parametric interpolation functions
//
static inline void wedgeInterpolationFunctions(float pcoords[3], float sf[6])
{
  sf[0] = (1.0 - pcoords[0] - pcoords[1]) * (1.0 - pcoords[2]);
  sf[1] = pcoords[0] * (1.0 - pcoords[2]);
  sf[2] = pcoords[1] * (1.0 - pcoords[2]);
  sf[3] = (1.0 - pcoords[0] - pcoords[1]) * pcoords[2];
  sf[4] = pcoords[0] * pcoords[2];
  sf[5] = pcoords[1] * pcoords[2];
}

//----------------------------------------------------------------------------
static inline void wedgeInterpolationDerivs(float pcoords[3], float derivs[18])
{
  // r-derivatives
  derivs[0] = -1.0 + pcoords[2];
  derivs[1] =  1.0 - pcoords[2];
  derivs[2] =  0.0;
  derivs[3] = -pcoords[2];
  derivs[4] =  pcoords[2];
  derivs[5] =  0.0;

  // s-derivatives
  derivs[6] = -1.0 + pcoords[2];
  derivs[7] =  0.0;
  derivs[8] =  1.0 - pcoords[2];
  derivs[9] = -pcoords[2];
  derivs[10] = 0.0;
  derivs[11] = pcoords[2];

  // t-derivatives
  derivs[12] = -1.0 + pcoords[0] + pcoords[1];
  derivs[13] = -pcoords[0];
  derivs[14] = -pcoords[1];
  derivs[15] =  1.0 - pcoords[0] - pcoords[1];
  derivs[16] =  pcoords[0];
  derivs[17] =  pcoords[1];
}

static const uniform float WEDGE_DIVERGED = 1.e6;
static const uniform int WEDGE_MAX_ITERATION = 10;
static const uniform float WEDGE_CONVERGED = 1.e-04;
static const uniform float WEDGE_OUTSIDE_CELL_TOLERANCE = 1.e-06;

static bool intersectAndSampleWedge(const void *uniform userData,
                                    uniform uint64 id,
                                    uniform bool assumeInside,
                                    float &result,
                                    vec3f samplePos)
{
  const VKLUnstructuredVolume *uniform self = (const VKLUnstructuredVolume * uniform) userData;

  float params[3] = { 0.5, 0.5, 0.5 };
  float pcoords[3] = { 0.5, 0.5, 0.5 };
  float derivs[18];
  float weights[6];

  // Get cell offset in index buffer
  const uniform uint64 cOffset = getCellOffset(self, id);

  const uniform int edges[9][2] = { {0,1}, {1,2}, {2,0},
                                    {3,4}, {4,5}, {5,3},
                                    {0,3}, {1,4}, {2,5} };
  uniform float longestEdge = 0;
  for (uniform int i = 0; i < 9; i++) {
      const uniform vec3f p0 = self->vertex[getVertexId(self, cOffset + edges[i][0])];
      const uniform vec3f p1 = self->vertex[getVertexId(self, cOffset + edges[i][1])];
      const uniform float dist = distance(p0, p1);
      if (longestEdge < dist)
         longestEdge = dist;
  }

  const uniform float volumeBound = longestEdge * longestEdge * longestEdge;
  const uniform float determinantTolerance =
      1e-20 < .00001*volumeBound ? 1e-20 : .00001*volumeBound;

  // Enter iteration loop
  uniform bool converged = false;
  for (uniform int iteration = 0; !converged && (iteration < WEDGE_MAX_ITERATION); iteration++) {
    // Calculate element interpolation functions and derivatives
    wedgeInterpolationFunctions(pcoords, weights);
    wedgeInterpolationDerivs(pcoords, derivs);

    // Calculate newton functions
    vec3f fcol = make_vec3f(0.f, 0.f, 0.f);
    vec3f rcol = make_vec3f(0.f, 0.f, 0.f);
    vec3f scol = make_vec3f(0.f, 0.f, 0.f);
    vec3f tcol = make_vec3f(0.f, 0.f, 0.f);
    for (uniform int i = 0; i < 6; i++) {
      const uniform vec3f pt = self->vertex[getVertexId(self, cOffset + i)];
      fcol = fcol + pt * weights[i];
      rcol = rcol + pt * derivs[i];
      scol = scol + pt * derivs[i + 6];
      tcol = tcol + pt * derivs[i + 12];
    }

    fcol = fcol - samplePos;

    // Compute determinants and generate improvements
    const float d = det(make_LinearSpace3f(rcol, scol, tcol));
    if (absf(d) < determinantTolerance) {
      return false;
    }

    pcoords[0] = params[0] - det(make_LinearSpace3f(fcol, scol, tcol)) / d;
    pcoords[1] = params[1] - det(make_LinearSpace3f(rcol, fcol, tcol)) / d;
    pcoords[2] = params[2] - det(make_LinearSpace3f(rcol, scol, fcol)) / d;

    // Convergence/divergence test - if neither, repeat
    if (((absf(pcoords[0] - params[0])) < WEDGE_CONVERGED) &&
        ((absf(pcoords[1] - params[1])) < WEDGE_CONVERGED) &&
        ((absf(pcoords[2] - params[2])) < WEDGE_CONVERGED)) {
      converged = true;
    } else if ((absf(pcoords[0]) > WEDGE_DIVERGED) ||
               (absf(pcoords[1]) > WEDGE_DIVERGED) ||
               (absf(pcoords[2]) > WEDGE_DIVERGED)) {
      return false;
    } else {
      params[0] = pcoords[0];
      params[1] = pcoords[1];
      params[2] = pcoords[2];
    }
  }

  if (!converged) {
    return false;
  }

  const uniform float lowerlimit = 0.0 - WEDGE_OUTSIDE_CELL_TOLERANCE;
  const uniform float upperlimit = 1.0 + WEDGE_OUTSIDE_CELL_TOLERANCE;
  if (assumeInside || (pcoords[0] >= lowerlimit && pcoords[0] <= upperlimit &&
                       pcoords[1] >= lowerlimit && pcoords[1] <= upperlimit &&
                       pcoords[2] >= lowerlimit && pcoords[2] <= upperlimit &&
                       pcoords[0] + pcoords[1] <= upperlimit)) {
    // Evaluation
    if (self->cellValue) {
      result = self->cellValue[id];
    } else {
      result = 0.f;
      wedgeInterpolationFunctions(pcoords, weights);
      for (uniform int i = 0; i < 6; i++) {
        result += weights[i] *
          self->vertexValue[getVertexId(self, cOffset + i)];
      }
    }

    return true;
  }

  return false;
}

static bool intersectAndSampleHexFast(const void *uniform userData,
                                      uniform uint64 id,
                                      float &result,
                                      vec3f samplePos)
{
  const VKLUnstructuredVolume* uniform self = (const VKLUnstructuredVolume* uniform)userData;

  // Get cell offset in index buffer
  const uniform uint64 cOffset = getCellOffset(self, id);

  // Calculate distances from each hexahedron face
  float dist[6];
  for (uniform int plane = 0; plane < 6; plane++) {
    const uniform vec3f v = self->vertex[getVertexId(self, cOffset + plane)];
    dist[plane] = dot(samplePos - v, hexahedronNormal(self, id, plane));
    if (dist[plane] > 0.f) // samplePos is outside of the cell
      return false;
  }

  // Skip interpolation if values are defined per cell
  if (self->cellValue) {
    result = self->cellValue[id];
    return true;
  }

  // Calculate 0..1 isoparametrics
  const float u0 = dist[2] / (dist[2] + dist[4]);
  const float v0 = dist[5] / (dist[5] + dist[0]);
  const float w0 = dist[3] / (dist[3] + dist[1]);
  const float u1 = 1.f - u0;
  const float v1 = 1.f - v0;
  const float w1 = 1.f - w0;

  // Do the trilinear interpolation
  const float* const uniform vv = self->vertexValue;
  result =
    u0 * v0 * w0 * vv[getVertexId(self, cOffset + 0)] +
    u1 * v0 * w0 * vv[getVertexId(self, cOffset + 1)] +
    u1 * v0 * w1 * vv[getVertexId(self, cOffset + 2)] +
    u0 * v0 * w1 * vv[getVertexId(self, cOffset + 3)] +
    u0 * v1 * w0 * vv[getVertexId(self, cOffset + 4)] +
    u1 * v1 * w0 * vv[getVertexId(self, cOffset + 5)] +
    u1 * v1 * w1 * vv[getVertexId(self, cOffset + 6)] +
    u0 * v1 * w1 * vv[getVertexId(self, cOffset + 7)];
  return true;
}

//----------------------------------------------------------------------------
// Compute iso-parametric interpolation functions
//
static inline void hexInterpolationFunctions(float pcoords[3], float sf[8])
{
  float rm, sm, tm;

  rm = 1.f - pcoords[0];
  sm = 1.f - pcoords[1];
  tm = 1.f - pcoords[2];

  sf[0] = rm * sm * tm;
  sf[1] = pcoords[0] * sm * tm;
  sf[2] = pcoords[0] *pcoords[1] * tm;
  sf[3] = rm * pcoords[1] * tm;
  sf[4] = rm * sm * pcoords[2];
  sf[5] = pcoords[0] * sm * pcoords[2];
  sf[6] = pcoords[0] * pcoords[1] * pcoords[2];
  sf[7] = rm * pcoords[1] * pcoords[2];
}

//----------------------------------------------------------------------------
static inline void hexInterpolationDerivs(float pcoords[3], float derivs[24])
{
  float rm, sm, tm;

  rm = 1.f - pcoords[0];
  sm = 1.f - pcoords[1];
  tm = 1.f - pcoords[2];

  // r-derivatives
  derivs[0] = -sm * tm;
  derivs[1] = sm * tm;
  derivs[2] = pcoords[1] * tm;
  derivs[3] = -pcoords[1] * tm;
  derivs[4] = -sm * pcoords[2];
  derivs[5] = sm * pcoords[2];
  derivs[6] = pcoords[1] * pcoords[2];
  derivs[7] = -pcoords[1] * pcoords[2];

  // s-derivatives
  derivs[8] = -rm * tm;
  derivs[9] = -pcoords[0] * tm;
  derivs[10] = pcoords[0] * tm;
  derivs[11] = rm * tm;
  derivs[12] = -rm * pcoords[2];
  derivs[13] = -pcoords[0] * pcoords[2];
  derivs[14] = pcoords[0] * pcoords[2];
  derivs[15] = rm * pcoords[2];

  // t-derivatives
  derivs[16] = -rm * sm;
  derivs[17] = -pcoords[0] * sm;
  derivs[18] = -pcoords[0] * pcoords[1];
  derivs[19] = -rm * pcoords[1];
  derivs[20] = rm * sm;
  derivs[21] = pcoords[0] * sm;
  derivs[22] = pcoords[0] * pcoords[1];
  derivs[23] = rm * pcoords[1];
}

static const uniform float HEX_DIVERGED = 1.e6;
static const uniform int HEX_MAX_ITERATION = 10;
static const uniform float HEX_CONVERGED = 1.e-05;
static const uniform float HEX_OUTSIDE_CELL_TOLERANCE = 1.e-06;

static bool intersectAndSampleHexIterative(const void *uniform userData,
                                           uniform uint64 id,
                                           uniform bool assumeInside,
                                           float &result,
                                           vec3f samplePos)
{
  const VKLUnstructuredVolume *uniform self = (const VKLUnstructuredVolume * uniform) userData;

  float params[3] = { 0.5, 0.5, 0.5 };
  float pcoords[3];
  float derivs[24];
  float weights[8];

  // Get cell offset in index buffer
  const uniform uint64 cOffset = getCellOffset(self, id);

  // Should precompute these
  const uniform int diagonals[4][2] = { {0, 6}, {1, 7}, {2, 4}, {3, 5} };
  uniform float longestDiagonal = 0;
  for (uniform int i = 0; i < 4; i++) {
      const uniform vec3f p0 = self->vertex[getVertexId(self, cOffset + diagonals[i][0])];
      const uniform vec3f p1 = self->vertex[getVertexId(self, cOffset + diagonals[i][1])];
      const uniform float dist = distance(p0, p1);
      if (longestDiagonal < dist)
         longestDiagonal = dist;
  }

  const uniform float volumeBound = longestDiagonal * longestDiagonal * longestDiagonal;
  const uniform float determinantTolerance =
      1e-20 < .00001*volumeBound ? 1e-20 : .00001*volumeBound;

  // Set initial position for Newton's method
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;

  // Enter iteration loop
  bool uniform converged = false;
  for (uniform int iteration = 0; !converged && (iteration < HEX_MAX_ITERATION); iteration++) {
    // Calculate element interpolation functions and derivatives
    hexInterpolationFunctions(pcoords, weights);
    hexInterpolationDerivs(pcoords, derivs);

    // Calculate newton functions
    vec3f fcol = make_vec3f(0.f, 0.f, 0.f);
    vec3f rcol = make_vec3f(0.f, 0.f, 0.f);
    vec3f scol = make_vec3f(0.f, 0.f, 0.f);
    vec3f tcol = make_vec3f(0.f, 0.f, 0.f);
    for (uniform int i = 0; i < 8; i++) {
      const uniform vec3f pt = self->vertex[getVertexId(self, cOffset + i)];
      fcol = fcol + pt * weights[i];
      rcol = rcol + pt * derivs[i];
      scol = scol + pt * derivs[i + 8];
      tcol = tcol + pt * derivs[i + 16];
    }

    fcol = fcol - samplePos;

    // Compute determinants and generate improvements
    const float d = det(make_LinearSpace3f(rcol, scol, tcol));
    if (absf(d) < determinantTolerance) {
      return false;
    }

    pcoords[0] = params[0] - det(make_LinearSpace3f(fcol, scol, tcol)) / d;
    pcoords[1] = params[1] - det(make_LinearSpace3f(rcol, fcol, tcol)) / d;
    pcoords[2] = params[2] - det(make_LinearSpace3f(rcol, scol, fcol)) / d;

    // Convergence/divergence test - if neither, repeat
    if (((absf(pcoords[0] - params[0])) < HEX_CONVERGED) &&
        ((absf(pcoords[1] - params[1])) < HEX_CONVERGED) &&
        ((absf(pcoords[2] - params[2])) < HEX_CONVERGED)) {
      converged = true;
    } else if ((absf(pcoords[0]) > HEX_DIVERGED) ||
               (absf(pcoords[1]) > HEX_DIVERGED) ||
               (absf(pcoords[2]) > HEX_DIVERGED)) {
      return false;
    } else {
      params[0] = pcoords[0];
      params[1] = pcoords[1];
      params[2] = pcoords[2];
    }
  }

  if (!converged) {
    return false;
  }

  const uniform float lowerlimit = 0.0 - HEX_OUTSIDE_CELL_TOLERANCE;
  const uniform float upperlimit = 1.0 + HEX_OUTSIDE_CELL_TOLERANCE;
  if (assumeInside ||
      (pcoords[0] >= lowerlimit && pcoords[0] <= upperlimit &&
       pcoords[1] >= lowerlimit && pcoords[1] <= upperlimit &&
       pcoords[2] >= lowerlimit && pcoords[2] <= upperlimit)) {
    // Evaluation
    if (self->cellValue) {
      result = self->cellValue[id];
    } else {
      result = 0.f;
      hexInterpolationFunctions(pcoords, weights);
      for (uniform int i = 0; i < 8; i++) {
        result += weights[i] *
          self->vertexValue[getVertexId(self, cOffset + i)];
      }
    }

    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
// Compute iso-parametric interpolation functions
//
static inline void pyramidInterpolationFunctions(float pcoords[3], float sf[5])
{
  float rm, sm, tm;

  rm = 1.f - pcoords[0];
  sm = 1.f - pcoords[1];
  tm = 1.f - pcoords[2];

  sf[0] = rm * sm * tm;
  sf[1] = pcoords[0] * sm * tm;
  sf[2] = pcoords[0] * pcoords[1] * tm;
  sf[3] = rm * pcoords[1] * tm;
  sf[4] = pcoords[2];
}

//----------------------------------------------------------------------------
static inline void pyramidInterpolationDerivs(float pcoords[3], float derivs[15])
{
  // r-derivatives
  derivs[0] = -(pcoords[1] - 1.f) * (pcoords[2] - 1.f);
  derivs[1] = (pcoords[1] - 1.f) * (pcoords[2] - 1.f);
  derivs[2] = pcoords[1] - pcoords[1] * pcoords[2];
  derivs[3] = pcoords[1] * (pcoords[2] - 1.f);
  derivs[4] =  0.f;

  // s-derivatives
  derivs[5] = -(pcoords[0] - 1.f) * (pcoords[2] - 1.f);
  derivs[6] =  pcoords[0] * (pcoords[2] - 1.f);
  derivs[7] =  pcoords[0] - pcoords[0] * pcoords[2];
  derivs[8] = (pcoords[0] - 1.f) * (pcoords[2] - 1.f);
  derivs[9] = 0.f;

  // t-derivatives
  derivs[10] = -(pcoords[0] - 1.f) * (pcoords[1] - 1.f);
  derivs[11] = pcoords[0] * (pcoords[1] - 1.f);
  derivs[12] = -pcoords[0] * pcoords[1];
  derivs[13] = (pcoords[0] - 1.f) * pcoords[1];
  derivs[14] = 1.f;
}

static const uniform float PYRAMID_DIVERGED = 1.e6;
static const uniform int PYRAMID_MAX_ITERATION = 10;
static const uniform float PYRAMID_CONVERGED = 1.e-04;
static const uniform float PYRAMID_OUTSIDE_CELL_TOLERANCE = 1.e-06;

static bool intersectAndSamplePyramid(const void *uniform userData,
                                      uniform uint64 id,
                                      uniform bool assumeInside,
                                      float &result,
                                      vec3f samplePos)
{
  const VKLUnstructuredVolume* uniform self = (const VKLUnstructuredVolume* uniform) userData;

  float params[3] = { 0.5, 0.5, 0.5 };
  float pcoords[3] = { 0.5, 0.5, 0.5 };
  float derivs[15];
  float weights[5];

  // Get cell offset in index buffer
  const uniform uint64 cOffset = getCellOffset(self, id);

  const uniform int edges[8][2] = { {0,1}, {1,2}, {2,3}, {3,0},
    {0,4}, {1,4}, {2,4}, {3,4} };
  uniform float longestEdge = 0;
  for (uniform int i = 0; i < 8; i++) {
    const uniform vec3f p0 = self->vertex[getVertexId(self, cOffset + edges[i][0])];
    const uniform vec3f p1 = self->vertex[getVertexId(self, cOffset + edges[i][1])];
    const uniform float dist = distance(p0, p1);
    if (longestEdge < dist)
      longestEdge = dist;
  }

  const uniform float volumeBound = longestEdge * longestEdge * longestEdge;
  const uniform float determinantTolerance =
    1e-20 < .00001*volumeBound ? 1e-20 : .00001*volumeBound;

  // Enter iteration loop
  uniform bool converged = false;
  for (uniform int iteration = 0; !converged && (iteration < PYRAMID_MAX_ITERATION); iteration++) {
    // Calculate element interpolation functions and derivatives
    pyramidInterpolationFunctions(pcoords, weights);
    pyramidInterpolationDerivs(pcoords, derivs);

    // Calculate newton functions
    vec3f fcol = make_vec3f(0.f, 0.f, 0.f);
    vec3f rcol = make_vec3f(0.f, 0.f, 0.f);
    vec3f scol = make_vec3f(0.f, 0.f, 0.f);
    vec3f tcol = make_vec3f(0.f, 0.f, 0.f);
    for (uniform int i = 0; i < 5; i++) {
      const uniform vec3f pt = self->vertex[getVertexId(self, cOffset + i)];
      fcol = fcol + pt * weights[i];
      rcol = rcol + pt * derivs[i];
      scol = scol + pt * derivs[i + 5];
      tcol = tcol + pt * derivs[i + 10];
    }

    fcol = fcol - samplePos;

    // Compute determinants and generate improvements
    const float d = det(make_LinearSpace3f(rcol, scol, tcol));
    if (absf(d) < determinantTolerance) {
      return false;
    }

    pcoords[0] = params[0] - det(make_LinearSpace3f(fcol, scol, tcol)) / d;
    pcoords[1] = params[1] - det(make_LinearSpace3f(rcol, fcol, tcol)) / d;
    pcoords[2] = params[2] - det(make_LinearSpace3f(rcol, scol, fcol)) / d;

    // Convergence/divergence test - if neither, repeat
    if (((absf(pcoords[0] - params[0])) < PYRAMID_CONVERGED) &&
        ((absf(pcoords[1] - params[1])) < PYRAMID_CONVERGED) &&
        ((absf(pcoords[2] - params[2])) < PYRAMID_CONVERGED)) {
      converged = true;
    } else if ((absf(pcoords[0]) > PYRAMID_DIVERGED) ||
               (absf(pcoords[1]) > PYRAMID_DIVERGED) ||
               (absf(pcoords[2]) > PYRAMID_DIVERGED)) {
      return false;
    } else {
      params[0] = pcoords[0];
      params[1] = pcoords[1];
      params[2] = pcoords[2];
    }
  }

  if (!converged) {
    return false;
  }

  const uniform float lowerlimit = 0.0 - PYRAMID_OUTSIDE_CELL_TOLERANCE;
  const uniform float upperlimit = 1.0 + PYRAMID_OUTSIDE_CELL_TOLERANCE;
  if (assumeInside ||
      (pcoords[0] >= lowerlimit && pcoords[0] <= upperlimit &&
       pcoords[1] >= lowerlimit && pcoords[1] <= upperlimit &&
       pcoords[2] >= lowerlimit && pcoords[2] <= upperlimit)) {
    // Evaluation
    if (self->cellValue) {
      result = self->cellValue[id];
    } else {
      result = 0.f;
      pyramidInterpolationFunctions(pcoords, weights);
      for (uniform int i = 0; i < 5; i++) {
        result += weights[i] *
          self->vertexValue[getVertexId(self, cOffset + i)];
      }
    }

    return true;
  }

  return false;
}

static bool intersectAndSampleCell(const void *uniform userData,
                                   uniform uint64 id,
                                   float &result,
                                   vec3f samplePos)
{
  bool hit = false;
  const VKLUnstructuredVolume* uniform self = (const VKLUnstructuredVolume* uniform)userData;

  switch (self->cellType[id]) {
  case VKL_TETRAHEDRON:
    hit = intersectAndSampleTet(userData, id, false, result, samplePos);
    break;
  case VKL_HEXAHEDRON:
    if (!self->hexIterative)
      hit = intersectAndSampleHexFast(userData, id, result, samplePos);
    else
      hit = intersectAndSampleHexIterative(userData, id, false, result, samplePos);
    break;
  case VKL_WEDGE:
    hit = intersectAndSampleWedge(userData, id, false, result, samplePos);
    break;
  case VKL_PYRAMID:
    hit = intersectAndSamplePyramid(userData, id, false, result, samplePos);
    break;
  }

  // Return true if samplePos is inside the cell
  return hit;
}

inline varying float VKLUnstructuredVolume_sample(
    const void *uniform _self, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  const VKLUnstructuredVolume *uniform self = (const VKLUnstructuredVolume * uniform) _self;

  float results = floatbits(0xffffffff);  /* NaN */

  traverse(self->bvh, _self, intersectAndSampleCell, results, worldCoordinates);

  return results;
}

inline varying vec3f VKLUnstructuredVolume_computeGradient(
    const void *uniform _self,
    const varying vec3f &objectCoordinates)
{
  // Cast to the actual Volume subtype.
  const VKLUnstructuredVolume *uniform self = (const VKLUnstructuredVolume * uniform) _self;

  // gradient step in each dimension (object coordinates)
  vec3f gradientStep = self->gradientStep;

  // compute via forward or backward differences depending on volume boundary
  const vec3f gradientExtent = objectCoordinates + gradientStep;

  if (gradientExtent.x >= self->boundingBox.upper.x)
    gradientStep.x *= -1.f;

  if (gradientExtent.y >= self->boundingBox.upper.y)
    gradientStep.y *= -1.f;

  if (gradientExtent.z >= self->boundingBox.upper.z)
    gradientStep.z *= -1.f;

  vec3f gradient;

  float sample = VKLUnstructuredVolume_sample(self, objectCoordinates);

  gradient.x =
      VKLUnstructuredVolume_sample(
          self, objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f)) -
      sample;
  gradient.y =
      VKLUnstructuredVolume_sample(
          self, objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f)) -
      sample;
  gradient.z =
      VKLUnstructuredVolume_sample(
          self, objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z)) -
      sample;

  return gradient / gradientStep;
}

export void VKLUnstructuredVolume_sample_export(
    uniform const int *uniform imask,
    void *uniform _volume,
    const void *uniform _objectCoordinates,
    void *uniform _samples)
{
  cif(imask[programIndex])
  {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float *uniform samples = (varying float *uniform)_samples;

    *samples = VKLUnstructuredVolume_sample(_volume, *objectCoordinates);
  }
}

export void VKLUnstructuredVolume_gradient_export(
    uniform const int *uniform imask,
    void *uniform _volume,
    const void *uniform _objectCoordinates,
    void *uniform _gradients)
{
  cif (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying vec3f *uniform gradients = (varying vec3f * uniform) _gradients;

    *gradients =
        VKLUnstructuredVolume_computeGradient(_volume, *objectCoordinates);
  }
}

export void *uniform
VKLUnstructuredVolume_Constructor()
{
  uniform VKLUnstructuredVolume *uniform self = uniform new uniform VKLUnstructuredVolume;

  self->super.computeSample = VKLUnstructuredVolume_sample;

  return self;
}

export void VKLUnstructuredVolume_set(void *uniform _self,
                                   const uniform box3f& _bbox,
                                   const vec3f* uniform _vertex,
                                   const uint32* uniform _index,
                                   const uniform bool _index32Bit,
                                   const float* uniform _vertexValue,
                                   const float* uniform _cellValue,
                                   const uint32* uniform _cell,
                                   const uniform bool _cell32Bit,
                                   const uniform uint32 _cellSkipIds,
                                   const uint8* uniform _cellType,
                                   uniform int64 rootRef,
                                   const void* uniform _bvhNode,
                                   const int64* uniform _bvhPrimID,
                                   const vec3f* uniform _faceNormals,
                                   const uniform bool _hexIterative)
{
  uniform VKLUnstructuredVolume *uniform self =
      (uniform VKLUnstructuredVolume * uniform) _self;

  self->vertex       = _vertex;
  self->index        = _index;
  self->index32Bit   = _index32Bit;
  self->vertexValue  = _vertexValue;
  self->cellValue    = _cellValue;
  self->cell         = _cell;
  self->cell32Bit    = _cell32Bit;
  self->cellSkipIds  = _cellSkipIds;
  self->cellType     = _cellType;

  self->faceNormals  = _faceNormals;
  self->hexIterative = _hexIterative;

  self->boundingBox = _bbox;

  self->gradientStep = make_vec3f(0.01f * reduce_min(self->boundingBox.upper - self->boundingBox.lower));

  self->bvh.rootRef = rootRef;
  self->bvh.node    = (MinMaxBVH2Node * uniform) _bvhNode;
  self->bvh.primID  = _bvhPrimID;
}
