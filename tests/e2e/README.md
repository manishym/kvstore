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
