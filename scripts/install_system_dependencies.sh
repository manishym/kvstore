#!/bin/bash
set -euo pipefail

echo "📦 Updating apt cache..."
apt-get update

echo "📦 Installing system dependencies..."
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
  autoconf \
  automake \
  binutils-dev \
  build-essential \
  ca-certificates \
  clang \
  cmake \
  curl \
  g++ \
  gcc \
  git \
  gnupg \
  htop \
  jq \
  lcov \
  libaio-dev \
  libboost-all-dev \
  libbsd-dev \
  libcli11-dev \
  libcunit1-dev \
  libdouble-conversion-dev \
  libevent-dev \
  libfmt-dev \
  libfuse3-dev \
  libgflags-dev \
  libgoogle-glog-dev \
  libgrpc++-dev \
  libgrpc-dev \
  libgtest-dev \
  libiberty-dev \
  libjemalloc-dev \
  libjson-c-dev \
  liblz4-dev \
  libncurses-dev \
  libnuma-dev \
  libpci-dev \
  libprotobuf-dev \
  libsnappy-dev \
  libssl-dev \
  libtool \
  libunwind-dev \
  libzstd-dev \
  meson \
  nasm\
  ninja-build \
  nlohmann-json3-dev \
  pkg-config \
  protobuf-compiler \
  protobuf-compiler-grpc \
  python3 \
  python3-pip \
  python3-pyelftools \
  software-properties-common \
  tmux \
  unzip \
  uuid-dev \
  wget \
  zlib1g-dev


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
echo "✅ FastFloat and Folly installed successfully."


echo "📦 Installing SPDK..."

git clone https://github.com/intel/isa-l /tmp/isa-l
cd /tmp/isa-l
./autogen.sh
./configure
make
make install
cd /

rm -rf /tmp/isa-l



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

