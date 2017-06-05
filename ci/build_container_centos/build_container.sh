#!/bin/bash

sed -ri -e "/mirrorlist/d" -e "s|^#baseurl|baseurl|" \
        -e "s|http://.*.centos.org(/centos)?|http://mirrors.ustc.edu.cn/centos|" \
        /etc/yum.repos.d/*.repo
sed -i -e "s/^enabled=1/enabled=0/" /etc/yum/pluginconf.d/fastestmirror.conf

yum install -y rh-git29-git golang libtool make openssl wget time python27-python-pip \
               unzip java-1.8.0-openjdk-devel which cmake devtoolset-4-libatomic-devel \
               clang

# bazel
wget https://github.com/bazelbuild/bazel/releases/download/0.4.5/bazel-0.4.5-installer-linux-x86_64.sh && \
  chmod ug+x ./bazel-0.4.5-installer-linux-x86_64.sh && ./bazel-0.4.5-installer-linux-x86_64.sh && \
  rm ./bazel-0.4.5-installer-linux-x86_64.sh

# enable git29/pip, need set +e
set +e
source /usr/bin/scl_source enable rh-git29
source /usr/bin/scl_source enable python27
set -e

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

export THIRDPARTY_DEPS=/tmp
export THIRDPARTY_SRC=/thirdparty
export THIRDPARTY_BUILD=/thirdparty_build
DEPS=$(python <(cat target_recipes.bzl; \
  echo "print ' '.join(\"${THIRDPARTY_DEPS}/%s.dep\" % r for r in set(TARGET_RECIPES.values()))"))
echo "Building deps ${DEPS}"
"$(dirname "$0")"/build_and_install_deps.sh ${DEPS}
