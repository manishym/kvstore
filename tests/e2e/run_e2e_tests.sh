#!/bin/bash

# E2E Test Runner for SPDK Key-Value Store
# This script runs comprehensive end-to-end tests across all configurations

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
BUILD_DIR="$PROJECT_ROOT/tests/unit/build"
SERVER_PATH="$BUILD_DIR/server"
CONFIGS_DIR="$SCRIPT_DIR/configs"
RESULTS_DIR="$SCRIPT_DIR/results"
LOG_DIR="$SCRIPT_DIR/logs"

# Test parameters
STRESS_OPERATIONS=${STRESS_OPERATIONS:-1000}
LATENCY_OPERATIONS=${LATENCY_OPERATIONS:-100}
FAULT_OPERATIONS=${FAULT_OPERATIONS:-50}
CONCURRENT_CLIENTS=${CONCURRENT_CLIENTS:-5}

# Create directories
mkdir -p "$RESULTS_DIR" "$LOG_DIR"

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1" | tee -a "$LOG_DIR/e2e_tests.log"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1" | tee -a "$LOG_DIR/e2e_tests.log"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1" | tee -a "$LOG_DIR/e2e_tests.log"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$LOG_DIR/e2e_tests.log"
}

# Cleanup function
cleanup() {
    log_info "Cleaning up..."
    # Kill only our kvstore server processes (more specific)
    pkill -f "tests/unit/build/server" || pkill -f "kvstore.*server" || true
    # Remove temporary files
    find /tmp -name "kvstore_*" -type f -delete 2>/dev/null || true
}

# Set up trap for cleanup
trap cleanup EXIT

# Check dependencies
check_dependencies() {
    log_info "Checking dependencies..."
    
    # Check Python
    if ! command -v python3 &> /dev/null; then
        log_error "Python 3 is required but not installed"
        exit 1
    fi
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake is required but not installed"
        exit 1
    fi
    
    # Check gRPC Python package
    if ! python3 -c "import grpc" &> /dev/null; then
        log_error "gRPC Python package is required but not installed"
        exit 1
    fi
    
    log_success "All dependencies are available"
}

# Build server
build_server() {
    log_info "Building server..."
    
    if [[ ! -f "$SERVER_PATH" ]]; then
        log_info "Server not found, building..."
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
        if ! cmake -DCMAKE_BUILD_TYPE=Release "$PROJECT_ROOT"; then
            log_error "CMake configuration failed"
            exit 1
        fi
        
        if ! make -j$(nproc); then
            log_error "Server build failed"
            exit 1
        fi
        
        log_success "Server built successfully"
    else
        log_info "Server already exists, skipping build"
    fi
}

# Discover test configurations
discover_configs() {
    log_info "Discovering test configurations..."
    
    local configs=()
    for config_file in "$CONFIGS_DIR"/*.json; do
        if [[ -f "$config_file" ]]; then
            config_name=$(basename "$config_file" .json)
            configs+=("$config_name")
            log_info "Found config: $config_name"
        fi
    done
    
    if [[ ${#configs[@]} -eq 0 ]]; then
        log_error "No configuration files found in $CONFIGS_DIR"
        exit 1
    fi
    
    echo "${configs[@]}"
}

# Run tests for a specific configuration
run_tests_for_config() {
    local config_name="$1"
    local test_results_file="$RESULTS_DIR/${config_name}_results.json"
    local test_log_file="$LOG_DIR/${config_name}_test.log"
    
    log_info "Running tests for configuration: $config_name"
    
    # Set environment variables for test parameters
    export STRESS_TEST_OPERATIONS="$STRESS_OPERATIONS"
    export LATENCY_TEST_OPERATIONS="$LATENCY_OPERATIONS"
    export FAULT_INJECTION_OPERATIONS="$FAULT_OPERATIONS"
    export CONCURRENT_CLIENTS="$CONCURRENT_CLIENTS"
    
    # Run pytest with specific configuration
    cd "$SCRIPT_DIR"
    if "$SCRIPT_DIR/venv/bin/python" -m pytest test_matrix.py \
        -v \
        --tb=short \
        --junitxml="$RESULTS_DIR/${config_name}_junit.xml" \
        --log-file="$test_log_file" \
        --log-level=INFO; then
        
        log_success "Tests passed for configuration: $config_name"
        echo "{\"config\": \"$config_name\", \"status\": \"PASSED\", \"timestamp\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\"}" > "$test_results_file"
    else
        log_error "Tests failed for configuration: $config_name"
        echo "{\"config\": \"$config_name\", \"status\": \"FAILED\", \"timestamp\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\"}" > "$test_results_file"
        return 1
    fi
}

# Run all tests
run_all_tests() {
    log_info "Starting E2E test suite..."
    
    local configs=($(discover_configs))
    local total_configs=${#configs[@]}
    local passed=0
    local failed=0
    
    log_info "Found $total_configs configurations to test"
    
    for config in "${configs[@]}"; do
        if run_tests_for_config "$config"; then
            ((passed++))
        else
            ((failed++))
        fi
    done
    
    # Generate summary report
    local summary_file="$RESULTS_DIR/test_summary.txt"
    {
        echo "E2E Test Summary"
        echo "================"
        echo "Total configurations: $total_configs"
        echo "Passed: $passed"
        echo "Failed: $failed"
        echo "Success rate: $((passed * 100 / total_configs))%"
        echo ""
        echo "Test parameters:"
        echo "- Stress operations: $STRESS_OPERATIONS"
        echo "- Latency operations: $LATENCY_OPERATIONS"
        echo "- Fault injection operations: $FAULT_OPERATIONS"
        echo "- Concurrent clients: $CONCURRENT_CLIENTS"
        echo ""
        echo "Results directory: $RESULTS_DIR"
        echo "Logs directory: $LOG_DIR"
    } > "$summary_file"
    
    log_info "Test summary written to: $summary_file"
    
    if [[ $failed -eq 0 ]]; then
        log_success "All tests passed! ($passed/$total_configs)"
        return 0
    else
        log_error "Some tests failed! ($failed/$total_configs failed)"
        return 1
    fi
}

# Run specific test types
run_test_type() {
    local test_type="$1"
    local configs=($(discover_configs))
    
    log_info "Running $test_type tests for all configurations..."
    
    for config in "${configs[@]}"; do
        log_info "Running $test_type test for configuration: $config"
        cd "$SCRIPT_DIR"
        if "$SCRIPT_DIR/venv/bin/python" -m pytest "test_matrix.py::test_${test_type}" \
            -v \
            --tb=short; then
            log_success "$test_type test passed for $config"
        else
            log_error "$test_type test failed for $config"
        fi
    done
}

# Main function
main() {
    log_info "Starting E2E test runner..."
    log_info "Project root: $PROJECT_ROOT"
    log_info "Script directory: $SCRIPT_DIR"
    log_info "Build directory: $BUILD_DIR"
    
    # Check dependencies
    check_dependencies
    
    # Build server
    build_server
    
    # Parse command line arguments
    case "${1:-all}" in
        "all")
            run_all_tests
            ;;
        "basic")
            run_test_type "basic_functionality"
            ;;
        "stress")
            run_test_type "stress_and_latency"
            ;;
        "fault")
            run_test_type "fault_injection"
            ;;
        "concurrent")
            run_test_type "concurrent_clients"
            ;;
        "help"|"-h"|"--help")
            echo "Usage: $0 [test_type]"
            echo ""
            echo "Test types:"
            echo "  all        - Run all tests (default)"
            echo "  basic      - Run basic functionality tests"
            echo "  stress     - Run stress and latency tests"
            echo "  fault      - Run fault injection tests"
            echo "  concurrent - Run concurrent client tests"
            echo "  help       - Show this help message"
            echo ""
            echo "Environment variables:"
            echo "  STRESS_OPERATIONS     - Number of operations for stress tests (default: 1000)"
            echo "  LATENCY_OPERATIONS    - Number of operations for latency tests (default: 100)"
            echo "  FAULT_OPERATIONS      - Number of operations for fault injection tests (default: 50)"
            echo "  CONCURRENT_CLIENTS    - Number of concurrent clients (default: 5)"
            ;;
        *)
            log_error "Unknown test type: $1"
            echo "Use '$0 help' for usage information"
            exit 1
            ;;
    esac
}

# Run main function
main "$@" 