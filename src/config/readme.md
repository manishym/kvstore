# Configuration Overview

This directory contains example JSON files used to configure the key-value store server. The configuration is parsed in `server_main.cpp` and in helper factories.

## address
String specifying the address and port for the gRPC server. If omitted the server listens on `0.0.0.0:50051`.

## wal
Configuration for the Write Ahead Log (WAL). The `wal` object may contain the following fields:

- `type` – WAL implementation. Supported values are `block` and `spdk`. Unknown or missing values fall back to the block device WAL.
- `device` – configuration for the selected WAL type.

### Block device WAL
When `type` is `block`, `device` can be either a string path or an object with a `path` entry. The default path is `kvstore_block.wal`.

### SPDK WAL
When `type` is `spdk`, `device` may be a string naming the SPDK bdev or an object with additional options:

- `spdk_bdev` – name of the SPDK block device. Default `"NVMe0n1"`.
- `wal_segment_size` – size of each WAL segment in bytes. Default `67108864` (64 MB).
- `batch_size` – number of entries per batch. Default `32`.

If the `device` field is omitted, default values are used.

## map_type and map_options
These settings are used by `MapFactory` for unit tests. `map_type` selects the in-memory map implementation and options are provided under `map_options`:

- `"boost_map"` – options under `map_options.boost_map`:
  - `initial_size` – initial capacity.
  - `load_factor` – load factor.
- `"std_map"` – options under `map_options.std_map`:
  - `initial_size` – initial capacity.

If `map_type` is missing or unrecognized, `MapFactory::createMap` returns `nullptr`.
