import os
import subprocess
import time
import pytest
import grpc
import socket
from concurrent import futures

# Import generated gRPC code
import kvstore_pb2
import kvstore_pb2_grpc

def wait_for_port(port, host="localhost", timeout=10.0):
    """Wait for a port to be available."""
    start = time.time()
    while time.time() - start < timeout:
        try:
            with socket.create_connection((host, port), timeout=1):
                return True
        except OSError:
            time.sleep(0.1)
    return False

@pytest.fixture(scope="session")
def server_port():
    """Return a port number for the server."""
    return 50051

@pytest.fixture(scope="session")
def server_process(server_port):
    """Start the server process and yield the process object."""
    # Build the server if not already built
    base_dir = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
    build_dir = os.path.join(base_dir, "tests", "unit", "build")
    server_path = os.path.join(build_dir, "kvstore_server")
    
    if not os.path.exists(server_path):
        # Run cmake first to generate build files
        subprocess.run(["cmake", "-B", build_dir, "-S", base_dir], check=True)
        # Then build
        subprocess.run(["cmake", "--build", build_dir], check=True)
    
    # Start the server with output redirected to pipes
    process = subprocess.Popen(
        [server_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,  # Use text mode for easier output handling
        bufsize=1   # Line buffered
    )
    
    # Wait for server to start
    if not wait_for_port(server_port, timeout=10):
        # If server didn't start, get its output and raise an error
        stdout, stderr = process.communicate(timeout=1)
        process.terminate()
        process.wait()
        raise RuntimeError(
            f"Server failed to start on port {server_port}!\n"
            f"stdout: {stdout}\n"
            f"stderr: {stderr}"
        )
    
    # Print server output for debugging
    print(f"Server started successfully on port {server_port}")
    
    yield process
    
    # Cleanup
    process.terminate()
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()

@pytest.fixture(scope="session")
def grpc_channel(server_port, server_process):
    """Create a gRPC channel."""
    channel = grpc.insecure_channel(f'localhost:{server_port}')
    yield channel
    channel.close()

@pytest.fixture(scope="session")
def stub(grpc_channel):
    """Create a gRPC stub."""
    return kvstore_pb2_grpc.KeyValueStoreStub(grpc_channel) 