#!/bin/bash

set -e  # Exit on error

# Go to project root
dir="$(dirname "$0")/../.."
cd "$dir"

# Create build directory for unit tests
mkdir -p tests/unit/build
cd tests/unit/build

# Configure with coverage
cmake -DCMAKE_BUILD_TYPE=Coverage ../

# Build
make -j$(nproc)

# Run tests
./kvstore_tests

# Check if lcov is available
if command -v lcov &> /dev/null; then
    # Generate coverage report
    lcov --capture --directory . --output-file coverage.info
    lcov --remove coverage.info '/usr/*' --output-file coverage.info
    lcov --list coverage.info

    # Check if genhtml is available
    if command -v genhtml &> /dev/null; then
        # Generate HTML report
        genhtml coverage.info --output-directory coverage_report
        echo "Coverage report generated in build/coverage_report/index.html"
    else
        echo "genhtml not found. Skipping HTML report generation."
        echo "Coverage data saved in build/coverage.info"
    fi
else
    echo "lcov not found. Skipping coverage report generation."
    echo "Tests completed successfully."
fi 