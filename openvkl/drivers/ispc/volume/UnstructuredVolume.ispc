// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "UnstructuredSamplerBase.ih"
#include "UnstructuredVolume.ih"

inline bool pointInAABBTest(const uniform box3fa &box, const vec3f &point)
{
  bool t1 = point.x >= box.lower.x;
  bool t2 = point.y >= box.lower.y;
  bool t3 = point.z >= box.lower.z;
  bool t4 = point.x <= box.upper.x;
  bool t5 = point.y <= box.upper.y;
  bool t6 = point.z <= box.upper.z;
  return t1 & t2 & t3 & t4 & t5 & t6;
}

#define template_traverseEmbree(userFuncType, resultType)                      \
  inline void traverseEmbree(uniform Node *uniform root,                       \
                             const void *uniform userPtr,                      \
                             uniform userFuncType userFunc,                    \
                             resultType &result,                               \
                             const vec3f &samplePos)                           \
  {                                                                            \
    uniform Node *uniform node = root;                                         \
    uniform Node *uniform nodeStack[32]; /* xxx */                             \
    uniform int stackPtr = 0;                                                  \
                                                                               \
    while (1) {                                                                \
      uniform bool isLeaf = (node->nominalLength < 0);                         \
      if (isLeaf) {                                                            \
        uniform LeafNode *uniform leaf = (uniform LeafNode * uniform) node;    \
        if (userFunc(userPtr, leaf->cellID, result, samplePos))                \
          return;                                                              \
      } else {                                                                 \
        uniform InnerNode *uniform inner = (uniform InnerNode * uniform) node; \
        const bool in0 = pointInAABBTest(inner->bounds[0], samplePos);         \
        const bool in1 = pointInAABBTest(inner->bounds[1], samplePos);         \
                                                                               \
        if (any(in0)) {                                                        \
          if (any(in1)) {                                                      \
            nodeStack[stackPtr++] = inner->children[1];                        \
            node                  = inner->children[0];                        \
            continue;                                                          \
          } else {                                                             \
            node = inner->children[0];                                         \
            continue;                                                          \
          }                                                                    \
        } else {                                                               \
          if (any(in1)) {                                                      \
            node = inner->children[1];                                         \
            continue;                                                          \
          } else {                                                             \
            /* Do nothing, just pop. */                                        \
          }                                                                    \
        }                                                                      \
      }                                                                        \
      if (stackPtr == 0)                                                       \
        return;                                                                \
      node = nodeStack[--stackPtr];                                            \
    }                                                                          \
  }

// #define USE_STACKLESS_TRAVERSAL

#ifndef USE_STACKLESS_TRAVERSAL
template_traverseEmbree(intersectAndSamplePrim, float);
#endif

template_traverseEmbree(intersectAndGradientPrim, vec3f);
#undef template_traverseEmbree

#ifdef USE_STACKLESS_TRAVERSAL
// stackless traversal; this approach may be useful with traversal caches in the
// future
inline void traverseEmbree(uniform Node *uniform root,
                           const void *uniform userPtr,
                           uniform intersectAndSamplePrim userFunc,
                           float &result,
                           const vec3f &samplePos)
{
  uniform Node *uniform node = root;
  uniform uint32 bitstack    = 0;

  while (1) {
    uniform bool isInner = (node->nominalLength >= 0);

    if (isInner) {
      uniform InnerNode *uniform inner = (uniform InnerNode * uniform) node;
      const bool in0 = pointInAABBTest(inner->bounds[0], samplePos);
      const bool in1 = pointInAABBTest(inner->bounds[1], samplePos);

      if (any(in0) || any(in1)) {
        bitstack = bitstack << 1;

        if (any(in0) && any(in1)) {
          node     = inner->children[0];  // should choose nearest child!
          bitstack = bitstack | 1;
        } else {
          if (any(in0)) {
            node = inner->children[0];
          } else {
            node = inner->children[1];
          }
        }

        continue;
      }
    } else {
      // leaf
      uniform LeafNode *uniform leaf = (uniform LeafNode * uniform) node;
      const bool in0 = pointInAABBTest(leaf->bounds, samplePos);
      if (in0 && userFunc(userPtr, leaf->cellID, result, samplePos))
        return;
    }

    // backtrack
    while ((bitstack & 1) == 0) {
      if (!bitstack) {
        return;
      }
      node     = node->parent;
      bitstack = bitstack >> 1;
    }

    // sibling pointer could be used here if we had it
    node = sibling(node);

    bitstack = bitstack ^ 1;
  }
}
#endif

struct LinearSpace3f
{
  vec3f vx;
  vec3f vy;
  vec3f vz;
};

inline varying LinearSpace3f make_LinearSpace3f(const varying vec3f x,
                                                const varying vec3f y,
                                                const varying vec3f z)
{
  varying LinearSpace3f l;
  l.vx = x;
  l.vy = y;
  l.vz = z;
  return l;
}

inline varying float det(const varying LinearSpace3f l)
{
  return dot(l.vx, cross(l.vy, l.vz));
}

// Read 32/64-bit integer value from given array
static inline uniform uint64 readInteger(const uniform Data1D array,
                                         const uniform bool is32Bit,
                                         const uniform uint64 id)
{
  return is32Bit ? get_uint32(array, id) : get_uint64(array, id);
}

// Get cell offset (location) in index array
static inline uniform uint64 getCellOffset(
    const VKLUnstructuredVolume *uniform self, const uniform uint64 id)
{
  return readInteger(self->cell, self->cell32Bit, id) + self->cellSkipIds;
}

// Get vertex index from index array
static inline uniform uint64
getVertexId(const VKLUnstructuredVolume *uniform self, const uniform uint64 id)
{
  return readInteger(self->index, self->index32Bit, id);
}

static inline uniform vec3f
calcPlaneNormal(const VKLUnstructuredVolume *uniform self,
                const uniform uint64 id,
                const uniform uint32 plane[3])
{
  // Retrieve cell offset first
  const uniform uint64 cOffset = getCellOffset(self, id);

  // Get 3 vertices for normal calculation
  const uniform vec3f v0 =
      get_vec3f(self->vertex, getVertexId(self, cOffset + plane[0]));
  const uniform vec3f v1 =
      get_vec3f(self->vertex, getVertexId(self, cOffset + plane[1]));
  const uniform vec3f v2 =
      get_vec3f(self->vertex, getVertexId(self, cOffset + plane[2]));

  // Calculate normal
  return normalize(cross(v0 - v1, v2 - v1));
}

static inline uniform vec3f
tetrahedronNormal(const VKLUnstructuredVolume *uniform self,
                  const uniform uint64 id,
                  const uniform int planeID)
{
  // Get precomputed normal if available
  if (self->faceNormals)
    return self->faceNormals[(id * 6) + planeID];

  // Prepare vertex offset bys plane
  const uniform uint32 planes[4][3] = {
      {2, 0, 1}, {3, 1, 0}, {3, 2, 1}, {2, 3, 0}};
  return calcPlaneNormal(self, id, planes[planeID]);
}

static inline uniform vec3f
hexahedronNormal(const VKLUnstructuredVolume *uniform self,
                 const uniform uint64 id,
                 const uniform int planeID)
{
  // Get precomputed normal if available
  if (self->faceNormals)
    return self->faceNormals[(id * 6) + planeID];

  // Prepare vertex offsets by plane
  const uniform uint32 planes[6][3] = {
      {3, 0, 1}, {5, 1, 0}, {6, 2, 1}, {7, 3, 2}, {7, 4, 0}, {6, 5, 4}};
  return calcPlaneNormal(self, id, planes[planeID]);
}

static inline uniform vec3f
wedgeNormal(const VKLUnstructuredVolume *uniform self,
            const uniform uint64 id,
            const uniform int planeID)
{
  // Get precomputed normal if available
  if (self->faceNormals)
    return self->faceNormals[(id * 6) + planeID];

  // Prepare vertex offsets by plane
  const uniform uint32 planes[5][3] = {
      {2, 0, 1}, {4, 1, 0}, {5, 2, 1}, {5, 3, 0}, {5, 4, 3}};
  return calcPlaneNormal(self, id, planes[planeID]);
}

static inline uniform vec3f
pyramidNormal(const VKLUnstructuredVolume *uniform self,
              const uniform uint64 id,
              const uniform int planeID)
{
  // Get precomputed normal if available
  if (self->faceNormals)
    return self->faceNormals[(id * 6) + planeID];

  // Prepare vertex offsets by plane
  const uniform uint32 planes[5][3] = {
      {3, 0, 1}, {4, 1, 0}, {4, 2, 1}, {4, 3, 2}, {3, 4, 0}};
  return calcPlaneNormal(self, id, planes[planeID]);
}

static bool intersectAndSampleTet(const void *uniform userData,
                                  uniform uint64 id,
                                  uniform bool assumeInside,
                                  float &result,
                                  vec3f samplePos)
{
  const VKLUnstructuredVolume *uniform self =
      (const VKLUnstructuredVolume *uniform)userData;

  // Get cell offset in index buffer
  const uniform uint64 cOffset = getCellOffset(self, id);

  const uniform vec3f p0 =
      get_vec3f(self->vertex, getVertexId(self, cOffset + 0));
  const uniform vec3f p1 =
      get_vec3f(self->vertex, getVertexId(self, cOffset + 1));
  const uniform vec3f p2 =
      get_vec3f(self->vertex, getVertexId(self, cOffset + 2));
  const uniform vec3f p3 =
      get_vec3f(self->vertex, getVertexId(self, cOffset + 3));

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
  if (valid(self->cellValue)) {
    result = get_float(self->cellValue, id);
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
  const uniform float v0 =
      get_float(self->vertexValue, getVertexId(self, cOffset + 0));
  const uniform float v1 =
      get_float(self->vertexValue, getVertexId(self, cOffset + 1));
  const uniform float v2 =
      get_float(self->vertexValue, getVertexId(self, cOffset + 2));
  const uniform float v3 =
      get_float(self->vertexValue, getVertexId(self, cOffset + 3));

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
  derivs[1] = 1.0 - pcoords[2];
  derivs[2] = 0.0;
  derivs[3] = -pcoords[2];
  derivs[4] = pcoords[2];
  derivs[5] = 0.0;

  // s-derivatives
  derivs[6]  = -1.0 + pcoords[2];
  derivs[7]  = 0.0;
  derivs[8]  = 1.0 - pcoords[2];
  derivs[9]  = -pcoords[2];
  derivs[10] = 0.0;
  derivs[11] = pcoords[2];

  // t-derivatives
  derivs[12] = -1.0 + pcoords[0] + pcoords[1];
  derivs[13] = -pcoords[0];
  derivs[14] = -pcoords[1];
  derivs[15] = 1.0 - pcoords[0] - pcoords[1];
  derivs[16] = pcoords[0];
  derivs[17] = pcoords[1];
}

static const uniform float WEDGE_DIVERGED               = 1.e6;
static const uniform int WEDGE_MAX_ITERATION            = 10;
static const uniform float WEDGE_CONVERGED              = 1.e-04;
static const uniform float WEDGE_OUTSIDE_CELL_TOLERANCE = 1.e-06;

static bool intersectAndSampleWedge(const void *uniform userData,
                                    uniform uint64 id,
                                    uniform bool assumeInside,
                                    float &result,
                                    vec3f samplePos)
{
  const VKLUnstructuredVolume *uniform self =
      (const VKLUnstructuredVolume *uniform)userData;

  float pcoords[3] = {0.5, 0.5, 0.5};
  float derivs[18];
  float weights[6];

  // Get cell offset in index buffer
  const uniform uint64 cOffset             = getCellOffset(self, id);
  const uniform float determinantTolerance = self->iterativeTolerance[id];

  // Enter iteration loop
  bool converged = false;
  for (uniform int iteration = 0;
       !converged && (iteration < WEDGE_MAX_ITERATION);
       iteration++) {
    unmasked
    {
      // Calculate element interpolation functions and derivatives
      wedgeInterpolationFunctions(pcoords, weights);
      wedgeInterpolationDerivs(pcoords, derivs);

      // Calculate newton functions
      vec3f fcol = make_vec3f(0.f, 0.f, 0.f);
      vec3f rcol = make_vec3f(0.f, 0.f, 0.f);
      vec3f scol = make_vec3f(0.f, 0.f, 0.f);
      vec3f tcol = make_vec3f(0.f, 0.f, 0.f);
      for (uniform int i = 0; i < 6; i++) {
        const uniform vec3f pt =
            get_vec3f(self->vertex, getVertexId(self, cOffset + i));
        fcol = fcol + pt * weights[i];
        rcol = rcol + pt * derivs[i];
        scol = scol + pt * derivs[i + 6];
        tcol = tcol + pt * derivs[i + 12];
      }

      fcol = fcol - samplePos;

      // Compute determinants and generate improvements
      const float d = det(make_LinearSpace3f(rcol, scol, tcol));
    }

    if (absf(d) < determinantTolerance) {
      return false;
    }

    const float d0 = det(make_LinearSpace3f(fcol, scol, tcol)) / d;
    const float d1 = det(make_LinearSpace3f(rcol, fcol, tcol)) / d;
    const float d2 = det(make_LinearSpace3f(rcol, scol, fcol)) / d;

    pcoords[0] = pcoords[0] - d0;
    pcoords[1] = pcoords[1] - d1;
    pcoords[2] = pcoords[2] - d2;

    // Convergence/divergence test - if neither, repeat
    if ((absf(d0) < WEDGE_CONVERGED) & (absf(d1) < WEDGE_CONVERGED) &
        (absf(d2) < WEDGE_CONVERGED)) {
      converged = true;
    } else if ((absf(pcoords[0]) > WEDGE_DIVERGED) |
               (absf(pcoords[1]) > WEDGE_DIVERGED) |
               (absf(pcoords[2]) > WEDGE_DIVERGED)) {
      return false;
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
    if (valid(self->cellValue)) {
      result = get_float(self->cellValue, id);
    } else {
      float val = 0.f;
      for (uniform int i = 0; i < 6; i++) {
        val += weights[i] *
               get_float(self->vertexValue, getVertexId(self, cOffset + i));
      }
      result = val;
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
  const VKLUnstructuredVolume *uniform self =
      (const VKLUnstructuredVolume *uniform)userData;

  // Get cell offset in index buffer
  const uniform uint64 cOffset = getCellOffset(self, id);

  // Calculate distances from each hexahedron face
  float dist[6];
  for (uniform int plane = 0; plane < 6; plane++) {
    const uniform vec3f v =
        get_vec3f(self->vertex, getVertexId(self, cOffset + plane));
    dist[plane] = dot(samplePos - v, hexahedronNormal(self, id, plane));
    if (dist[plane] > 0.f)  // samplePos is outside of the cell
      return false;
  }

  // Skip interpolation if values are defined per cell
  if (valid(self->cellValue)) {
    result = get_float(self->cellValue, id);
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
  result = u0 * v0 * w0 *
               get_float(self->vertexValue, getVertexId(self, cOffset + 0)) +
           u1 * v0 * w0 *
               get_float(self->vertexValue, getVertexId(self, cOffset + 1)) +
           u1 * v0 * w1 *
               get_float(self->vertexValue, getVertexId(self, cOffset + 2)) +
           u0 * v0 * w1 *
               get_float(self->vertexValue, getVertexId(self, cOffset + 3)) +
           u0 * v1 * w0 *
               get_float(self->vertexValue, getVertexId(self, cOffset + 4)) +
           u1 * v1 * w0 *
               get_float(self->vertexValue, getVertexId(self, cOffset + 5)) +
           u1 * v1 * w1 *
               get_float(self->vertexValue, getVertexId(self, cOffset + 6)) +
           u0 * v1 * w1 *
               get_float(self->vertexValue, getVertexId(self, cOffset + 7));
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
  sf[2] = pcoords[0] * pcoords[1] * tm;
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
  derivs[8]  = -rm * tm;
  derivs[9]  = -pcoords[0] * tm;
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

static const uniform float HEX_DIVERGED               = 1.e6;
static const uniform int HEX_MAX_ITERATION            = 10;
static const uniform float HEX_CONVERGED              = 1.e-04;
static const uniform float HEX_OUTSIDE_CELL_TOLERANCE = 1.e-06;

static bool intersectAndSampleHexIterative(const void *uniform userData,
                                           uniform uint64 id,
                                           uniform bool assumeInside,
                                           float &result,
                                           vec3f samplePos)
{
  const VKLUnstructuredVolume *uniform self =
      (const VKLUnstructuredVolume *uniform)userData;

  float pcoords[3] = {0.5, 0.5, 0.5};
  float derivs[24];
  float weights[8];

  // Get cell offset in index buffer
  const uniform uint64 cOffset             = getCellOffset(self, id);
  const uniform float determinantTolerance = self->iterativeTolerance[id];

  // Enter iteration loop
  bool converged = false;
  for (uniform int iteration = 0; !converged && (iteration < HEX_MAX_ITERATION);
       iteration++) {
    unmasked
    {
      // Calculate element interpolation functions and derivatives
      hexInterpolationFunctions(pcoords, weights);
      hexInterpolationDerivs(pcoords, derivs);

      // Calculate newton functions
      vec3f fcol = make_vec3f(0.f, 0.f, 0.f);
      vec3f rcol = make_vec3f(0.f, 0.f, 0.f);
      vec3f scol = make_vec3f(0.f, 0.f, 0.f);
      vec3f tcol = make_vec3f(0.f, 0.f, 0.f);
      for (uniform int i = 0; i < 8; i++) {
        const uniform vec3f pt =
            get_vec3f(self->vertex, getVertexId(self, cOffset + i));
        fcol = fcol + pt * weights[i];
        rcol = rcol + pt * derivs[i];
        scol = scol + pt * derivs[i + 8];
        tcol = tcol + pt * derivs[i + 16];
      }

      fcol = fcol - samplePos;

      // Compute determinants and generate improvements
      const float d = det(make_LinearSpace3f(rcol, scol, tcol));
    }

    if (absf(d) < determinantTolerance) {
      return false;
    }

    const float d0 = det(make_LinearSpace3f(fcol, scol, tcol)) / d;
    const float d1 = det(make_LinearSpace3f(rcol, fcol, tcol)) / d;
    const float d2 = det(make_LinearSpace3f(rcol, scol, fcol)) / d;

    pcoords[0] = pcoords[0] - d0;
    pcoords[1] = pcoords[1] - d1;
    pcoords[2] = pcoords[2] - d2;

    // Convergence/divergence test - if neither, repeat
    if ((absf(d0) < HEX_CONVERGED) & (absf(d1) < HEX_CONVERGED) &
        (absf(d2) < HEX_CONVERGED)) {
      converged = true;
    } else if ((absf(pcoords[0]) > HEX_DIVERGED) |
               (absf(pcoords[1]) > HEX_DIVERGED) |
               (absf(pcoords[2]) > HEX_DIVERGED)) {
      return false;
    }
  }

  if (!converged) {
    return false;
  }

  const uniform float lowerlimit = 0.0 - HEX_OUTSIDE_CELL_TOLERANCE;
  const uniform float upperlimit = 1.0 + HEX_OUTSIDE_CELL_TOLERANCE;
  if (assumeInside || (pcoords[0] >= lowerlimit && pcoords[0] <= upperlimit &&
                       pcoords[1] >= lowerlimit && pcoords[1] <= upperlimit &&
                       pcoords[2] >= lowerlimit && pcoords[2] <= upperlimit)) {
    // Evaluation
    if (valid(self->cellValue)) {
      result = get_float(self->cellValue, id);
    } else {
      float val = 0.f;
      for (uniform int i = 0; i < 8; i++) {
        val += weights[i] *
               get_float(self->vertexValue, getVertexId(self, cOffset + i));
      }
      result = val;
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
static inline void pyramidInterpolationDerivs(float pcoords[3],
                                              float derivs[15])
{
  // r-derivatives
  derivs[0] = -(pcoords[1] - 1.f) * (pcoords[2] - 1.f);
  derivs[1] = (pcoords[1] - 1.f) * (pcoords[2] - 1.f);
  derivs[2] = pcoords[1] - pcoords[1] * pcoords[2];
  derivs[3] = pcoords[1] * (pcoords[2] - 1.f);
  derivs[4] = 0.f;

  // s-derivatives
  derivs[5] = -(pcoords[0] - 1.f) * (pcoords[2] - 1.f);
  derivs[6] = pcoords[0] * (pcoords[2] - 1.f);
  derivs[7] = pcoords[0] - pcoords[0] * pcoords[2];
  derivs[8] = (pcoords[0] - 1.f) * (pcoords[2] - 1.f);
  derivs[9] = 0.f;

  // t-derivatives
  derivs[10] = -(pcoords[0] - 1.f) * (pcoords[1] - 1.f);
  derivs[11] = pcoords[0] * (pcoords[1] - 1.f);
  derivs[12] = -pcoords[0] * pcoords[1];
  derivs[13] = (pcoords[0] - 1.f) * pcoords[1];
  derivs[14] = 1.f;
}

static const uniform float PYRAMID_DIVERGED               = 1.e6;
static const uniform int PYRAMID_MAX_ITERATION            = 10;
static const uniform float PYRAMID_CONVERGED              = 1.e-04;
static const uniform float PYRAMID_OUTSIDE_CELL_TOLERANCE = 1.e-06;

static bool intersectAndSamplePyramid(const void *uniform userData,
                                      uniform uint64 id,
                                      uniform bool assumeInside,
                                      float &result,
                                      vec3f samplePos)
{
  const VKLUnstructuredVolume *uniform self =
      (const VKLUnstructuredVolume *uniform)userData;

  float pcoords[3] = {0.5, 0.5, 0.5};
  float derivs[15];
  float weights[5];

  // Get cell offset in index buffer
  const uniform uint64 cOffset             = getCellOffset(self, id);
  const uniform float determinantTolerance = self->iterativeTolerance[id];

  // Enter iteration loop
  bool converged = false;
  for (uniform int iteration = 0;
       !converged && (iteration < PYRAMID_MAX_ITERATION);
       iteration++) {
    unmasked
    {
      // Calculate element interpolation functions and derivatives
      pyramidInterpolationFunctions(pcoords, weights);
      pyramidInterpolationDerivs(pcoords, derivs);

      // Calculate newton functions
      vec3f fcol = make_vec3f(0.f, 0.f, 0.f);
      vec3f rcol = make_vec3f(0.f, 0.f, 0.f);
      vec3f scol = make_vec3f(0.f, 0.f, 0.f);
      vec3f tcol = make_vec3f(0.f, 0.f, 0.f);
      for (uniform int i = 0; i < 5; i++) {
        const uniform vec3f pt =
            get_vec3f(self->vertex, getVertexId(self, cOffset + i));
        fcol = fcol + pt * weights[i];
        rcol = rcol + pt * derivs[i];
        scol = scol + pt * derivs[i + 5];
        tcol = tcol + pt * derivs[i + 10];
      }

      fcol = fcol - samplePos;

      // Compute determinants and generate improvements
      const float d = det(make_LinearSpace3f(rcol, scol, tcol));
    }

    if (absf(d) < determinantTolerance) {
      return false;
    }

    const float d0 = det(make_LinearSpace3f(fcol, scol, tcol)) / d;
    const float d1 = det(make_LinearSpace3f(rcol, fcol, tcol)) / d;
    const float d2 = det(make_LinearSpace3f(rcol, scol, fcol)) / d;

    pcoords[0] = pcoords[0] - d0;
    pcoords[1] = pcoords[1] - d1;
    pcoords[2] = pcoords[2] - d2;

    // Convergence/divergence test - if neither, repeat
    if ((absf(d0) < PYRAMID_CONVERGED) & (absf(d1) < PYRAMID_CONVERGED) &
        (absf(d2) < PYRAMID_CONVERGED)) {
      converged = true;
    } else if ((absf(pcoords[0]) > PYRAMID_DIVERGED) |
               (absf(pcoords[1]) > PYRAMID_DIVERGED) |
               (absf(pcoords[2]) > PYRAMID_DIVERGED)) {
      return false;
    }
  }

  if (!converged) {
    return false;
  }

  const uniform float lowerlimit = 0.0 - PYRAMID_OUTSIDE_CELL_TOLERANCE;
  const uniform float upperlimit = 1.0 + PYRAMID_OUTSIDE_CELL_TOLERANCE;
  if (assumeInside || (pcoords[0] >= lowerlimit && pcoords[0] <= upperlimit &&
                       pcoords[1] >= lowerlimit && pcoords[1] <= upperlimit &&
                       pcoords[2] >= lowerlimit && pcoords[2] <= upperlimit)) {
    // Evaluation
    if (valid(self->cellValue)) {
      result = get_float(self->cellValue, id);
    } else {
      float val = 0.f;
      for (uniform int i = 0; i < 5; i++) {
        val += weights[i] *
               get_float(self->vertexValue, getVertexId(self, cOffset + i));
      }
      result = val;
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
  const VKLUnstructuredVolume *uniform self =
      (const VKLUnstructuredVolume *uniform)userData;

  switch (get_uint8(self->cellType, id)) {
  case VKL_TETRAHEDRON:
    hit = intersectAndSampleTet(userData, id, false, result, samplePos);
    break;
  case VKL_HEXAHEDRON:
    if (!self->hexIterative)
      hit = intersectAndSampleHexFast(userData, id, result, samplePos);
    else
      hit = intersectAndSampleHexIterative(
          userData, id, false, result, samplePos);
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
    const Sampler *uniform sampler, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  const VKLUnstructuredVolume *uniform self =
      (const VKLUnstructuredVolume *uniform)sampler->volume;

  float results = floatbits(0xffffffff); /* NaN */

  traverseEmbree(self->super.bvhRoot,
                 self,
                 intersectAndSampleCell,
                 results,
                 worldCoordinates);

  return results;
}

inline varying vec3f VKLUnstructuredVolume_computeGradient(
    const Sampler *uniform sampler, const varying vec3f &objectCoordinates)
{
  // Cast to the actual Volume subtype.
  const VKLUnstructuredVolume *uniform self =
      (const VKLUnstructuredVolume *uniform)sampler->volume;

  // gradient step in each dimension (object coordinates)
  vec3f gradientStep = self->gradientStep;

  // compute via forward or backward differences depending on volume / cell
  // boundaries (as determined by NaN sample values outside any volume cell)
  vec3f gradient;

  float sample = VKLUnstructuredVolume_sample(sampler, objectCoordinates);

  gradient.x =
      VKLUnstructuredVolume_sample(
          sampler, objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f)) -
      sample;
  gradient.y =
      VKLUnstructuredVolume_sample(
          sampler, objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f)) -
      sample;
  gradient.z =
      VKLUnstructuredVolume_sample(
          sampler, objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z)) -
      sample;

  if (isnan(gradient.x)) {
    gradientStep.x *= -1.f;

    gradient.x =
        VKLUnstructuredVolume_sample(
            sampler, objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f)) -
        sample;
  }

  if (isnan(gradient.y)) {
    gradientStep.y *= -1.f;

    gradient.y =
        VKLUnstructuredVolume_sample(
            sampler, objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f)) -
        sample;
  }

  if (isnan(gradient.z)) {
    gradientStep.z *= -1.f;

    gradient.z =
        VKLUnstructuredVolume_sample(
            sampler, objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z)) -
        sample;
  }

  return gradient / gradientStep;
}

export void EXPORT_UNIQUE(VKLUnstructuredVolume_sample_export,
                          uniform const int *uniform imask,
                          void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          void *uniform _samples)
{
  const Sampler *uniform sampler = (const Sampler *uniform) _sampler;
  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float *uniform samples = (varying float *uniform)_samples;

    *samples = VKLUnstructuredVolume_sample(sampler, *objectCoordinates);
  }
}

export void EXPORT_UNIQUE(VKLUnstructuredVolume_gradient_export,
                          uniform const int *uniform imask,
                          void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          void *uniform _gradients)
{
  const Sampler *uniform sampler = (const Sampler *uniform) _sampler;
  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying vec3f *uniform gradients = (varying vec3f * uniform) _gradients;

    *gradients = VKLUnstructuredVolume_computeGradient(sampler, *objectCoordinates);
  }
}

export void *uniform EXPORT_UNIQUE(VKLUnstructuredVolume_Constructor)
{
  uniform VKLUnstructuredVolume *uniform self =
      uniform new uniform VKLUnstructuredVolume;
  memset(self, 0, sizeof(uniform VKLUnstructuredVolume));

  return self;
}

export void EXPORT_UNIQUE(VKLUnstructuredVolume_Destructor, void *uniform _self)
{
  VKLUnstructuredVolume *uniform volume =
      (VKLUnstructuredVolume * uniform) _self;
  delete volume;
}

export void EXPORT_UNIQUE(VKLUnstructuredVolume_set,
                          void *uniform _self,
                          const uniform box3f &_bbox,
                          const Data1D *uniform _vertex,
                          const Data1D *uniform _index,
                          const uniform bool _index32Bit,
                          const Data1D *uniform _vertexValue,
                          const Data1D *uniform _cellValue,
                          const Data1D *uniform _cell,
                          const uniform bool _cell32Bit,
                          const uniform uint32 _cellSkipIds,
                          const Data1D *uniform _cellType,
                          const void *uniform bvhRoot,
                          const vec3f *uniform _faceNormals,
                          const float *uniform _iterativeTolerance,
                          const uniform bool _hexIterative)
{
  uniform VKLUnstructuredVolume *uniform self =
      (uniform VKLUnstructuredVolume * uniform) _self;

  self->vertex      = *_vertex;
  self->index       = *_index;
  self->index32Bit  = _index32Bit;
  self->vertexValue = *_vertexValue;
  self->cellValue   = *_cellValue;
  self->cell        = *_cell;
  self->cell32Bit   = _cell32Bit;
  self->cellSkipIds = _cellSkipIds;
  self->cellType    = *_cellType;

  self->faceNormals        = _faceNormals;
  self->iterativeTolerance = _iterativeTolerance;
  self->hexIterative       = _hexIterative;

  self->super.boundingBox = _bbox;

  self->gradientStep =
      make_vec3f(0.01f * reduce_min(self->super.boundingBox.upper -
                                    self->super.boundingBox.lower));

  self->super.bvhRoot          = (uniform Node * uniform) bvhRoot;
}

export UnstructuredSamplerBase *uniform
EXPORT_UNIQUE(VKLUnstructuredSampler_Constructor, void *uniform _volume)
{
  UnstructuredSamplerBase *uniform sampler =
      uniform new UnstructuredSamplerBase;
  memset(sampler, 0, sizeof(uniform UnstructuredSamplerBase));

  sampler->super.volume                = (const Volume *uniform)_volume;
  sampler->super.computeSample_varying = VKLUnstructuredVolume_sample;
  sampler->super.computeGradient_varying =
      VKLUnstructuredVolume_computeGradient;

  return sampler;
}

export void EXPORT_UNIQUE(VKLUnstructuredSampler_Destructor,
                          void *uniform _sampler)
{
  UnstructuredSamplerBase *uniform sampler =
      (UnstructuredSamplerBase * uniform) _sampler;
  delete sampler;
}

export void EXPORT_UNIQUE(VKLUnstructuredSampler_set,
                          void *uniform _self,
                          const uniform int maxIteratorDepth)
{
  UnstructuredSamplerBase *uniform self =
      (UnstructuredSamplerBase * uniform) _self;

  self->maxIteratorDepth = maxIteratorDepth;
}
