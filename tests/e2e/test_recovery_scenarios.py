import json
import os
import struct
import subprocess
import time
import socket
import grpc

import kvstore_pb2
import kvstore_pb2_grpc
from conftest import wait_for_port, wait_for_port_close

def wait_until_server_ready(port, timeout=10.0):
    """Wait until we can connect to gRPC server on given port."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("localhost", port), timeout=0.5):
                return True
        except (ConnectionRefusedError, socket.timeout):
            time.sleep(0.1)  # Small, consistent polling interval
    return False

def build_server():
    base_dir = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
    build_dir = os.path.join(base_dir, "tests", "unit", "build")
    server_path = os.path.join(build_dir, "server")
    if not os.path.exists(server_path):
        subprocess.run(["cmake", "-B", build_dir, "-S", base_dir], check=True)
        subprocess.run(["cmake", "--build", build_dir], check=True)
    return server_path


def start_server(server_path, config_path, port):
    process = subprocess.Popen(
        [server_path, config_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )
    if not wait_for_port(port, timeout=10):
        out, err = process.communicate(timeout=1)
        process.terminate()
        process.wait()
        raise RuntimeError(f"Server failed to start\nstdout:{out}\nstderr:{err}")
    return process


def test_recover_from_crash(tmp_path, server_port):
    server_path = build_server()
    wal_file = tmp_path / "wal.log"
    config_path = tmp_path / "config.json"
    config = {
        "address": f"0.0.0.0:{server_port}",
        "wal": {"type": "block", "device": str(wal_file)},
    }
    with open(config_path, "w") as f:
        json.dump(config, f)

    proc = start_server(server_path, str(config_path), server_port)
    channel = grpc.insecure_channel(f"localhost:{server_port}")
    stub = kvstore_pb2_grpc.KeyValueStoreStub(channel)
    assert stub.Put(kvstore_pb2.PutRequest(key=b"crash", value=b"value")).success
    channel.close()
    proc.kill()  # simulate crash
    proc.wait()
    assert wait_until_server_ready(server_port)
    proc = start_server(server_path, str(config_path), server_port)
    channel = grpc.insecure_channel(f"localhost:{server_port}")
    stub = kvstore_pb2_grpc.KeyValueStoreStub(channel)
    resp = stub.Get(kvstore_pb2.GetRequest(key=b"crash"))
    assert resp.found
    assert resp.value == b"value"
    channel.close()
    proc.terminate()
    proc.wait()


def test_recover_with_corrupt_wal(tmp_path, server_port):
    server_path = build_server()
    wal_file = tmp_path / "wal_corrupt.log"
    config_path = tmp_path / "config.json"
    config = {
        "address": f"0.0.0.0:{server_port}",
        "wal": {"type": "block", "device": str(wal_file)},
    }
    with open(config_path, "w") as f:
        json.dump(config, f)

    proc = start_server(server_path, str(config_path), server_port)
    channel = grpc.insecure_channel(f"localhost:{server_port}")
    stub = kvstore_pb2_grpc.KeyValueStoreStub(channel)
    assert stub.Put(kvstore_pb2.PutRequest(key=b"good1", value=b"v1")).success
    assert stub.Put(kvstore_pb2.PutRequest(key=b"good2", value=b"v2")).success
    channel.close()
    proc.terminate()
    proc.wait()

    # Corrupt the WAL with a partial entry
    with open(wal_file, "ab") as f:
        f.write(struct.pack("B", 1))
        f.write(struct.pack("<I", 4))
        f.write(struct.pack("<I", 4))
        f.write(b"bad")  # incomplete key

    proc = start_server(server_path, str(config_path), server_port)
    channel = grpc.insecure_channel(f"localhost:{server_port}")
    stub = kvstore_pb2_grpc.KeyValueStoreStub(channel)
    resp1 = stub.Get(kvstore_pb2.GetRequest(key=b"good1"))
    assert resp1.found and resp1.value == b"v1"
    resp2 = stub.Get(kvstore_pb2.GetRequest(key=b"good2"))
    assert resp2.found and resp2.value == b"v2"
    channel.close()
    proc.terminate()
    proc.wait()
