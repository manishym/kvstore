# SPDK Key-Value Store

This project implements a key-value store using SPDK and gRPC. It provides a simple API for storing, retrieving, and deleting key-value pairs.

## Prerequisites

- CMake (version 3.10 or higher)
- C++ compiler with C++14 support
- Protocol Buffers (protobuf)
- Git (for submodules)

## Building the Project

1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd SpdkKeyValueStore
   ```

2. Initialize and update submodules:
   ```bash
   git submodule update --init --recursive
   ```

3. Create a build directory and navigate into it:
   ```bash
   mkdir build
   cd build
   ```

4. Configure the project with CMake:
   ```bash
   cmake ..
   ```

5. Build the project:
   ```bash
   make
   ```

6. Run the server:
   ```bash
   ./server
   ```

7. In another terminal, run the client:
   ```bash
   ./client
   ```

## Project Structure

- `proto/`: Contains the Protocol Buffers definition file (`kvstore.proto`).
- `src/`: Contains the C++ source files for the server and client.
- `generated/`: Contains the generated Protocol Buffers and gRPC files.
- `build/`: Contains the build artifacts.
- `third_party/`: Contains external dependencies (gRPC).

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details. 