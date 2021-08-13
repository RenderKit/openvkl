// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "openvkl_testing.h"

template <typename VOLUME_TYPE>
inline void num_attributes(std::shared_ptr<VOLUME_TYPE> v)
{
  VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());
  REQUIRE(vklGetNumAttributes(vklVolume) == v->getNumAttributes());
}

template <typename VOLUME_TYPE>
void computed_vs_api_value_range(std::shared_ptr<VOLUME_TYPE> v,
                                 unsigned int attributeIndex)
{
  VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());

  vkl_range1f apiValueRange = vklGetValueRange(vklVolume, attributeIndex);

  range1f computedValueRange = v->getComputedValueRange(attributeIndex);

  INFO("api valueRange = " << apiValueRange.lower << " "
                           << apiValueRange.upper);
  INFO("computed valueRange = " << computedValueRange.lower << " "
                                << computedValueRange.upper);

  REQUIRE((apiValueRange.lower == computedValueRange.lower &&
           apiValueRange.upper == computedValueRange.upper));
}
