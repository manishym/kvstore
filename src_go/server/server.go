package server

import (
	"context"
	"fmt"
	"log"
	"net"
	"sync"

	pb "kvstore/proto"

	"google.golang.org/grpc"
)

type KeyValueServer struct {
	pb.UnimplementedKeyValueStoreServer
	store sync.Map
}

func NewKeyValueServer() *KeyValueServer {
	return &KeyValueServer{}
}

func (s *KeyValueServer) Put(ctx context.Context, req *pb.PutRequest) (*pb.PutResponse, error) {
	s.store.Store(string(req.Key), req.Value)
	return &pb.PutResponse{Success: true}, nil
}

func (s *KeyValueServer) Get(ctx context.Context, req *pb.GetRequest) (*pb.GetResponse, error) {
	value, ok := s.store.Load(string(req.Key))
	if !ok {
		return &pb.GetResponse{Found: false}, nil
	}

	return &pb.GetResponse{
		Value: value.([]byte),
		Found: true,
	}, nil
}

func (s *KeyValueServer) Delete(ctx context.Context, req *pb.DeleteRequest) (*pb.DeleteResponse, error) {
	_, ok := s.store.LoadAndDelete(string(req.Key))
	return &pb.DeleteResponse{Success: ok}, nil
}

func RunServer(port int) error {
	lis, err := net.Listen("tcp", fmt.Sprintf(":%d", port))
	if err != nil {
		return fmt.Errorf("failed to listen: %v", err)
	}

	s := grpc.NewServer()
	pb.RegisterKeyValueStoreServer(s, NewKeyValueServer())

	log.Printf("Server listening on port %d", port)
	if err := s.Serve(lis); err != nil {
		return fmt.Errorf("failed to serve: %v", err)
	}

	return nil
}
