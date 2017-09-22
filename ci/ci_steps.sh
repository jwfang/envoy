#!/bin/bash

# Script that lists all the steps take by the CI system when doing Envoy builds.
set -e

. ./ci/envoy_build_sha.sh

# Lint travis file.
travis lint .travis.yml --skip-completion-check

# Where the Envoy build takes place.
export ENVOY_BUILD_DIR=/tmp/envoy-docker-build

function finish {
  echo "disk space at end of build:"
  df -h
}
trap finish EXIT

echo "disk space at beginning of build:"
df -h

# Do a build matrix with different types of builds docs, coverage, bazel.release, etc.
if [ "$TEST_TYPE" == "docs" ]
then
  echo "docs build..."
  ./docs/build.sh
  ./docs/publish.sh
  exit 0
elif [ "$TEST_TYPE" == "build_image" ]
then
  # The script builds lyft/envoy-build-* and pushes that image when ci/build_container
  # has changed on a push to master.
  echo "lyft/envoy-build-* pushing..."
  ./ci/build_container/docker_push.sh
else
  docker run -t -i -v "$ENVOY_BUILD_DIR":/build -v $TRAVIS_BUILD_DIR:/source \
    lyft/envoy-build:$ENVOY_BUILD_SHA /bin/bash -c "cd /source && ci/do_ci.sh $TEST_TYPE"

  if [ "$TEST_TYPE" == "bazel.coverage" ]
  then
    ./ci/coverage_publish.sh
  elif [ "$TEST_TYPE" == "bazel.release" ]
  then
    mkdir -p build_release
    cp -f "$ENVOY_BUILD_DIR"/envoy/source/exe/envoy ./build_release
    # This script builds lyft/envoy and lyft/envoy-alpine images and pushes that image on merge to master.
    # It also makes sure that the docker examples (https://envoyproxy.github.io/envoy/install/sandboxes/sandboxes.html)
    # can be built and run.
    ./ci/docker_push.sh
  fi

fi
