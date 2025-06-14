package main

import (
	"flag"
	"log"

	"kvstore/server"
)

func main() {
	port := flag.Int("port", 50051, "The server port")
	flag.Parse()

	if err := server.RunServer(*port); err != nil {
		log.Fatalf("Failed to run server: %v", err)
	}
}
