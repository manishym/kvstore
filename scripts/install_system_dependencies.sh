#!/bin/bash
set -euo pipefail

echo "📦 Updating apt cache..."
apt-get update

echo "📦 Installing system dependencies..."
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
  build-essential \
  gcc \
  g++ \
  clang \
  cmake \
  git \
  wget \
  curl \
  python3 \
  python3-pip \
  libnuma-dev \
  libaio-dev \
  libssl-dev \
  uuid-dev \
  unzip \
  htop \
  tmux \
  jq \
  libgtest-dev \
  pkg-config \
  ca-certificates \
  gnupg \
  software-properties-common \
  libboost-all-dev \
  libfmt-dev \
  libgflags-dev \
  libgoogle-glog-dev \
  libevent-dev \
  libjemalloc-dev \
  libzstd-dev \
  liblz4-dev \
  libsnappy-dev \
  libdouble-conversion-dev \
  libunwind-dev \
  zlib1g-dev \
  binutils-dev \
  nlohmann-json3-dev \
  libgrpc++-dev \
  libgrpc-dev \
  protobuf-compiler \
  protobuf-compiler-grpc \
  libprotobuf-dev \
  libiberty-dev \
  lcov \
  libpci-dev \
  libnuma-dev \
  uuid-dev \
  libaio-dev \
  libjson-c-dev \
  libpci-dev \
  libnuma-dev \
  uuid-dev \
  libaio-dev \
  libjson-c-dev \
  libbsd-dev \
  libfuse3-dev \
  meson \
  ninja-build\
  libbsd-dev \
  libcunit1-dev \
  libncurses-dev \
  python3 python3-pip\
  python3-pyelftools 


echo "✅ System dependencies installed successfully."

echo "📂 Installing FastFloat..."
git clone https://github.com/fastfloat/fast_float.git /tmp/fast_float
mkdir -p /usr/local/include/fast_float
cp /tmp/fast_float/include/fast_float/*.h /usr/local/include/fast_float
rm -rf /tmp/fast_float

echo "📂 Installing Folly..."
git clone https://github.com/facebook/folly.git /tmp/folly
cd /tmp/folly
cmake -S . -B _build
make -C _build -j$(nproc)
make -C _build install
cd /
rm -rf /tmp/folly


echo "📦 Installing SPDK..."

# Clone and build SPDK
git clone https://github.com/spdk/spdk.git /tmp/spdk
cd /tmp/spdk
git submodule update --init

# Use minimal build to save time
./configure --prefix=/usr/local --with-shared --without-nvme-cuse
make -j$(nproc)
make install

cd /
rm -rf /tmp/spdk

echo "✅ SPDK installed."

echo "✅ FastFloat and Folly installed successfully."
