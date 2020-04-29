#!/bin/bash -xe

## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

git log -1

# environment for benchmark client
source ~/benchmark_client.git/env.sh
source ~/system_token.sh

# benchmark configuration
SOURCE_ROOT=`pwd`
PROJECT_NAME="Open VKL"
BENCHMARK_FLAGS="--benchmark_min_time=${BENCHMARK_MIN_TIME_SECONDS:-10}"

export LD_LIBRARY_PATH=`pwd`/build/install/lib:${LD_LIBRARY_PATH}

cd build/install/bin
rm -f *.json

################################# PLEASE READ ##################################
#
# Note that suites and subsuites must exist in the database _before_ attempting
# insertion of results. This is intentional! You should think carefully about
# your [suite -> subsuite -> benchmark] hierarchy and definitions. These should
# be stable over time (especially for suites and subsuites) to facilitate
# long-term comparisons.
#
# These can be inserted using the benchmark client, through the "insert suite"
# and "insert subsuite" commands. Ask for help if you have questions.
#
################################# PLEASE READ ###################################

################################
# Structured volume benchmarks #
################################

SUITE_NAME="StructuredVolume"

SUBSUITE_NAME="Sampling"
SUBSUITE_REGEX="Sample"
./vklBenchmarkStructuredVolume ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json

# wait to insert contexts (which will be reused for all subsequent benchmark runs)
# until first benchmark successfully finishes.
benny insert code_context "${PROJECT_NAME}" ${SOURCE_ROOT} --save-json code_context.json
benny insert run_context ${TOKEN} ./code_context.json --save-json run_context.json

benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

# subsequent subsuite runs...

SUBSUITE_NAME="Gradients"
SUBSUITE_REGEX="Gradient"
./vklBenchmarkStructuredVolume ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

SUBSUITE_NAME="IntervalIterators"
SUBSUITE_REGEX="IntervalIterator"
./vklBenchmarkStructuredVolume ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

#########################
# VDB volume benchmarks #
#########################

SUITE_NAME="VDBVolume"

SUBSUITE_NAME="Sampling"
SUBSUITE_REGEX="Sample"
./vklBenchmarkVdbVolume ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

SUBSUITE_NAME="IntervalIterators"
SUBSUITE_REGEX="IntervalIterator"
./vklBenchmarkVdbVolume ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

###############################
# Example renderer benchmarks #
###############################

SUITE_NAME="ExampleRenderers"

SUBSUITE_NAME="StructuredVolume"
SUBSUITE_REGEX="structured_regular"
./vklBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

SUBSUITE_NAME="VDBVolume"
SUBSUITE_REGEX="vdb"
./vklBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

SUBSUITE_NAME="UnstructuredVolume"
SUBSUITE_REGEX="unstructured"
./vklBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json
