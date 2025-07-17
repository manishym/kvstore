#!/usr/bin/env python3
"""
Comprehensive E2E test matrix for Key-Value Store.

This module provides:
1. Dynamic configuration discovery from configs/ directory
2. Stress and latency benchmarking for each config combination
3. Fault injection testing (server kill/recovery)
4. Proper logging and error capture
5. Temporary directory management for WAL/log outputs
"""

import os
import json
import glob
import pytest
import time
import threading
import subprocess
import signal
import tempfile
import shutil
import logging
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from concurrent.futures import ThreadPoolExecutor, as_completed
import grpc
import kvstore_pb2
import kvstore_pb2_grpc

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# Constants
CONFIGS_DIR = Path(__file__).parent / "configs"
BUILD_DIR = Path(__file__).parent.parent.parent / "tests" / "unit" / "build"
SERVER_PATH = BUILD_DIR / "server"

# Test parameters
STRESS_TEST_OPERATIONS = 1000
LATENCY_TEST_OPERATIONS = 100
FAULT_INJECTION_OPERATIONS = 50
CONCURRENT_CLIENTS = 5


def discover_configs() -> List[Tuple[str, Path]]:
    """Discover all configuration files in the configs directory."""
    configs = []
    for config_file in CONFIGS_DIR.glob("*.json"):
        config_name = config_file.stem
        configs.append((config_name, config_file))
    
    logger.info(f"Discovered {len(configs)} configuration files: {[name for name, _ in configs]}")
    return configs


def load_config(config_path: Path) -> Dict:
    """Load and validate a configuration file."""
    with open(config_path, 'r') as f:
        config = json.load(f)
    
    # Ensure required fields exist
    if "map_type" not in config:
        raise ValueError(f"Config {config_path} missing 'map_type' field")
    if "wal" not in config:
        raise ValueError(f"Config {config_path} missing 'wal' field")
    
    return config


def build_server_if_needed():
    """Build the server if it doesn't exist."""
    if not SERVER_PATH.exists():
        logger.info("Building server...")
        base_dir = Path(__file__).parent.parent.parent
        subprocess.run(["cmake", "-B", str(BUILD_DIR), "-S", str(base_dir)], check=True)
        subprocess.run(["cmake", "--build", str(BUILD_DIR)], check=True)
        logger.info("Server built successfully")


def create_temp_config(base_config: Dict, temp_dir: Path, port: int) -> Path:
    """Create a temporary configuration file with server address."""
    config = base_config.copy()
    config["address"] = f"0.0.0.0:{port}"
    
    # Update WAL device path to use temp directory if it's a file path
    if "wal" in config and "device" in config["wal"]:
        if isinstance(config["wal"]["device"], str) and not config["wal"]["device"].startswith("/"):
            config["wal"]["device"] = str(temp_dir / config["wal"]["device"])
    
    config_path = temp_dir / "runtime_config.json"
    with open(config_path, 'w') as f:
        json.dump(config, f, indent=2)
    
    return config_path


def wait_for_port(port: int, host: str = "localhost", timeout: float = 10.0) -> bool:
    """Wait for a port to be available."""
    import socket
    start = time.time()
    while time.time() - start < timeout:
        try:
            with socket.create_connection((host, port), timeout=1):
                return True
        except OSError:
            time.sleep(0.1)
    return False


def wait_for_port_close(port: int, host: str = "localhost", timeout: float = 10.0) -> bool:
    """Wait for a port to become unavailable."""
    import socket
    start = time.time()
    while time.time() - start < timeout:
        try:
            with socket.create_connection((host, port), timeout=1):
                time.sleep(0.1)
        except OSError:
            return True
    return False


class ServerManager:
    """Manages server lifecycle with proper logging and cleanup."""
    
    def __init__(self, config_path: Path, port: int, temp_dir: Path):
        self.config_path = config_path
        self.port = port
        self.temp_dir = temp_dir
        self.process: Optional[subprocess.Popen] = None
        self.stdout_log = temp_dir / "server_stdout.log"
        self.stderr_log = temp_dir / "server_stderr.log"
        
        # Setup log files
        self.stdout_file = open(self.stdout_log, 'w')
        self.stderr_file = open(self.stderr_log, 'w')
    
    def start(self) -> bool:
        """Start the server and wait for it to be ready."""
        try:
            self.process = subprocess.Popen(
                [str(SERVER_PATH), "--config", str(self.config_path)],
                stdout=self.stdout_file,
                stderr=self.stderr_file,
                text=True,
                bufsize=1
            )
            
            if wait_for_port(self.port, timeout=15):
                logger.info(f"Server started successfully on port {self.port}")
                return True
            else:
                logger.error(f"Server failed to start on port {self.port}")
                return False
        except Exception as e:
            logger.error(f"Failed to start server: {e}")
            return False
    
    def stop(self):
        """Stop the server gracefully."""
        if self.process:
            logger.info("Stopping server...")
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                logger.warning("Server didn't terminate gracefully, killing...")
                self.process.kill()
                self.process.wait()
            self.process = None
    
    def kill(self):
        """Kill the server abruptly (for fault injection)."""
        if self.process:
            logger.info("Killing server abruptly...")
            self.process.kill()
            self.process.wait()
            self.process = None
    
    def get_logs(self) -> Tuple[str, str]:
        """Get server logs."""
        self.stdout_file.flush()
        self.stderr_file.flush()
        
        with open(self.stdout_log, 'r') as f:
            stdout = f.read()
        with open(self.stderr_log, 'r') as f:
            stderr = f.read()
        
        return stdout, stderr
    
    def cleanup(self):
        """Clean up resources."""
        self.stop()
        self.stdout_file.close()
        self.stderr_file.close()


class KVStoreClient:
    """Client for interacting with the KV store."""
    
    def __init__(self, port: int):
        self.port = port
        self.channel = grpc.insecure_channel(f'localhost:{port}')
        self.stub = kvstore_pb2_grpc.KeyValueStoreStub(self.channel)
    
    def put(self, key: bytes, value: bytes) -> bool:
        """Put a key-value pair."""
        try:
            request = kvstore_pb2.PutRequest(key=key, value=value)
            response = self.stub.Put(request)
            return response.success
        except Exception as e:
            logger.error(f"Put failed: {e}")
            return False
    
    def get(self, key: bytes) -> Tuple[bool, Optional[bytes]]:
        """Get a value by key."""
        try:
            request = kvstore_pb2.GetRequest(key=key)
            response = self.stub.Get(request)
            return response.found, response.value if response.found else None
        except Exception as e:
            logger.error(f"Get failed: {e}")
            return False, None
    
    def delete(self, key: bytes) -> bool:
        """Delete a key-value pair."""
        try:
            request = kvstore_pb2.DeleteRequest(key=key)
            response = self.stub.Delete(request)
            return response.success
        except Exception as e:
            logger.error(f"Delete failed: {e}")
            return False
    
    def close(self):
        """Close the gRPC channel."""
        self.channel.close()


def run_basic_functionality_test(client: KVStoreClient) -> bool:
    """Run basic functionality tests."""
    logger.info("Running basic functionality tests...")
    
    # Test put and get
    key = b"test_key"
    value = b"test_value"
    
    if not client.put(key, value):
        logger.error("Put operation failed")
        return False
    
    found, retrieved_value = client.get(key)
    if not found or retrieved_value != value:
        logger.error("Get operation failed or returned wrong value")
        return False
    
    # Test delete
    if not client.delete(key):
        logger.error("Delete operation failed")
        return False
    
    found, _ = client.get(key)
    if found:
        logger.error("Key still exists after delete")
        return False
    
    logger.info("Basic functionality tests passed")
    return True


def run_stress_test(client: KVStoreClient, operations: int = STRESS_TEST_OPERATIONS) -> Dict:
    """Run stress test with concurrent operations."""
    logger.info(f"Running stress test with {operations} operations...")
    
    results = {
        "total_operations": operations,
        "successful_puts": 0,
        "successful_gets": 0,
        "successful_deletes": 0,
        "failed_operations": 0,
        "start_time": time.time(),
        "end_time": None
    }
    
    # Phase 1: Put operations
    for i in range(operations):
        key = f"stress_key_{i}".encode()
        value = f"stress_value_{i}".encode()
        if client.put(key, value):
            results["successful_puts"] += 1
        else:
            results["failed_operations"] += 1
    
    # Phase 2: Get operations
    for i in range(operations):
        key = f"stress_key_{i}".encode()
        found, value = client.get(key)
        if found:
            results["successful_gets"] += 1
        else:
            results["failed_operations"] += 1
    
    # Phase 3: Delete operations
    for i in range(operations):
        key = f"stress_key_{i}".encode()
        if client.delete(key):
            results["successful_deletes"] += 1
        else:
            results["failed_operations"] += 1
    
    results["end_time"] = time.time()
    results["duration"] = results["end_time"] - results["start_time"]
    results["throughput"] = operations / results["duration"]
    
    logger.info(f"Stress test completed: {results['successful_puts']}/{operations} puts, "
                f"{results['successful_gets']}/{operations} gets, "
                f"{results['successful_deletes']}/{operations} deletes")
    
    return results


def run_latency_test(client: KVStoreClient, operations: int = LATENCY_TEST_OPERATIONS) -> Dict:
    """Run latency test with timing measurements."""
    logger.info(f"Running latency test with {operations} operations...")
    
    latencies = {
        "put_latencies": [],
        "get_latencies": [],
        "delete_latencies": []
    }
    
    # Measure put latencies
    for i in range(operations):
        key = f"latency_key_{i}".encode()
        value = f"latency_value_{i}".encode()
        
        start_time = time.time()
        success = client.put(key, value)
        end_time = time.time()
        
        if success:
            latencies["put_latencies"].append((end_time - start_time) * 1000)  # Convert to ms
    
    # Measure get latencies
    for i in range(operations):
        key = f"latency_key_{i}".encode()
        
        start_time = time.time()
        found, _ = client.get(key)
        end_time = time.time()
        
        if found:
            latencies["get_latencies"].append((end_time - start_time) * 1000)  # Convert to ms
    
    # Measure delete latencies
    for i in range(operations):
        key = f"latency_key_{i}".encode()
        
        start_time = time.time()
        success = client.delete(key)
        end_time = time.time()
        
        if success:
            latencies["delete_latencies"].append((end_time - start_time) * 1000)  # Convert to ms
    
    # Calculate statistics
    results = {
        "put_avg_latency": sum(latencies["put_latencies"]) / len(latencies["put_latencies"]) if latencies["put_latencies"] else 0,
        "get_avg_latency": sum(latencies["get_latencies"]) / len(latencies["get_latencies"]) if latencies["get_latencies"] else 0,
        "delete_avg_latency": sum(latencies["delete_latencies"]) / len(latencies["delete_latencies"]) if latencies["delete_latencies"] else 0,
        "put_p95_latency": sorted(latencies["put_latencies"])[int(len(latencies["put_latencies"]) * 0.95)] if latencies["put_latencies"] else 0,
        "get_p95_latency": sorted(latencies["get_latencies"])[int(len(latencies["get_latencies"]) * 0.95)] if latencies["get_latencies"] else 0,
        "delete_p95_latency": sorted(latencies["delete_latencies"])[int(len(latencies["delete_latencies"]) * 0.95)] if latencies["delete_latencies"] else 0,
    }
    
    logger.info(f"Latency test completed: Put avg={results['put_avg_latency']:.2f}ms, "
                f"Get avg={results['get_avg_latency']:.2f}ms, "
                f"Delete avg={results['delete_avg_latency']:.2f}ms")
    
    return results


def run_fault_injection_test(server_manager: ServerManager, client: KVStoreClient, 
                            operations: int = FAULT_INJECTION_OPERATIONS) -> Dict:
    """Run fault injection test by killing server mid-operation."""
    logger.info(f"Running fault injection test with {operations} operations...")
    
    results = {
        "operations_before_crash": 0,
        "operations_after_recovery": 0,
        "recovery_successful": False,
        "data_consistency": False
    }
    
    # Phase 1: Put some data
    test_data = {}
    for i in range(operations):
        key = f"fault_key_{i}".encode()
        value = f"fault_value_{i}".encode()
        if client.put(key, value):
            test_data[key] = value
            results["operations_before_crash"] += 1
    
    # Phase 2: Kill server abruptly
    logger.info("Killing server abruptly...")
    server_manager.kill()
    
    # Phase 3: Restart server
    logger.info("Restarting server...")
    if not server_manager.start():
        logger.error("Failed to restart server after crash")
        return results
    
    # Create new client for restarted server
    new_client = KVStoreClient(server_manager.port)
    
    # Phase 4: Verify data consistency
    consistent_count = 0
    for key, expected_value in test_data.items():
        found, retrieved_value = new_client.get(key)
        if found and retrieved_value == expected_value:
            consistent_count += 1
            results["operations_after_recovery"] += 1
    
    results["recovery_successful"] = True
    results["data_consistency"] = consistent_count == len(test_data)
    results["consistency_ratio"] = consistent_count / len(test_data) if test_data else 0
    
    new_client.close()
    
    logger.info(f"Fault injection test completed: {consistent_count}/{len(test_data)} keys recovered correctly")
    
    return results


def run_concurrent_test(port: int, num_clients: int = CONCURRENT_CLIENTS, 
                       operations_per_client: int = 100) -> Dict:
    """Run concurrent client test."""
    logger.info(f"Running concurrent test with {num_clients} clients, {operations_per_client} ops each...")
    
    def client_worker(client_id: int) -> Dict:
        client = KVStoreClient(port)
        results = {"client_id": client_id, "successful_ops": 0, "failed_ops": 0}
        
        for i in range(operations_per_client):
            key = f"concurrent_key_{client_id}_{i}".encode()
            value = f"concurrent_value_{client_id}_{i}".encode()
            
            if client.put(key, value):
                results["successful_ops"] += 1
            else:
                results["failed_ops"] += 1
        
        client.close()
        return results
    
    # Run concurrent clients
    with ThreadPoolExecutor(max_workers=num_clients) as executor:
        futures = [executor.submit(client_worker, i) for i in range(num_clients)]
        client_results = [future.result() for future in as_completed(futures)]
    
    # Aggregate results
    total_successful = sum(r["successful_ops"] for r in client_results)
    total_failed = sum(r["failed_ops"] for r in client_results)
    
    results = {
        "num_clients": num_clients,
        "operations_per_client": operations_per_client,
        "total_operations": num_clients * operations_per_client,
        "successful_operations": total_successful,
        "failed_operations": total_failed,
        "success_rate": total_successful / (total_successful + total_failed) if (total_successful + total_failed) > 0 else 0
    }
    
    logger.info(f"Concurrent test completed: {total_successful}/{results['total_operations']} operations successful")
    
    return results


# Dynamic test generation
def generate_test_params():
    """Generate test parameters for pytest parametrize."""
    configs = discover_configs()
    test_params = []
    
    for config_name, config_path in configs:
        test_params.append((config_name, config_path))
    
    return test_params


# Main test functions
@pytest.mark.parametrize("config_name,config_path", generate_test_params())
def test_basic_functionality(config_name: str, config_path: Path):
    """Test basic functionality for a specific configuration."""
    logger.info(f"Testing basic functionality with config: {config_name}")
    
    # Setup
    temp_dir = Path(tempfile.mkdtemp())
    port = 50051
    
    try:
        # Build server if needed
        build_server_if_needed()
        
        # Load and create config
        base_config = load_config(config_path)
        temp_config_path = create_temp_config(base_config, temp_dir, port)
        
        # Start server
        server_manager = ServerManager(temp_config_path, port, temp_dir)
        if not server_manager.start():
            pytest.fail("Failed to start server")
        
        # Run tests
        client = KVStoreClient(port)
        success = run_basic_functionality_test(client)
        client.close()
        
        if not success:
            pytest.fail("Basic functionality test failed")
        
        # Get logs for debugging
        stdout, stderr = server_manager.get_logs()
        if stderr:
            logger.warning(f"Server stderr: {stderr}")
        
    finally:
        server_manager.cleanup()
        shutil.rmtree(temp_dir, ignore_errors=True)


@pytest.mark.parametrize("config_name,config_path", generate_test_params())
def test_stress_and_latency(config_name: str, config_path: Path):
    """Test stress and latency for a specific configuration."""
    logger.info(f"Testing stress and latency with config: {config_name}")
    
    # Setup
    temp_dir = Path(tempfile.mkdtemp())
    port = 50052
    
    try:
        # Build server if needed
        build_server_if_needed()
        
        # Load and create config
        base_config = load_config(config_path)
        temp_config_path = create_temp_config(base_config, temp_dir, port)
        
        # Start server
        server_manager = ServerManager(temp_config_path, port, temp_dir)
        if not server_manager.start():
            pytest.fail("Failed to start server")
        
        # Run tests
        client = KVStoreClient(port)
        
        # Stress test
        stress_results = run_stress_test(client)
        assert stress_results["successful_puts"] > 0, "No successful put operations"
        assert stress_results["successful_gets"] > 0, "No successful get operations"
        assert stress_results["successful_deletes"] > 0, "No successful delete operations"
        
        # Latency test
        latency_results = run_latency_test(client)
        assert latency_results["put_avg_latency"] > 0, "Invalid put latency"
        assert latency_results["get_avg_latency"] > 0, "Invalid get latency"
        assert latency_results["delete_avg_latency"] > 0, "Invalid delete latency"
        
        client.close()
        
        # Log results
        logger.info(f"Stress results for {config_name}: {stress_results}")
        logger.info(f"Latency results for {config_name}: {latency_results}")
        
    finally:
        server_manager.cleanup()
        shutil.rmtree(temp_dir, ignore_errors=True)


@pytest.mark.parametrize("config_name,config_path", generate_test_params())
def test_fault_injection(config_name: str, config_path: Path):
    """Test fault injection and recovery for a specific configuration."""
    logger.info(f"Testing fault injection with config: {config_name}")
    
    # Setup
    temp_dir = Path(tempfile.mkdtemp())
    port = 50053
    
    try:
        # Build server if needed
        build_server_if_needed()
        
        # Load and create config
        base_config = load_config(config_path)
        temp_config_path = create_temp_config(base_config, temp_dir, port)
        
        # Start server
        server_manager = ServerManager(temp_config_path, port, temp_dir)
        if not server_manager.start():
            pytest.fail("Failed to start server")
        
        # Run fault injection test
        client = KVStoreClient(port)
        fault_results = run_fault_injection_test(server_manager, client)
        client.close()
        
        # Assertions
        assert fault_results["recovery_successful"], "Server failed to recover after crash"
        assert fault_results["data_consistency"], "Data consistency check failed after recovery"
        assert fault_results["operations_before_crash"] > 0, "No operations completed before crash"
        assert fault_results["operations_after_recovery"] > 0, "No operations recovered after restart"
        
        # Log results
        logger.info(f"Fault injection results for {config_name}: {fault_results}")
        
    finally:
        server_manager.cleanup()
        shutil.rmtree(temp_dir, ignore_errors=True)


@pytest.mark.parametrize("config_name,config_path", generate_test_params())
def test_concurrent_clients(config_name: str, config_path: Path):
    """Test concurrent client access for a specific configuration."""
    logger.info(f"Testing concurrent clients with config: {config_name}")
    
    # Setup
    temp_dir = Path(tempfile.mkdtemp())
    port = 50054
    
    try:
        # Build server if needed
        build_server_if_needed()
        
        # Load and create config
        base_config = load_config(config_path)
        temp_config_path = create_temp_config(base_config, temp_dir, port)
        
        # Start server
        server_manager = ServerManager(temp_config_path, port, temp_dir)
        if not server_manager.start():
            pytest.fail("Failed to start server")
        
        # Run concurrent test
        concurrent_results = run_concurrent_test(port)
        
        # Assertions
        assert concurrent_results["successful_operations"] > 0, "No successful concurrent operations"
        assert concurrent_results["success_rate"] > 0.8, f"Success rate too low: {concurrent_results['success_rate']}"
        
        # Log results
        logger.info(f"Concurrent test results for {config_name}: {concurrent_results}")
        
    finally:
        server_manager.cleanup()
        shutil.rmtree(temp_dir, ignore_errors=True)


if __name__ == "__main__":
    # Run tests directly if script is executed
    pytest.main([__file__, "-v", "--tb=short"]) 