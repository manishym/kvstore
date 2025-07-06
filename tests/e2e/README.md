# E2E Test Suite for SPDK Key-Value Store

This directory contains comprehensive end-to-end tests for the SPDK Key-Value Store, designed to validate functionality, performance, and reliability across different configurations.

## Overview

The E2E test suite provides:

1. **Dynamic Configuration Discovery** - Automatically discovers and tests all configuration combinations
2. **Comprehensive Test Coverage** - Basic functionality, stress testing, latency benchmarking, and fault injection
3. **Fault Injection Testing** - Simulates server crashes and validates WAL recovery
4. **Concurrent Client Testing** - Validates system behavior under concurrent load
5. **Detailed Logging** - Captures server and client logs for post-mortem analysis
6. **CI/CD Integration** - GitHub Actions workflow for automated testing

## Test Configurations

The test suite supports the following configuration combinations:

### WAL Types
- **SPDK WAL** - High-performance WAL using SPDK for NVMe devices
- **Block Device WAL** - File-based WAL for block devices
- **Passthrough WAL** - No-op WAL for testing without persistence

### Memtable Types
- **Skiplist** - High-performance ordered data structure
- **std::map** - Standard C++ map implementation
- **boost::unordered_map** - Boost hash map implementation

### Configuration Files

All configuration files are located in `configs/`:

```
configs/
├── spdk_wal_skiplist.json
├── spdk_wal_stdmap.json
├── spdk_wal_boostmap.json
├── block_wal_skiplist.json
├── block_wal_stdmap.json
├── block_wal_boostmap.json
├── passthrough_wal_skiplist.json
├── passthrough_wal_stdmap.json
└── passthrough_wal_boostmap.json
```

## Test Types

### 1. Basic Functionality Tests (`test_basic_functionality`)
- Put/Get operations
- Delete operations
- Non-existent key handling
- Data consistency validation

### 2. Stress and Latency Tests (`test_stress_and_latency`)
- High-volume operations (configurable via `STRESS_TEST_OPERATIONS`)
- Latency measurements for Put/Get/Delete operations
- Throughput calculation
- P95 latency statistics

### 3. Fault Injection Tests (`test_fault_injection`)
- Server crash simulation
- WAL recovery validation
- Data consistency after recovery
- Recovery time measurement

### 4. Concurrent Client Tests (`test_concurrent_clients`)
- Multiple concurrent clients
- Race condition testing
- System stability under load
- Success rate measurement

## Running Tests

### Prerequisites

1. **System Dependencies**
   ```bash
   sudo apt-get update
   sudo apt-get install -y build-essential cmake libboost-all-dev
   sudo apt-get install -y libspdk-dev spdk-tools
   ```

2. **Python Dependencies**
   ```bash
   cd tests/e2e
   pip install -r requirements.txt
   ```

### Using the Test Runner Script

The easiest way to run tests is using the provided script:

```bash
# Run all tests for all configurations
./run_e2e_tests.sh

# Run specific test types
./run_e2e_tests.sh basic      # Basic functionality tests
./run_e2e_tests.sh stress     # Stress and latency tests
./run_e2e_tests.sh fault      # Fault injection tests
./run_e2e_tests.sh concurrent # Concurrent client tests

# Show help
./run_e2e_tests.sh help
```

### Using pytest Directly

```bash
# Run all tests
python -m pytest test_matrix.py -v

# Run specific test type
python -m pytest test_matrix.py::test_basic_functionality -v

# Run with specific configuration
python -m pytest test_matrix.py --config-name=spdk_wal_skiplist -v

# Generate JUnit XML report
python -m pytest test_matrix.py --junitxml=results.xml
```

### Environment Variables

You can customize test parameters using environment variables:

```bash
export STRESS_TEST_OPERATIONS=2000      # Default: 1000
export LATENCY_TEST_OPERATIONS=200      # Default: 100
export FAULT_INJECTION_OPERATIONS=100   # Default: 50
export CONCURRENT_CLIENTS=10            # Default: 5

./run_e2e_tests.sh
```

## Test Output and Logging

### Directory Structure

```
tests/e2e/
├── configs/           # Configuration files
├── results/           # Test results and reports
├── logs/             # Detailed logs
├── test_matrix.py    # Main test suite
└── run_e2e_tests.sh # Test runner script
```

### Log Files

- **Server Logs**: `logs/{config_name}_server_stdout.log` and `logs/{config_name}_server_stderr.log`
- **Test Logs**: `logs/{config_name}_test.log`
- **Results**: `results/{config_name}_results.json`
- **JUnit XML**: `results/{config_name}_junit.xml`

### Example Output

```
[INFO] Starting E2E test suite...
[INFO] Found 9 configurations to test
[INFO] Running tests for configuration: spdk_wal_skiplist
[SUCCESS] Tests passed for configuration: spdk_wal_skiplist
[INFO] Running tests for configuration: block_wal_stdmap
[SUCCESS] Tests passed for configuration: block_wal_stdmap
...
[SUCCESS] All tests passed! (9/9)
```

## CI/CD Integration

### GitHub Actions

The `.github/workflows/e2e-tests.yml` file provides:

1. **Matrix Testing** - Runs tests across all configuration combinations
2. **Parallel Execution** - Tests run in parallel for faster completion
3. **Artifact Upload** - Test results and logs are preserved
4. **Failure Reporting** - Detailed failure information in pull requests

### Local CI

For local development, you can run the same tests locally:

```bash
# Run full test matrix
./run_e2e_tests.sh

# Check results
cat tests/e2e/results/test_summary.txt
```

## Adding New Configurations

To add a new configuration:

1. Create a new JSON file in `configs/` directory
2. Follow the existing configuration format
3. Include required fields: `map_type`, `map_options`, and `wal`
4. The test suite will automatically discover and test the new configuration

Example new configuration:

```json
{
    "map_type": "custom_map",
    "map_options": {
        "custom_map": {
            "initial_size": 1000,
            "custom_option": "value"
        }
    },
    "wal": {
        "type": "custom_wal",
        "device": "custom_device"
    }
}
```

## Troubleshooting

### Common Issues

1. **Server Build Failures**
   ```bash
   # Clean build directory
   rm -rf tests/unit/build
   ./run_e2e_tests.sh
   ```

2. **Port Conflicts**
   ```bash
   # Kill existing kvstore server processes (more specific)
   pgrep -f "tests/unit/build/server" && pkill -f "tests/unit/build/server"
   pgrep -f "kvstore.*server" && pkill -f "kvstore.*server"
   ```

3. **Permission Issues**
   ```bash
   # Make script executable
   chmod +x run_e2e_tests.sh
   ```

4. **Missing Dependencies**
   ```bash
   # Install Python dependencies
   pip install -r requirements.txt
   
   # Install system dependencies
   sudo apt-get install -y build-essential cmake libboost-all-dev
   ```

### Debug Mode

For detailed debugging, run tests with verbose output:

```bash
python -m pytest test_matrix.py -v -s --log-level=DEBUG
```

### Analyzing Test Results

1. **Check Test Summary**
   ```bash
   cat tests/e2e/results/test_summary.txt
   ```

2. **View Detailed Logs**
   ```bash
   tail -f tests/e2e/logs/e2e_tests.log
   ```

3. **Analyze Performance Results**
   ```bash
   # Look for latency and throughput metrics in test logs
   grep "latency\|throughput" tests/e2e/logs/*.log
   ```

## Performance Benchmarks

The test suite includes built-in performance benchmarking:

- **Throughput**: Operations per second
- **Latency**: Average and P95 latencies for Put/Get/Delete operations
- **Recovery Time**: Time to recover after server crash
- **Concurrent Performance**: System behavior under concurrent load

Results are logged and can be analyzed for performance regression detection.

## Contributing

When adding new tests or modifying existing ones:

1. Follow the existing test structure
2. Add proper logging and error handling
3. Include cleanup in test fixtures
4. Document any new configuration options
5. Update this README if needed

## License

This test suite is part of the SPDK Key-Value Store project and follows the same license terms.
