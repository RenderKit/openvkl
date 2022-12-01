// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <openvkl/openvkl.h>

#include <openvkl/device/openvkl.h>

void initializeOpenVKL();
void shutdownOpenVKL();
VKLDevice getOpenVKLDevice();
#ifdef OPENVKL_TESTING_GPU
sycl::queue getSyclQueue();
#endif