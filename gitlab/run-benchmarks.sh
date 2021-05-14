#!/bin/bash -xe

## Copyright 2020-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

git log -1

# environment for benchmark client
source ~/benchmark_client.git/env.sh
source ~/system_token.sh

# benchmark configuration
SOURCE_ROOT=`pwd`
PROJECT_NAME="Open VKL"
BENCHMARK_FLAGS="--benchmark_repetitions=${BENCHMARK_REPETITIONS:-5}"

# can be used to restrict processes to the first CPU core (so only for
# single-threaded benchmarks!)
RESTRICT_TO_CORE0="numactl -C 0 -m 0"

SUITE_REGEX=${BENCHMARK_SUITE:-.*}

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

initContext() {
  if [ -z "$HAVE_CONTEXT" ]; then
    HAVE_CONTEXT=1
    benny insert code_context "${PROJECT_NAME}" ${SOURCE_ROOT} --save-json code_context.json
    benny insert run_context ${TOKEN} ./code_context.json --save-json run_context.json
  fi
}

################################
# Structured volume benchmarks #
################################

SUITE_NAME="StructuredVolume"
if [[ $SUITE_NAME =~ $SUITE_REGEX ]]
then
  SUBSUITE_NAME="Sampling"
  SUBSUITE_REGEX="Sample"
  ${RESTRICT_TO_CORE0} ./vklBenchmarkStructuredVolume ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  # wait to insert contexts (which will be reused for all subsequent benchmark runs)
  # until first benchmark successfully finishes.
  initContext

  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  # subsequent subsuite runs...

  SUBSUITE_NAME="Gradients"
  SUBSUITE_REGEX="Gradient"
  ${RESTRICT_TO_CORE0} ./vklBenchmarkStructuredVolume ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  SUBSUITE_NAME="IntervalIterators"
  SUBSUITE_REGEX="IntervalIterator"
  ${RESTRICT_TO_CORE0} ./vklBenchmarkStructuredVolume ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json
fi

##################################################
# Structured volume (multi-attribute) benchmarks #
##################################################

SUITE_NAME="StructuredVolumeMulti"
if [[ $SUITE_NAME =~ $SUITE_REGEX ]]
then
  SUBSUITE_NAME="ScalarSampling"
  SUBSUITE_REGEX="scalar.*Sample"
  ${RESTRICT_TO_CORE0} ./vklBenchmarkStructuredVolumeMulti ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  initContext
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  SUBSUITE_NAME="VectorSampling"
  SUBSUITE_REGEX="vector.*Sample"
  ${RESTRICT_TO_CORE0} ./vklBenchmarkStructuredVolumeMulti ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  SUBSUITE_NAME="StreamSampling"
  SUBSUITE_REGEX="stream.*Sample"
  ${RESTRICT_TO_CORE0} ./vklBenchmarkStructuredVolumeMulti ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json
fi

#########################
# VDB volume benchmarks #
#########################

SUITE_NAME="VDBVolume"
if [[ $SUITE_NAME =~ $SUITE_REGEX ]]
then
  SUBSUITE_NAME="Sampling"
  SUBSUITE_REGEX="Sample"
  ${RESTRICT_TO_CORE0} ./vklBenchmarkVdbVolume ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  initContext
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  SUBSUITE_NAME="Gradients"
  SUBSUITE_REGEX="Gradient"
  ${RESTRICT_TO_CORE0} ./vklBenchmarkVdbVolume ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  SUBSUITE_NAME="IntervalIterators"
  SUBSUITE_REGEX="IntervalIterator"
  ${RESTRICT_TO_CORE0} ./vklBenchmarkVdbVolume ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json
fi

############################################
# VDB volume (multi-attribute) benchmarks #
###########################################

SUITE_NAME="VDBVolumeMulti"
if [[ $SUITE_NAME =~ $SUITE_REGEX ]]
then
  SUBSUITE_NAME="ScalarSampling"
  SUBSUITE_REGEX="scalar.*Sample"
  ${RESTRICT_TO_CORE0} ./vklBenchmarkVdbVolumeMulti ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  initContext
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  SUBSUITE_NAME="VectorSampling"
  SUBSUITE_REGEX="vector.*Sample"
  ${RESTRICT_TO_CORE0} ./vklBenchmarkVdbVolumeMulti ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  SUBSUITE_NAME="StreamSampling"
  SUBSUITE_REGEX="stream.*Sample"
  ${RESTRICT_TO_CORE0} ./vklBenchmarkVdbVolumeMulti ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json
fi

###############################
# Example renderer benchmarks #
###############################

# Note that example renderer benchmarks are multi-threaded

SUITE_NAME="ExampleRenderers"
if [[ $SUITE_NAME =~ $SUITE_REGEX ]]
then
  SUBSUITE_NAME="StructuredVolume"
  SUBSUITE_REGEX="structured_regular"
  ./vklBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  initContext
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  SUBSUITE_NAME="VDBVolume"
  SUBSUITE_REGEX="vdb"
  ./vklBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

  SUBSUITE_NAME="UnstructuredVolume"
  SUBSUITE_REGEX="unstructured"
  ./vklBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
  benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json
fi
