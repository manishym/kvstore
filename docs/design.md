# KVStore Design Overview

## Goals

The project implements a simple key–value storage service that exposes a gRPC
API.  Durability is optional and provided via pluggable write‑ahead log (WAL)
implementations.  The code is intentionally small so it can serve as a base for
experiments with different storage backends or in‑memory map structures.

## Architecture

```
+---------------+       gRPC        +----------------+
|  gRPC Client  | <---------------> | AsyncKVServer  |
+---------------+                   +----------------+
        |                                    |
        |  In-memory store (folly skiplist)   |
        |------------------------------------>|
        |                                     |
        |    optional persistence via WAL     |
        +------------------------------------>|
```

### AsyncKVServer

- Listens on a configurable address (default `0.0.0.0:50051`).
- Uses `folly::ConcurrentSkipList` to store key–value pairs in memory.
- Each RPC type (`Put`, `Get`, `Delete`) has an asynchronous handler defined in
  `server_impl.h`.
- If a WAL instance is provided, every `Put` and `Delete` operation is appended
  to the log before completing the RPC.
- On startup the server replays the WAL to rebuild the skiplist.

### gRPC Service

`proto/kvstore.proto` defines the service:

```proto
service KeyValueStore {
  rpc Put(PutRequest) returns (PutResponse);
  rpc Get(GetRequest) returns (GetResponse);
  rpc Delete(DeleteRequest) returns (DeleteResponse);
}
```

Each request and response message contains simple key/value or status fields.

### Write‑Ahead Log (WAL)

The abstract interface is declared in `src/wal.h`.  Implementations include:

- **BlockDeviceWAL** – appends log entries to a file.  Used by default or when
  the configuration sets `"wal": {"type": "block"}`.
- **SpdkWAL** – placeholder for a future SPDK backed WAL.  Configuration uses
  `"wal": {"type": "spdk"}` with options such as `spdk_bdev` or
  `wal_segment_size`.
- **PassThroughWAL** – no persistence; methods return success immediately.

Log entries are serialized using `WalEntrySerializer` or the low‑level
`WalSerializer` helpers.  Example usage can be found in
`docs/wal_serialization_examples.md`.

### Map Abstraction

`src/map` contains a small factory that can create different in‑memory maps for
experimentation.  JSON configuration chooses between `BoostMap` (based on
`boost::container::flat_map`) and `StdMap` (thin wrapper over `std::map`).
These are mostly used by the unit tests.

### Go Client/Server

The `src_go` directory provides minimal Go implementations of the client and
server using the same protobuf definitions.  They are useful for benchmarks and
interoperability tests.

## Testing and Benchmarks

- **Unit tests** (C++, GoogleTest) live under `tests/unit`.  The `run_tests.sh`
  script builds and executes them and optionally generates coverage data.
- **End-to-end tests** (Python, pytest) reside in `tests/e2e`.  They spin up the
  C++ server and verify behaviour via gRPC.
- **Benchmarks** written in Go are available under `tests/benchmark`.

## Configuration

Runtime configuration is supplied as JSON.  Examples are in `src/config/`.
Important fields include:

- `address` – server listen address.
- `wal.type` and `wal.device` – choose and configure the WAL implementation.
- `map_type` and `map_options` – control the map factory (mainly for tests).

## Build

The project uses CMake for the C++ code.  Generated protobuf files are placed in
the build directory.  See `README.md` for detailed build instructions.
