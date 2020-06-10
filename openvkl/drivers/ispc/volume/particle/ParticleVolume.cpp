// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ParticleVolume.h"
#include "../common/Data.h"
#include "ParticleSampler.h"
#include "rkcommon/containers/AlignedVector.h"
#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/utility/multidim_index_sequence.h"

#include <algorithm>

namespace openvkl {
  namespace ispc_driver {

    static inline void errorFunction(void *userPtr,
                                     enum RTCError error,
                                     const char *str)
    {
      LogMessageStream(VKL_LOG_WARNING)
          << "error " << error << ": " << str << std::endl;
    }

    template <int W>
    ParticleVolume<W>::~ParticleVolume()
    {
      if (this->ispcEquivalent) {
        CALL_ISPC(VKLParticleVolume_Destructor, this->ispcEquivalent);
      }

      if (rtcBVH)
        rtcReleaseBVH(rtcBVH);
      if (rtcDevice)
        rtcReleaseDevice(rtcDevice);
    }

    template <int W>
    void ParticleVolume<W>::commit()
    {
      Volume<W>::commit();

      positions = this->template getParamDataT<vec3f>("particle.position");
      radii     = this->template getParamDataT<float>("particle.radius");
      weights = this->template getParamDataT<float>("particle.weight", nullptr);

      // positions, radii, and weights (if provided) must all be of the same
      // length
      const size_t numParticles = positions->size();

      if (radii->size() != numParticles) {
        throw std::runtime_error(
            "particle.radius array must have same number of elements as "
            "particle.position");
      }

      if (weights && weights->size() != numParticles) {
        throw std::runtime_error(
            "particle.weight array must have same number of elements as "
            "particle.position");
      }

      // a multiple of each particle's base radius that we wish to use for
      // actual BVH bounds. Higher values result in smoother reconstruction
      // results, with lower performance.
      radiusSupportFactor =
          this->template getParam<float>("radiusSupportFactor", 3.f);

      if (radiusSupportFactor <= 0.f) {
        throw std::runtime_error("radiusSupportFactor must be positive");
      }

      // The maximum range value, set by user. All cumulative values will be
      // clamped to this, and further traversal (hence summation) of the
      // particle volume will halt when this value is reached. A value of zero
      // or less turns this off.
      clampMaxCumulativeValue =
          this->template getParam<float>("clampMaxCumulativeValue", 0.f);

      // Enable heuristic estimation of value ranges which are used in internal
      // acceleration structures for interval and hit iterators, as well as for
      // determining the volume's overall value range. When set to `false`, the
      // user *must* specify `clampMaxCumulativeValue`, and all value ranges
      // will be assumed [0, `clampMaxCumulativeValue`]. Disabling this may
      // improve volume commit time, but will make interval and hit iteration
      // less efficient.
      estimateValueRanges =
          this->template getParam<bool>("estimateValueRanges", true);

      if (estimateValueRanges == false && clampMaxCumulativeValue == 0) {
        throw std::runtime_error(
            "If estimateValueRanges is set to 'false', the user must specify a "
            "clampMaxCumulativeValue greater than zero.");
      }

      buildBvhAndCalculateBounds();

      if (!this->ispcEquivalent) {
        this->ispcEquivalent = CALL_ISPC(VKLParticleVolume_Constructor);
      }

      CALL_ISPC(VKLParticleVolume_set,
                this->ispcEquivalent,
                (const ispc::box3f &)bounds,
                ispc(positions),
                ispc(radii),
                ispc(weights),
                radiusSupportFactor,
                clampMaxCumulativeValue,
                (void *)(rtcRoot));

      computeValueRanges();
    }

    template <int W>
    Sampler<W> *ParticleVolume<W>::newSampler()
    {
      return new ParticleSampler<W>(this);
    }

    template <int W>
    void ParticleVolume<W>::buildBvhAndCalculateBounds()
    {
      rtcDevice = rtcNewDevice(NULL);
      if (!rtcDevice) {
        throw std::runtime_error("cannot create device");
      }
      rtcSetDeviceErrorFunction(rtcDevice, errorFunction, NULL);

      containers::AlignedVector<RTCBuildPrimitive> prims;
      containers::AlignedVector<float> primRadii;

      const size_t numParticles = positions->size();

      prims.resize(numParticles);
      primRadii.resize(numParticles);

      tasking::parallel_for(numParticles, [&](size_t taskIndex) {
        const vec3f &position = (*positions)[taskIndex];
        const float &radius   = (*radii)[taskIndex];

        float weight = weights ? (*weights)[taskIndex] : 1.f;

        const float supportRadius = radius * radiusSupportFactor;

        prims[taskIndex].lower_x = position.x - supportRadius;
        prims[taskIndex].lower_y = position.y - supportRadius;
        prims[taskIndex].lower_z = position.z - supportRadius;
        prims[taskIndex].geomID  = taskIndex >> 32;
        prims[taskIndex].upper_x = position.x + supportRadius;
        prims[taskIndex].upper_y = position.y + supportRadius;
        prims[taskIndex].upper_z = position.z + supportRadius;
        prims[taskIndex].primID  = taskIndex & 0xffffffff;

        primRadii[taskIndex] = radius;
      });

      rtcBVH = rtcNewBVH(rtcDevice);
      if (!rtcBVH) {
        throw std::runtime_error("bvh creation failure");
      }

      RTCBuildArguments arguments      = rtcDefaultBuildArguments();
      arguments.byteSize               = sizeof(arguments);
      arguments.buildFlags             = RTC_BUILD_FLAG_NONE;
      arguments.buildQuality           = RTC_BUILD_QUALITY_MEDIUM;
      arguments.maxBranchingFactor     = 2;
      arguments.maxDepth               = 1024;
      arguments.sahBlockSize           = 1;
      arguments.minLeafSize            = 1;
      arguments.maxLeafSize            = 1;  // our leaf nodes limited to 1 prim
      arguments.traversalCost          = 1.0f;
      arguments.intersectionCost       = 2.0f;
      arguments.bvh                    = rtcBVH;
      arguments.primitives             = prims.data();
      arguments.primitiveCount         = prims.size();
      arguments.primitiveArrayCapacity = prims.size();
      arguments.createNode             = InnerNode::create;
      arguments.setNodeChildren        = InnerNode::setChildren;
      arguments.setNodeBounds          = InnerNode::setBounds;
      arguments.createLeaf             = ParticleLeafNode::create;
      arguments.splitPrimitive         = nullptr;
      arguments.buildProgress          = nullptr;
      arguments.userPtr                = primRadii.data();

      rtcRoot = (Node *)rtcBuildBVH(&arguments);
      if (!rtcRoot) {
        throw std::runtime_error("bvh build failure");
      }

      if (rtcRoot->nominalLength < 0) {
        auto &val = ((LeafNode *)rtcRoot)->bounds;
        bounds    = box3f(val.lower, val.upper);
      } else {
        auto &vals = ((InnerNode *)rtcRoot)->bounds;
        bounds     = box3f(vals[0].lower, vals[0].upper);
        bounds.extend(box3f(vals[1].lower, vals[1].upper));
      }
    }

    template <int W>
    void ParticleVolume<W>::computeValueRanges()
    {
      // this method computes an _estimate_ of value range; this fractional
      // uncertainty will be used to conservatively expand the computed ranges
      // This value also shows up in tests/particle_volume_interval_iterator.cpp
      const float uncertainty = 0.05f;

      // build vector of leaf nodes
      const size_t numParticles = positions->size();
      std::vector<LeafNode *> leafNodes;
      leafNodes.reserve(numParticles);

      populateLeafNodes(rtcRoot, leafNodes);

      if (leafNodes.size() != numParticles) {
        throw std::runtime_error("incorrect number of leaf nodes found");
      }

      // compute value ranges of leaf nodes in parallel
      std::unique_ptr<Sampler<W>> sampler(newSampler());

      if (estimateValueRanges) {
        // restrict to first attribute index
        const unsigned int attributeIndex = 0;

        tasking::parallel_for(leafNodes.size(), [&](size_t leafNodeIndex) {
          LeafNode *leafNode         = leafNodes[leafNodeIndex];
          const size_t particleIndex = leafNode->cellID;

          range1f computedValueRange(empty);

          // estimates use sampling interfaces directly, to ensure all
          // constraints are consistently considered (e.g.
          // clampMaxCumulativeValue, radiusSupportFactor)

          // initial estimate based sampling particle center
          vfloatn<1> sample;
          sampler->computeSample(
              (*positions)[particleIndex], sample, attributeIndex);
          computedValueRange.extend(sample[0]);

          // sample over regular grid within leaf bounds to improve estimate
          const box3fa leafBounds = leafNode->bounds;

          const int samplesPerDimension = 10;

          std::vector<vvec3fn<1>> objectCoordinates;
          objectCoordinates.reserve(samplesPerDimension * samplesPerDimension *
                                    samplesPerDimension);

          multidim_index_sequence<3> mis{vec3i(samplesPerDimension)};

          for (const auto &ijk : mis) {
            objectCoordinates.push_back(
                leafBounds.lower + vec3f(ijk) / float(samplesPerDimension - 1) *
                                       leafBounds.size());
          }

          std::vector<float> samples(objectCoordinates.size());
          sampler->computeSampleN(objectCoordinates.size(),
                                  objectCoordinates.data(),
                                  samples.data(),
                                  attributeIndex);

          auto minmax = std::minmax_element(samples.begin(), samples.end());
          computedValueRange.extend(range1f(*minmax.first, *minmax.second));

          // apply uncertainty to computed value range
          computedValueRange.lower *= (1.f - uncertainty);
          computedValueRange.upper *= (1.f + uncertainty);

          leafNode->valueRange = computedValueRange;
        });
      } else {
        tasking::parallel_for(leafNodes.size(), [&](size_t leafNodeIndex) {
          LeafNode *leafNode   = leafNodes[leafNodeIndex];
          leafNode->valueRange = range1f(0.f, clampMaxCumulativeValue);
        });
      }

      // accumulate ranges in root and inner nodes
      accumulateNodeValueRanges(rtcRoot);

      valueRange = rtcRoot->valueRange;
    }

    VKL_REGISTER_VOLUME(ParticleVolume<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_particle_, VKL_TARGET_WIDTH))

  }  // namespace ispc_driver
}  // namespace openvkl
