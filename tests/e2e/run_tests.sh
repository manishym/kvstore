#!/bin/bash

# Exit on error
set -e

# Create and activate virtual environment
python3 -m venv venv
source venv/bin/activate

# Install dependencies
pip install -r requirements.txt

# Generate gRPC Python code
python -m grpc_tools.protoc -I../../proto \
    --python_out=. \
    --grpc_python_out=. \
    ../../proto/kvstore.proto

# Run tests
pytest -v test_kvstore.py

# Deactivate virtual environment
deactivate 