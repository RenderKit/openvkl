#!/bin/bash -xe

## Copyright 2023 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

BENCHMARK_DEVICE=$1
case "$BENCHMARK_DEVICE" in
  "CPU" | "GPU")
    echo "Going to run benchmarks on $BENCHMARK_DEVICE device"
    ;;

  *)
    echo "$BENCHMARK_DEVICE is not valid device, please use CPU or GPU"
    exit 1
    ;;
esac

SOURCE_ROOT=$GITHUB_WORKSPACE
PROJECT_NAME="Open VKL"
BENCHMARK_FLAGS="--benchmark_repetitions=5 --benchmark_min_time=10"



################################# PLEASE READ ##################################
#
# Note that suites and subsuites must exist in the database _before_ attempting
# insertion of results. This is intentional! You should think carefully about
# your [suite -> subsuite -> benchmark] hierarchy and definitions. These should
# be stable over time (especially for suites and subsuites) to facilitate
# long-term comparisons.
#
# These can be inserted using the benchmark client, through the "ls"
# and "insert subsuite" commands. Ask for help if you have questions.
#
################################# PLEASE READ ###################################

initContext() {
  if [ -z "$HAVE_CONTEXT" ]; then
    HAVE_CONTEXT=1
    benny insert code_context "${PROJECT_NAME}" ${SOURCE_ROOT} --save-json code_context.json
    benny insert run_context ${BENNY_SYSTEM_TOKEN} ./code_context.json --save-json run_context.json
  fi
}


SUITE_NAME="ExampleRenderers"

initContext

SUBSUITE_NAME="StructuredVolume"
SUBSUITE_REGEX="structured_regular"
./bin/vklBenchmark${BENCHMARK_DEVICE} ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

SUBSUITE_NAME="VDBVolume"
SUBSUITE_REGEX="vdb"
./bin/vklBenchmark${BENCHMARK_DEVICE} ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

SUBSUITE_NAME="UnstructuredVolume"
SUBSUITE_REGEX="unstructured"
./bin/vklBenchmark${BENCHMARK_DEVICE} ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json