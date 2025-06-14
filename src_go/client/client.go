package client

import (
	"context"
	"fmt"
	"time"

	pb "kvstore/proto"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

type KeyValueClient struct {
	conn   *grpc.ClientConn
	client pb.KeyValueStoreClient
}

func NewKeyValueClient(serverAddr string) (*KeyValueClient, error) {
	conn, err := grpc.Dial(serverAddr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		return nil, fmt.Errorf("failed to connect: %v", err)
	}

	return &KeyValueClient{
		conn:   conn,
		client: pb.NewKeyValueStoreClient(conn),
	}, nil
}

func (c *KeyValueClient) Close() error {
	return c.conn.Close()
}

func (c *KeyValueClient) Put(key, value []byte) error {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	resp, err := c.client.Put(ctx, &pb.PutRequest{
		Key:   key,
		Value: value,
	})
	if err != nil {
		return err
	}
	if !resp.Success {
		return fmt.Errorf("put failed: %s", resp.Error)
	}
	return nil
}

func (c *KeyValueClient) Get(key []byte) ([]byte, bool, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	resp, err := c.client.Get(ctx, &pb.GetRequest{
		Key: key,
	})
	if err != nil {
		return nil, false, err
	}
	if resp.Error != "" {
		return nil, false, fmt.Errorf("get failed: %s", resp.Error)
	}
	return resp.Value, resp.Found, nil
}

func (c *KeyValueClient) Delete(key []byte) error {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	resp, err := c.client.Delete(ctx, &pb.DeleteRequest{
		Key: key,
	})
	if err != nil {
		return err
	}
	if !resp.Success {
		return fmt.Errorf("delete failed: %s", resp.Error)
	}
	return nil
}
