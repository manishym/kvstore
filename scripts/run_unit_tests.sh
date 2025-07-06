#!/bin/bash

set -e  # Exit on error

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "$script_dir/.." && pwd)"
config_dir="$project_root/src/config"
# Go to project root
build_dir="$project_root/build"
# Create build directory for unit tests
rm -rf "$build_dir" && mkdir -p "$build_dir" && cd "$build_dir" && cmake -DCMAKE_BUILD_TYPE=Coverage ..  && make -j$(nproc)

cp $config_dir/runtime_config.json runtime_config.json

./kvstore_tests

# Check if lcov is available
if command -v lcov &> /dev/null; then
    # Generate coverage report
    lcov --capture --directory . --output-file coverage.info --ignore-errors mismatch
    lcov --remove coverage.info '/usr/*' '*/tests/unit*' --output-file coverage.info --ignore-errors unused
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