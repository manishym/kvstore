# SPDK Key-Value Store
![Unit Tests](https://github.com/manishym/kvstore/actions/workflows/c-cpp.yml/badge.svg)
![Code Coverage](https://github.com/manishym/kvstore/actions/workflows/pr-code-coverage.yaml/badge.svg)

This project implements a key-value store using SPDK and gRPC. It provides a simple API for storing, retrieving, and deleting key-value pairs.

## Prerequisites

The project uses a number of C++ libraries in addition to gRPC and Protobuf.
Make sure the following packages are available on your system before building:

- CMake (version 3.10 or higher)
- C++17 compiler (gcc or clang)
- gRPC and Protocol Buffers
- Boost (container component)
- Folly
- fmt
- nlohmann\_json
- gflags and glog
- libsnappy, liblz4 and libzstd
- libiberty (from binutils, required by folly)
- GTest (for running the unit tests)
- Git (for submodules)

### Installing Folly on Ubuntu

Folly is not available in the standard Ubuntu repositories. Use the
following steps to build and install it from source:

```bash
sudo apt-get install -y git cmake g++ libboost-all-dev \
    libdouble-conversion-dev libgoogle-glog-dev libgflags-dev \
    libevent-dev libssl-dev libfmt-dev libiberty-dev \
    libsnappy-dev liblz4-dev libzstd-dev

git clone https://github.com/facebook/folly.git
cd folly
git submodule update --init --recursive
mkdir _build && cd _build
cmake ..
make -j$(nproc)
sudo make install
```

More details are available in Folly's [installation guide](https://github.com/facebook/folly).

## Building the Project

1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd SpdkKeyValueStore
   ```

2. Install the required system packages (Ubuntu example):
   ```bash
   sudo apt-get update
   sudo apt-get install -y cmake g++ libboost-container-dev \
       libgrpc++-dev protobuf-compiler libprotobuf-dev \
       libgoogle-glog-dev libgflags-dev nlohmann-json3-dev \
       libfmt-dev libsnappy-dev liblz4-dev libzstd-dev libiberty-dev
   ```

   If Folly is not available as a package, follow the steps in the
   [Installing Folly on Ubuntu](#installing-folly-on-ubuntu) section.
3. Initialize and update submodules:
   ```bash
   git submodule update --init --recursive
   ```
4. Create a build directory and navigate into it:
   ```bash
   mkdir build
   cd build
   ```
5. Configure the project with CMake:
   ```bash
   cmake ..
   ```
   This step also generates the protobuf and gRPC source files in the
   `build/` directory.
6. Build the project:
   ```bash
   make
   ```
7. Run the server:
   ```bash
   ./server
   ```
   The server listens on port `50051` by default. To use a different port,
   edit the `address` field in `src/config/runtime_config.json` or pass a
   custom configuration file as the first argument when starting the server.
8. In another terminal, run the client:
   ```bash
   ./client
   ```

## Project Structure

- `proto/`: Contains the Protocol Buffers definition file (`kvstore.proto`).
- `src/`: Contains the C++ source files for the server and client.
- `build/`: Contains the build artifacts. Protobuf and gRPC source files are
  generated here when CMake configures the project.
- `third_party/`: Contains external dependencies (gRPC).

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details. 