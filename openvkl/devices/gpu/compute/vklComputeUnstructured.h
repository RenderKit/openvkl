// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../cpu/volume/UnstructuredVolumeShared.h"

namespace ispc {

  ////////////////////////////////////////////////////////////////////////////
  // utility functions ///////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  inline bool pointInAABBTest(const box3fa &box, const vec3f &point)
  {
    bool t1 = point.x >= box.lower.x;
    bool t2 = point.y >= box.lower.y;
    bool t3 = point.z >= box.lower.z;
    bool t4 = point.x <= box.upper.x;
    bool t5 = point.y <= box.upper.y;
    bool t6 = point.z <= box.upper.z;
    return t1 & t2 & t3 & t4 & t5 & t6;
  }

  // Read 32/64-bit integer value from given array
  inline uint64_t readInteger(const Data1D array,
                              const bool is32Bit,
                              const uint64_t id)
  {
    return is32Bit ? get_uint32(array, id) : get_uint64(array, id);
  }

  // Get cell offset (location) in index array
  inline uint64_t getCellOffset(const VKLUnstructuredVolume *self,
                                const uint64_t id)
  {
    return readInteger(self->cell, self->cell32Bit, id) + self->cellSkipIds;
  }

  // Get vertex index from index array
  inline uint64_t getVertexId(const VKLUnstructuredVolume *self,
                              const uint64_t id)
  {
    return readInteger(self->index, self->index32Bit, id);
  }

  ////////////////////////////////////////////////////////////////////////////
  // normal computations /////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  inline vec3f calcPlaneNormal(const VKLUnstructuredVolume *self,
                               const uint64_t id,
                               const uint32_t plane[3])
  {
    // Retrieve cell offset first
    const uint64_t cOffset = getCellOffset(self, id);

    // Get 3 vertices for normal calculation
    const vec3f v0 =
        get_vec3f(self->vertex, getVertexId(self, cOffset + plane[0]));
    const vec3f v1 =
        get_vec3f(self->vertex, getVertexId(self, cOffset + plane[1]));
    const vec3f v2 =
        get_vec3f(self->vertex, getVertexId(self, cOffset + plane[2]));

    // Calculate normal
    return normalize(cross(v0 - v1, v2 - v1));
  }

  inline vec3f tetrahedronNormal(const VKLUnstructuredVolume *self,
                                 const uint64_t id,
                                 const int planeID)
  {
    // Get precomputed normal if available
    if (self->faceNormals)
      return self->faceNormals[(id * 6) + planeID];

    // Prepare vertex offset bys plane
    const uint32_t planes[4][3] = {{2, 0, 1}, {3, 1, 0}, {3, 2, 1}, {2, 3, 0}};
    return calcPlaneNormal(self, id, planes[planeID]);
  }

  inline vec3f pyramidNormal(const VKLUnstructuredVolume *self,
                             const uint64_t id,
                             const int planeID)
  {
    // Get precomputed normal if available
    if (self->faceNormals)
      return self->faceNormals[(id * 6) + planeID];

    // Prepare vertex offsets by plane
    const uint32_t planes[5][3] = {
        {3, 0, 1}, {4, 1, 0}, {4, 2, 1}, {4, 3, 2}, {3, 4, 0}};
    return calcPlaneNormal(self, id, planes[planeID]);
  }

  inline vec3f hexahedronNormal(const VKLUnstructuredVolume *self,
                                const uint64_t id,
                                const int planeID)
  {
    // Get precomputed normal if available
    if (self->faceNormals)
      return self->faceNormals[(id * 6) + planeID];

    // Prepare vertex offsets by plane
    const uint32_t planes[6][3] = {
        {3, 0, 1}, {5, 1, 0}, {6, 2, 1}, {7, 3, 2}, {7, 4, 0}, {6, 5, 4}};
    return calcPlaneNormal(self, id, planes[planeID]);
  }

  ////////////////////////////////////////////////////////////////////////////
  // intersect / sampling functions //////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  inline bool intersectAndSampleTet(const void *userData,
                                    uint64_t id,
                                    bool assumeInside,
                                    float &result,
                                    vec3f samplePos)
  {
    const VKLUnstructuredVolume *self = (const VKLUnstructuredVolume *)userData;

    // Get cell offset in index buffer
    const uint64_t cOffset = getCellOffset(self, id);

    const vec3f p0 = get_vec3f(self->vertex, getVertexId(self, cOffset + 0));
    const vec3f p1 = get_vec3f(self->vertex, getVertexId(self, cOffset + 1));
    const vec3f p2 = get_vec3f(self->vertex, getVertexId(self, cOffset + 2));
    const vec3f p3 = get_vec3f(self->vertex, getVertexId(self, cOffset + 3));

    const vec3f norm0 = tetrahedronNormal(self, id, 0);
    const vec3f norm1 = tetrahedronNormal(self, id, 1);
    const vec3f norm2 = tetrahedronNormal(self, id, 2);
    const vec3f norm3 = tetrahedronNormal(self, id, 3);

    // Distance from the world point to the faces.
    const float d0 = dot(norm0, p0 - samplePos);
    const float d1 = dot(norm1, p1 - samplePos);
    const float d2 = dot(norm2, p2 - samplePos);
    const float d3 = dot(norm3, p3 - samplePos);

    // Exit if samplePos is outside the cell
    if (!assumeInside && !(d0 > 0.0f && d1 > 0.0f && d2 > 0.0f && d3 > 0.0f))
      return false;

    // Skip interpolation if values are defined per cell
    if (isValid(self->cellValue)) {
      result = get_float(self->cellValue, id);
      return true;
    }

    // Distance of tetrahedron corners to their opposite faces.
    const float h0 = dot(norm0, p0 - p3);
    const float h1 = dot(norm1, p1 - p2);
    const float h2 = dot(norm2, p2 - p0);
    const float h3 = dot(norm3, p3 - p1);

    // Local coordinates = ratio of distances.
    const float z0 = d0 / h0;
    const float z1 = d1 / h1;
    const float z2 = d2 / h2;
    const float z3 = d3 / h3;

    // Field/attribute values at the tetrahedron corners.
    const float v0 =
        get_float(self->vertexValue, getVertexId(self, cOffset + 0));
    const float v1 =
        get_float(self->vertexValue, getVertexId(self, cOffset + 1));
    const float v2 =
        get_float(self->vertexValue, getVertexId(self, cOffset + 2));
    const float v3 =
        get_float(self->vertexValue, getVertexId(self, cOffset + 3));

    // Interpolated field/attribute value at the world position.
    result = z0 * v3 + z1 * v2 + z2 * v0 + z3 * v1;
    return true;
  }

  inline void wedgeInterpolationFunctions(float pcoords[3], float sf[6])
  {
    sf[0] = (1.0f - pcoords[0] - pcoords[1]) * (1.0f - pcoords[2]);
    sf[1] = pcoords[0] * (1.0f - pcoords[2]);
    sf[2] = pcoords[1] * (1.0f - pcoords[2]);
    sf[3] = (1.0f - pcoords[0] - pcoords[1]) * pcoords[2];
    sf[4] = pcoords[0] * pcoords[2];
    sf[5] = pcoords[1] * pcoords[2];
  }

  inline void wedgeInterpolationDerivs(float pcoords[3], float derivs[18])
  {
    // r-derivatives
    derivs[0] = -1.0f + pcoords[2];
    derivs[1] = 1.0f - pcoords[2];
    derivs[2] = 0.0f;
    derivs[3] = -pcoords[2];
    derivs[4] = pcoords[2];
    derivs[5] = 0.0f;

    // s-derivatives
    derivs[6]  = -1.0f + pcoords[2];
    derivs[7]  = 0.0f;
    derivs[8]  = 1.0f - pcoords[2];
    derivs[9]  = -pcoords[2];
    derivs[10] = 0.0f;
    derivs[11] = pcoords[2];

    // t-derivatives
    derivs[12] = -1.0f + pcoords[0] + pcoords[1];
    derivs[13] = -pcoords[0];
    derivs[14] = -pcoords[1];
    derivs[15] = 1.0f - pcoords[0] - pcoords[1];
    derivs[16] = pcoords[0];
    derivs[17] = pcoords[1];
  }

  static const float WEDGE_DIVERGED               = 100000.f;
  static const int WEDGE_MAX_ITERATION            = 10;
  static const float WEDGE_CONVERGED              = 0.0001f;
  static const float WEDGE_OUTSIDE_CELL_TOLERANCE = 0.000001f;

  inline bool intersectAndSampleWedge(const void *userData,
                                      uint64_t id,
                                      bool assumeInside,
                                      float &result,
                                      vec3f samplePos)
  {
    const VKLUnstructuredVolume *self = (const VKLUnstructuredVolume *)userData;

    float pcoords[3] = {0.5f, 0.5f, 0.5f};
    float derivs[18];
    float weights[6];

    // Get cell offset in index buffer
    const uint64_t cOffset           = getCellOffset(self, id);
    const float determinantTolerance = self->iterativeTolerance[id];

    // Enter iteration loop
    bool converged = false;

    for (int iteration = 0; !converged && (iteration < WEDGE_MAX_ITERATION);
         iteration++) {
      //  Calculate element interpolation functions and derivatives
      wedgeInterpolationFunctions(pcoords, weights);
      wedgeInterpolationDerivs(pcoords, derivs);

      // Calculate newton functions
      vec3f fcol = vec3f(0.f, 0.f, 0.f);
      vec3f rcol = vec3f(0.f, 0.f, 0.f);
      vec3f scol = vec3f(0.f, 0.f, 0.f);
      vec3f tcol = vec3f(0.f, 0.f, 0.f);

      for (int i = 0; i < 6; i++) {
        const vec3f pt =
            get_vec3f(self->vertex, getVertexId(self, cOffset + i));
        fcol = fcol + pt * weights[i];
        rcol = rcol + pt * derivs[i];
        scol = scol + pt * derivs[i + 6];
        tcol = tcol + pt * derivs[i + 12];
      }

      fcol = fcol - samplePos;

      // Compute determinants and generate improvements
      const float d = det(LinearSpace3f(rcol, scol, tcol));

      if (absf(d) < determinantTolerance) {
        return false;
      }

      const float d0 = det(LinearSpace3f(fcol, scol, tcol)) / d;
      const float d1 = det(LinearSpace3f(rcol, fcol, tcol)) / d;
      const float d2 = det(LinearSpace3f(rcol, scol, fcol)) / d;

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

    const float lowerlimit = 0.0f - WEDGE_OUTSIDE_CELL_TOLERANCE;
    const float upperlimit = 1.0f + WEDGE_OUTSIDE_CELL_TOLERANCE;
    if (assumeInside || (pcoords[0] >= lowerlimit && pcoords[0] <= upperlimit &&
                         pcoords[1] >= lowerlimit && pcoords[1] <= upperlimit &&
                         pcoords[2] >= lowerlimit && pcoords[2] <= upperlimit &&
                         pcoords[0] + pcoords[1] <= upperlimit)) {
      // Evaluation
      if (isValid(self->cellValue)) {
        result = get_float(self->cellValue, id);
      } else {
        float val = 0.f;
        for (int i = 0; i < 6; i++) {
          val += weights[i] *
                 get_float(self->vertexValue, getVertexId(self, cOffset + i));
        }
        result = val;
      }

      return true;
    }

    return false;
  }

  inline void pyramidInterpolationFunctions(float pcoords[3], float sf[5])
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

  inline void pyramidInterpolationDerivs(float pcoords[3], float derivs[15])
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

  static const float PYRAMID_DIVERGED               = 1000000.f;
  static const int PYRAMID_MAX_ITERATION            = 10;
  static const float PYRAMID_CONVERGED              = 0.0001f;
  static const float PYRAMID_OUTSIDE_CELL_TOLERANCE = 0.000001f;

  inline bool intersectAndSamplePyramid(const void *userData,
                                        uint64_t id,
                                        bool assumeInside,
                                        float &result,
                                        vec3f samplePos)
  {
    const VKLUnstructuredVolume *self = (const VKLUnstructuredVolume *)userData;

    float pcoords[3] = {0.5f, 0.5f, 0.5f};
    float derivs[15];
    float weights[5];

    // Get cell offset in index buffer
    const uint64_t cOffset           = getCellOffset(self, id);
    const float determinantTolerance = self->iterativeTolerance[id];

    // Enter iteration loop
    bool converged = false;
    for (int iteration = 0; !converged && (iteration < PYRAMID_MAX_ITERATION);
         iteration++) {
      // Calculate element interpolation functions and derivatives
      pyramidInterpolationFunctions(pcoords, weights);
      pyramidInterpolationDerivs(pcoords, derivs);

      // Calculate newton functions
      vec3f fcol = vec3f(0.f, 0.f, 0.f);
      vec3f rcol = vec3f(0.f, 0.f, 0.f);
      vec3f scol = vec3f(0.f, 0.f, 0.f);
      vec3f tcol = vec3f(0.f, 0.f, 0.f);

      for (int i = 0; i < 5; i++) {
        const vec3f pt =
            get_vec3f(self->vertex, getVertexId(self, cOffset + i));
        fcol = fcol + pt * weights[i];
        rcol = rcol + pt * derivs[i];
        scol = scol + pt * derivs[i + 5];
        tcol = tcol + pt * derivs[i + 10];
      }

      fcol = fcol - samplePos;

      // Compute determinants and generate improvements
      const float d = det(LinearSpace3f(rcol, scol, tcol));

      if (absf(d) < determinantTolerance) {
        return false;
      }

      const float d0 = det(LinearSpace3f(fcol, scol, tcol)) / d;
      const float d1 = det(LinearSpace3f(rcol, fcol, tcol)) / d;
      const float d2 = det(LinearSpace3f(rcol, scol, fcol)) / d;

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

    const float lowerlimit = 0.0f - PYRAMID_OUTSIDE_CELL_TOLERANCE;
    const float upperlimit = 1.0f + PYRAMID_OUTSIDE_CELL_TOLERANCE;
    if (assumeInside ||
        (pcoords[0] >= lowerlimit && pcoords[0] <= upperlimit &&
         pcoords[1] >= lowerlimit && pcoords[1] <= upperlimit &&
         pcoords[2] >= lowerlimit && pcoords[2] <= upperlimit)) {
      // Evaluation
      if (isValid(self->cellValue)) {
        result = get_float(self->cellValue, id);
      } else {
        float val = 0.f;
        for (int i = 0; i < 5; i++) {
          val += weights[i] *
                 get_float(self->vertexValue, getVertexId(self, cOffset + i));
        }
        result = val;
      }

      return true;
    }

    return false;
  }

  inline bool intersectAndSampleHexFast(const void *userData,
                                        uint64_t id,
                                        float &result,
                                        vec3f samplePos)
  {
    const VKLUnstructuredVolume *self = (const VKLUnstructuredVolume *)userData;

    // Get cell offset in index buffer
    const uint64_t cOffset = getCellOffset(self, id);

    // Calculate distances from each hexahedron face
    float dist[6];
    for (int plane = 0; plane < 6; plane++) {
      const vec3f v =
          get_vec3f(self->vertex, getVertexId(self, cOffset + plane));
      dist[plane] = dot(samplePos - v, hexahedronNormal(self, id, plane));
      if (dist[plane] > 0.f)  // samplePos is outside of the cell
        return false;
    }

    // Skip interpolation if values are defined per cell
    if (isValid(self->cellValue)) {
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

  inline void hexInterpolationDerivs(float pcoords[3], float derivs[24])
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

  inline void hexInterpolationFunctions(float pcoords[3], float sf[8])
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

  static const float HEX_DIVERGED               = 1000000.f;
  static const int HEX_MAX_ITERATION            = 10;
  static const float HEX_CONVERGED              = 0.0001f;
  static const float HEX_OUTSIDE_CELL_TOLERANCE = 0.000001f;

  inline bool intersectAndSampleHexIterative(const void *userData,
                                             uint64_t id,
                                             bool assumeInside,
                                             float &result,
                                             vec3f samplePos)
  {
    const VKLUnstructuredVolume *self = (const VKLUnstructuredVolume *)userData;

    float pcoords[3] = {0.5f, 0.5f, 0.5f};
    float derivs[24];
    float weights[8];

    // Get cell offset in index buffer
    const uint64_t cOffset           = getCellOffset(self, id);
    const float determinantTolerance = self->iterativeTolerance[id];

    // Enter iteration loop
    bool converged = false;

    for (int iteration = 0; !converged && (iteration < HEX_MAX_ITERATION);
         iteration++) {
      //  Calculate element interpolation functions and derivatives
      hexInterpolationFunctions(pcoords, weights);
      hexInterpolationDerivs(pcoords, derivs);

      // Calculate newton functions
      vec3f fcol = vec3f(0.f, 0.f, 0.f);
      vec3f rcol = vec3f(0.f, 0.f, 0.f);
      vec3f scol = vec3f(0.f, 0.f, 0.f);
      vec3f tcol = vec3f(0.f, 0.f, 0.f);
      for (int i = 0; i < 8; i++) {
        const vec3f pt =
            get_vec3f(self->vertex, getVertexId(self, cOffset + i));
        fcol = fcol + pt * weights[i];
        rcol = rcol + pt * derivs[i];
        scol = scol + pt * derivs[i + 8];
        tcol = tcol + pt * derivs[i + 16];
      }

      fcol = fcol - samplePos;

      // Compute determinants and generate improvements
      const float d = det(LinearSpace3f(rcol, scol, tcol));

      if (absf(d) < determinantTolerance) {
        return false;
      }

      const float d0 = det(LinearSpace3f(fcol, scol, tcol)) / d;
      const float d1 = det(LinearSpace3f(rcol, fcol, tcol)) / d;
      const float d2 = det(LinearSpace3f(rcol, scol, fcol)) / d;

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

    const float lowerlimit = 0.0f - HEX_OUTSIDE_CELL_TOLERANCE;
    const float upperlimit = 1.0f + HEX_OUTSIDE_CELL_TOLERANCE;
    if (assumeInside ||
        (pcoords[0] >= lowerlimit && pcoords[0] <= upperlimit &&
         pcoords[1] >= lowerlimit && pcoords[1] <= upperlimit &&
         pcoords[2] >= lowerlimit && pcoords[2] <= upperlimit)) {
      // Evaluation
      if (isValid(self->cellValue)) {
        result = get_float(self->cellValue, id);
      } else {
        float val = 0.f;
        for (int i = 0; i < 8; i++) {
          val += weights[i] *
                 get_float(self->vertexValue, getVertexId(self, cOffset + i));
        }
        result = val;
      }

      return true;
    }

    return false;
  }

  inline bool intersectAndSampleCell(const void *userData,
                                     uint64_t id,
                                     float &result,
                                     vec3f samplePos,
                                     const VKLFeatureFlags featureFlags)
  {
    bool hit = false;

    const VKLUnstructuredVolume *self = (const VKLUnstructuredVolume *)userData;

    const uint8_t cellType = get_uint8(self->cellType, id);

    if (cellType == VKL_TETRAHEDRON &&
        featureFlags & VKL_FEATURE_FLAG_HAS_CELL_TYPE_TETRAHEDRON) {
      hit = intersectAndSampleTet(userData, id, false, result, samplePos);

    } else if (cellType == VKL_HEXAHEDRON &&
               featureFlags & VKL_FEATURE_FLAG_HAS_CELL_TYPE_HEXAHEDRON) {
      if (!self->hexIterative) {
        hit = intersectAndSampleHexFast(userData, id, result, samplePos);
      } else {
        hit = intersectAndSampleHexIterative(
            userData, id, false, result, samplePos);
      }

    } else if (cellType == VKL_WEDGE &&
               featureFlags & VKL_FEATURE_FLAG_HAS_CELL_TYPE_WEDGE) {
      hit = intersectAndSampleWedge(userData, id, false, result, samplePos);

    } else if (cellType == VKL_PYRAMID &&
               featureFlags & VKL_FEATURE_FLAG_HAS_CELL_TYPE_PYRAMID) {
      hit = intersectAndSamplePyramid(userData, id, false, result, samplePos);

    } else {
      assert(false);
    }

    // Return true if samplePos is inside the cell
    return hit;
  }

  ////////////////////////////////////////////////////////////////////////////
  // traversal ///////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  inline void traverseBVHSingle(Node *const root,
                                const void *userPtr,
                                float &result,
                                const vec3f &samplePos,
                                const VKLFeatureFlags featureFlags)
  {
    Node *node = root;
    Node *nodeStack[32];
    int stackPtr = 0;
    while (1) {
      bool isLeaf = (node->nominalLength.x < 0.0f);
      if (isLeaf) {
        LeafNodeSingle *leaf = (LeafNodeSingle *)node;
        if (pointInAABBTest(leaf->super.bounds, samplePos)) {
          if (intersectAndSampleCell(
                  userPtr, leaf->cellID, result, samplePos, featureFlags)) {
            return;
          }
        }
      } else {
        InnerNode *inner = (InnerNode *)node;

        const bool in0 = pointInAABBTest(inner->bounds[0], samplePos);
        const bool in1 = pointInAABBTest(inner->bounds[1], samplePos);

        if (in0) {
          if (in1) {
            nodeStack[stackPtr++] = inner->children[1];
            node                  = inner->children[0];
            continue;
          } else {
            node = inner->children[0];
            continue;
          }
        } else {
          if (in1) {
            node = inner->children[1];
            continue;
          } else {
            /* Do nothing, just pop. */
          }
        }
      }
      if (stackPtr == 0)
        return;
      node = nodeStack[--stackPtr];
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  // API entrypoints /////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  inline float UnstructuredVolume_sample(const SamplerShared *sampler,
                                         const vec3f &objectCoordinates,
                                         const float &_time,
                                         const uint32_t &_attributeIndex,
                                         const VKLFeatureFlags featureFlags)
  {
    const VKLUnstructuredVolume *self =
        (const VKLUnstructuredVolume *)sampler->volume;

    float results = self->super.super.background[0];

    traverseBVHSingle(
        self->super.bvhRoot, self, results, objectCoordinates, featureFlags);

    return results;
  }

  inline void UnstructuredVolume_sampleM(const SamplerShared *sampler,
                                         const vec3f &objectCoordinates,
                                         const uint32_t M,
                                         const uint32_t *attributeIndices,
                                         float *samples,
                                         const VKLFeatureFlags featureFlags)
  {
    const VKLUnstructuredVolume *self =
        (const VKLUnstructuredVolume *)sampler->volume;

    for (uint32_t i = 0; i < M; i++) {
      // we still support this API, but unstructured volumes only support a
      // single attribute
      assert(attributeIndices[i] == 0);

      samples[i] = UnstructuredVolume_sample(
          sampler, objectCoordinates, 0.f, 0, featureFlags);
    }
  }

  inline vkl_vec3f UnstructuredVolume_computeGradient(
      const SamplerShared *sampler,
      const vec3f &objectCoordinates,
      const VKLFeatureFlags featureFlags)
  {
    // Cast to the actual Volume subtype.
    const VKLUnstructuredVolume *self =
        (const VKLUnstructuredVolume *)sampler->volume;

    // gradient step in each dimension (object coordinates)
    vec3f gradientStep = self->gradientStep;

    // compute via forward or backward differences depending on volume / cell
    // boundaries (as determined by NaN sample values outside any volume cell)
    vec3f gradient;

    float sample = UnstructuredVolume_sample(
        sampler, objectCoordinates, 0.f, 0, featureFlags);

    gradient.x = UnstructuredVolume_sample(
                     sampler,
                     objectCoordinates + vec3f(gradientStep.x, 0.f, 0.f),
                     0.f,
                     0,
                     featureFlags) -
                 sample;
    gradient.y = UnstructuredVolume_sample(
                     sampler,
                     objectCoordinates + vec3f(0.f, gradientStep.y, 0.f),
                     0.f,
                     0,
                     featureFlags) -
                 sample;
    gradient.z = UnstructuredVolume_sample(
                     sampler,
                     objectCoordinates + vec3f(0.f, 0.f, gradientStep.z),
                     0.f,
                     0,
                     featureFlags) -
                 sample;

    if (isnan(gradient.x)) {
      gradientStep.x *= -1.f;

      gradient.x = UnstructuredVolume_sample(
                       sampler,
                       objectCoordinates + vec3f(gradientStep.x, 0.f, 0.f),
                       0.f,
                       0,
                       featureFlags) -
                   sample;
    }

    if (isnan(gradient.y)) {
      gradientStep.y *= -1.f;

      gradient.y = UnstructuredVolume_sample(
                       sampler,
                       objectCoordinates + vec3f(0.f, gradientStep.y, 0.f),
                       0.f,
                       0,
                       featureFlags) -
                   sample;
    }

    if (isnan(gradient.z)) {
      gradientStep.z *= -1.f;

      gradient.z = UnstructuredVolume_sample(
                       sampler,
                       objectCoordinates + vec3f(0.f, 0.f, gradientStep.z),
                       0.f,
                       0,
                       featureFlags) -
                   sample;
    }

    return vkl_vec3f{gradient.x / gradientStep.x,
                     gradient.y / gradientStep.y,
                     gradient.z / gradientStep.z};
  }

}  // namespace ispc
