// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Catch2 include wrapper: v3 no longer pulls Approx into the global namespace

#pragma once

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
