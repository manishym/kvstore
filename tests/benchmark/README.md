# Benchmark Tests

This directory contains Go programs used to benchmark the key-value store.
Results are written to the `results/` folder.

## Running

Use Go 1.21 or newer. From this directory run:

```bash
go run ./cmd -server localhost:50051 -concurrency 50 -requests 1000
```

The command accepts flags for the server address, level of concurrency,
number of requests, path to the `kvstore.proto` file and an optional `tag`
used to annotate the results. CSV output is placed in the `results/`
subdirectory.
