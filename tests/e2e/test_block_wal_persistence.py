import json
import os
import subprocess
import time
import grpc
import tempfile

from conftest import wait_for_port

import kvstore_pb2
import kvstore_pb2_grpc



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


def test_block_device_wal_persistence(tmp_path, server_port):
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
    assert stub.Put(kvstore_pb2.PutRequest(key=b"persist", value=b"value")).success
    channel.close()
    proc.terminate()
    proc.wait()
    time.sleep(1)

    proc = start_server(server_path, str(config_path), server_port)
    channel = grpc.insecure_channel(f"localhost:{server_port}")
    stub = kvstore_pb2_grpc.KeyValueStoreStub(channel)
    resp = stub.Get(kvstore_pb2.GetRequest(key=b"persist"))
    assert resp.found
    assert resp.value == b"value"
    channel.close()
    proc.terminate()
    proc.wait()
