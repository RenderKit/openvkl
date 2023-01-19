// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ParticleVolume.h"
#include "../common/Data.h"
#include "ParticleSampler.h"
#include "rkcommon/containers/AlignedVector.h"
#include "rkcommon/tasking/parallel_for.h"

#include <algorithm>

namespace openvkl {
  namespace cpu_device {

    static inline void errorFunction(void *userPtr,
                                     enum RTCError error,
                                     const char *str)
    {
      Device *device = reinterpret_cast<Device *>(userPtr);
      LogMessageStream(device, VKL_LOG_WARNING)
          << "error " << error << ": " << str << std::endl;
    }

    template <int W>
    static range1f computeValueRangeOverBoundingBox(
        std::shared_ptr<Sampler<W>> sampler,
        unsigned int attributeIndex,
        const box3fa &bounds,
        int samplesPerDimension)
    {
      const std::vector<float> times(
          samplesPerDimension * samplesPerDimension * samplesPerDimension, 0.f);

      std::vector<vvec3fn<1>> objectCoordinates;
      objectCoordinates.reserve(samplesPerDimension * samplesPerDimension *
                                samplesPerDimension);

      for (int i = 0; i < samplesPerDimension; i++) {
        for (int j = 0; j < samplesPerDimension; j++) {
          for (int k = 0; k < samplesPerDimension; k++) {
            objectCoordinates.push_back(
                bounds.lower + vec3f(i, j, k) / float(samplesPerDimension - 1) *
                                   bounds.size());
          }
        }
      }

      std::vector<float> samples(objectCoordinates.size());
      sampler->computeSampleN(objectCoordinates.size(),
                              objectCoordinates.data(),
                              samples.data(),
                              attributeIndex,
                              times.data());

      auto minmax = std::minmax_element(samples.begin(), samples.end());
      return range1f(*minmax.first, *minmax.second);
    }

    template <int W>
    ParticleVolume<W>::~ParticleVolume()
    {
      if (this->SharedStructInitialized) {
        CALL_ISPC(VKLParticleVolume_Destructor, this->getSh());
        this->SharedStructInitialized = false;
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

      if (numParticles == 0) {
        throw std::runtime_error("no particles provided");
      }

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

      background = this->template getParamDataT<float>(
          "background", 1, VKL_BACKGROUND_UNDEFINED);

      buildBvhAndCalculateBounds();

      if (!this->SharedStructInitialized) {
        ispc::VKLParticleVolume *self =
            static_cast<ispc::VKLParticleVolume *>(this->getSh());

        memset(self, 0, sizeof(ispc::VKLParticleVolume));

        CALL_ISPC(VKLParticleVolume_Constructor, self);

        self->super.super.type = ispc::DeviceVolumeType::VOLUME_TYPE_PARTICLE;

        this->SharedStructInitialized = true;
      }

      this->setBackground(background->data());

      CALL_ISPC(VKLParticleVolume_set,
                this->getSh(),
                (const ispc::box3f &)bounds,
                ispc(positions),
                ispc(radii),
                ispc(weights),
                radiusSupportFactor,
                clampMaxCumulativeValue,
                (void *)(rtcRoot));

      computeValueRanges();

      computeOverlappingNodeMetadata(rtcRoot);
    }

    template <int W>
    Sampler<W> *ParticleVolume<W>::newSampler()
    {
      return new ParticleSampler<W>(this->getDevice(), *this);
    }

    template <int W>
    void ParticleVolume<W>::buildBvhAndCalculateBounds()
    {
      rtcDevice = rtcNewDevice(NULL);
      if (!rtcDevice) {
        throw std::runtime_error("cannot create device");
      }
      rtcSetDeviceErrorFunction(rtcDevice, errorFunction, this->device.ptr);

      AlignedVector<RTCBuildPrimitive> prims;
      AlignedVector<float> primRadii;

      const size_t numParticles = positions->size();

      prims.resize(numParticles);
      primRadii.resize(numParticles);

      tasking::parallel_for(numParticles, [&](size_t taskIndex) {
        const vec3f &position = (*positions)[taskIndex];
        const float &radius   = (*radii)[taskIndex];

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

      // filter out any prims with radius <= 0. note we need to leave other
      // arrays such as primRadii unchanged, as we will not change primID values
      // in this operation.
      const bool haveZeroRadiiParticles = std::any_of(
          primRadii.begin(), primRadii.end(), [](float r) { return r <= 0; });

      if (haveZeroRadiiParticles) {
        AlignedVector<RTCBuildPrimitive> primsFiltered;
        primsFiltered.reserve(numParticles);

        std::copy_if(
            prims.begin(),
            prims.end(),
            std::back_inserter(primsFiltered),
            [&](RTCBuildPrimitive p) { return primRadii[p.primID] > 0; });

        LogMessageStream(this->device.ptr, VKL_LOG_DEBUG)
            << "filtered out " << prims.size() - primsFiltered.size() << " / "
            << prims.size() << " particles with <= 0 radius" << std::endl;

        if (primsFiltered.size() == 0) {
          throw std::runtime_error("no particles with radius > 0 provided");
        }

        prims = primsFiltered;
      }

      numBVHParticles = prims.size();

      bvhBuildAllocator = make_unique<BvhBuildAllocator>(this->getDevice());

      userPtrStruct myUPS{&primRadii, bvhBuildAllocator.get()};

      rtcBVH = rtcNewBVH(rtcDevice);
      if (!rtcBVH) {
        throw std::runtime_error("bvh creation failure");
      }

      RTCBuildArguments arguments      = rtcDefaultBuildArguments();
      arguments.byteSize               = sizeof(arguments);
      arguments.buildFlags             = RTC_BUILD_FLAG_NONE;
      arguments.buildQuality           = RTC_BUILD_QUALITY_LOW;
      arguments.maxBranchingFactor     = 2;
      arguments.maxDepth               = 1024;
      arguments.sahBlockSize           = 1;
      arguments.minLeafSize            = MAX_PRIMS_PER_LEAF;
      arguments.maxLeafSize            = MAX_PRIMS_PER_LEAF;
      arguments.traversalCost          = 0.01f;
      arguments.intersectionCost       = 0.99f;
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
      arguments.userPtr                = &myUPS;

      rtcRoot = (Node *)rtcBuildBVH(&arguments);
      if (!rtcRoot) {
        throw std::runtime_error("bvh build failure");
      }

      if (rtcRoot->nominalLength.x < 0) {
        auto &val = ((ParticleLeafNode *)rtcRoot)->bounds;
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
      const float uncertainty = 0.1f;

      // build vector of leaf nodes
      std::vector<LeafNode *> leafNodes;
      leafNodes.reserve(numBVHParticles);

      getLeafNodes(rtcRoot, leafNodes);

      // compute value ranges of leaf nodes in parallel
      std::shared_ptr<Sampler<W>> sampler(newSampler());
      sampler->commit();

      if (estimateValueRanges) {
        // restrict to first attribute index
        const unsigned int attributeIndex = 0;
        const vfloatn<1> time             = {0.f};

        tasking::parallel_for(leafNodes.size(), [&](size_t leafNodeIndex) {
          ParticleLeafNode *leafNode =
              static_cast<ParticleLeafNode *>(leafNodes[leafNodeIndex]);

          range1f computedValueRange(empty);

          // estimates use sampling interfaces directly, to ensure all
          // constraints are consistently considered (e.g.
          // clampMaxCumulativeValue, radiusSupportFactor)

          for (uint64_t i = 0; i < leafNode->numCells; i++) {
            const uint64_t particleIndex = leafNode->cellIDs[i];

            // sample over regular grid within particle radius to improve
            // estimate; the intent is to cover regions where particles may
            // overlap
            const vec3f particleCenter = (*positions)[particleIndex];
            const float particleRadius = (*radii)[particleIndex];

            box3fa particleBounds = empty;
            particleBounds.extend(particleCenter -
                                  radiusSupportFactor * vec3f(particleRadius));
            particleBounds.extend(particleCenter +
                                  radiusSupportFactor * vec3f(particleRadius));

            // choose an odd value to cover particle center
            const int samplesPerDimension = 5;

            computedValueRange.extend(computeValueRangeOverBoundingBox(
                sampler, attributeIndex, particleBounds, samplesPerDimension));
          }

          // sample over regular grid within leaf bounds to improve estimate;
          // this addresses contributions from particles in other overlapping
          // leaf nodes
          const box3fa leafBounds = leafNode->bounds;

          const int samplesPerDimension = 10;

          computedValueRange.extend(computeValueRangeOverBoundingBox(
              sampler, attributeIndex, leafBounds, samplesPerDimension));

          // apply uncertainty to computed value range
          computedValueRange.lower *= (1.f - uncertainty);
          computedValueRange.upper *= (1.f + uncertainty);

          leafNode->valueRange = computedValueRange;
        });
      } else {
        tasking::parallel_for(leafNodes.size(), [&](size_t leafNodeIndex) {
          ParticleLeafNode *leafNode =
              static_cast<ParticleLeafNode *>(leafNodes[leafNodeIndex]);
          leafNode->valueRange = range1f(0.f, clampMaxCumulativeValue);
        });
      }

      accumulateNodeValueRanges(rtcRoot);
      addLevelToNodes(rtcRoot, 0);
      bvhDepth = getMaxNodeLevel(rtcRoot);

      valueRange = rtcRoot->valueRange;
    }

    VKL_REGISTER_VOLUME(ParticleVolume<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_particle_, VKL_TARGET_WIDTH))

  }  // namespace cpu_device
}  // namespace openvkl
