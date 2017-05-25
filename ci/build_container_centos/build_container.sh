#!/bin/bash

yum install -y rh-git29-git golang libtool make openssl wget time python27-python-pip \
               unzip java-1.8.0-openjdk-devel which cmake devtoolset-4-libatomic-devel

# bazel
wget https://people.centos.org/tru/bazel-centos7/bazel-0.4.5-1.el7.centos.x86_64.rpm && \
  rpm -ivh bazel-0.4.5-1.el7.centos.x86_64.rpm && rm bazel-0.4.5-1.el7.centos.x86_64.rpm

# virtualenv
pip install virtualenv

# buildifier
export GOPATH=/usr/lib/go
go get github.com/bazelbuild/buildifier/buildifier

# GCC for everything.
export CC=gcc
export CXX=g++
CXX_VERSION="$(${CXX} --version | grep ^g++)"
if [[ "${CXX_VERSION}" != "g++ (GCC) 5.3.1 20160406 (Red Hat 5.3.1-6)" ]]; then
  echo "Unexpected compiler version: ${CXX_VERSION}"
  exit 1
fi

# enable git29, need set +e
set +e
source /usr/bin/scl_source enable rh-git29
set -e

export THIRDPARTY_DEPS=/tmp
export THIRDPARTY_SRC=/thirdparty
export THIRDPARTY_BUILD=/thirdparty_build
DEPS=$(python <(cat target_recipes.bzl; \
  echo "print ' '.join(\"${THIRDPARTY_DEPS}/%s.dep\" % r for r in set(TARGET_RECIPES.values()))"))
echo "Building deps ${DEPS}"
"$(dirname "$0")"/build_and_install_deps.sh ${DEPS}
