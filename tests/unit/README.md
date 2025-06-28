# Unit Tests

This directory contains the C++ unit tests for the key-value store. The `run_tests.sh` script
builds the test binary using CMake and executes it. Code coverage is generated if `lcov` and `genhtml` are installed.

## Running

```bash
# From the repository root
cd tests/unit
./run_tests.sh
```

The script creates a `build/` folder inside this directory, compiles the `kvstore_tests`
executable and then runs it. Coverage results are placed under `build/coverage_report/`
when the required tools are available.
