# Stage 1: Build and run unit tests + collect coverage
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Copy just the dependency installer script first
COPY scripts/install_system_dependencies.sh /tmp/install_system_dependencies.sh

# Install system dependencies early
RUN bash /tmp/install_system_dependencies.sh && rm /tmp/install_system_dependencies.sh

# Create working dir and copy rest of the source
WORKDIR /app
COPY . .

# Build with coverage flags
RUN rm -rf build && mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage" && \
    make -j$(nproc)

# Copy config if required at runtime
RUN cp src/config/runtime_config.json build/runtime_config.json

# Run unit tests and collect coverage
RUN cd build && \
    ctest --output-on-failure && \
    lcov --capture --directory . --output-file coverage.info --ignore-errors mismatch --rc geninfo_unexecuted_blocks=1 && \
    lcov --remove coverage.info '/usr/*' --output-file coverage.info && \
    lcov --list coverage.info > coverage.txt

# Make test script executable
RUN chmod +x tests/e2e/run_tests.sh

# Run E2E + show coverage
CMD echo "✅ Running E2E tests..." && \
    ./tests/e2e/run_tests.sh && \
    echo "📊 Unit Test Coverage Summary:" && \
    cat build/coverage.txt
