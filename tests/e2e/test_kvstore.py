import pytest
import kvstore_pb2

def test_put_and_get(stub):
    """Test putting a value and then getting it back."""
    # Put a value
    put_request = kvstore_pb2.PutRequest(key=b"test_key", value=b"test_value")
    put_response = stub.Put(put_request)
    assert put_response.success

    # Get the value
    get_request = kvstore_pb2.GetRequest(key=b"test_key")
    get_response = stub.Get(get_request)
    assert get_response.found
    assert get_response.value == b"test_value"

def test_get_nonexistent_key(stub):
    """Test getting a key that doesn't exist."""
    get_request = kvstore_pb2.GetRequest(key=b"nonexistent_key")
    get_response = stub.Get(get_request)
    assert not get_response.found

def test_delete(stub):
    """Test deleting a key-value pair."""
    # First put a value
    put_request = kvstore_pb2.PutRequest(key=b"delete_test_key", value=b"delete_test_value")
    put_response = stub.Put(put_request)
    assert put_response.success

    # Delete the key
    delete_request = kvstore_pb2.DeleteRequest(key=b"delete_test_key")
    delete_response = stub.Delete(delete_request)
    assert delete_response.success

    # Verify the key is deleted
    get_request = kvstore_pb2.GetRequest(key=b"delete_test_key")
    get_response = stub.Get(get_request)
    assert not get_response.found

def test_delete_nonexistent_key(stub):
    """Test deleting a key that doesn't exist."""
    delete_request = kvstore_pb2.DeleteRequest(key=b"nonexistent_key")
    delete_response = stub.Delete(delete_request)
    assert not delete_response.success

def test_concurrent_operations(stub):
    """Test concurrent operations on different keys."""
    # Put multiple values
    for i in range(10):
        key = f"concurrent_key_{i}".encode()
        value = f"concurrent_value_{i}".encode()
        put_request = kvstore_pb2.PutRequest(key=key, value=value)
        put_response = stub.Put(put_request)
        assert put_response.success

    # Get all values
    for i in range(10):
        key = f"concurrent_key_{i}".encode()
        expected_value = f"concurrent_value_{i}".encode()
        get_request = kvstore_pb2.GetRequest(key=key)
        get_response = stub.Get(get_request)
        assert get_response.found
        assert get_response.value == expected_value

    # Delete all values
    for i in range(10):
        key = f"concurrent_key_{i}".encode()
        delete_request = kvstore_pb2.DeleteRequest(key=key)
        delete_response = stub.Delete(delete_request)
        assert delete_response.success

    # Verify all values are deleted
    for i in range(10):
        key = f"concurrent_key_{i}".encode()
        get_request = kvstore_pb2.GetRequest(key=key)
        get_response = stub.Get(get_request)
        assert not get_response.found 