# End-to-End Tests

This directory contains the Python end-to-end tests. The `run_tests.sh` script
creates a virtual environment, installs dependencies, generates the gRPC client
code and then runs the test suite with `pytest`.

## Running

```bash
# From the repository root
cd tests/e2e
./run_tests.sh
```

The server binary is built automatically if it does not already exist. Test
results are shown in the console. The generated gRPC Python files and the
virtual environment are created locally in this directory.

The tests use port `50051` by default. Set the `KVSTORE_PORT` environment
variable before running the tests to use a different port:

```bash
KVSTORE_PORT=60000 ./run_tests.sh
```
