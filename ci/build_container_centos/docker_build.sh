#!/bin/bash

# Using Docker relative path workaround from 
# https://github.com/docker/docker/issues/2745#issuecomment-253230025
tar cf - . -C ../build_container \
              build_and_install_deps.sh Makefile recipe_wrapper.sh build_recipes/ \
           -C ../../bazel \
              target_recipes.bzl | \
    docker build --rm -t jwfang/envoy-build-centos:fjw -
