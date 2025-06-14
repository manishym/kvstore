package main

import (
	"flag"
	"fmt"
	"log"

	"kvstore/client"
)

func main() {
	serverAddr := flag.String("server", "localhost:50051", "The server address")
	flag.Parse()

	c, err := client.NewKeyValueClient(*serverAddr)
	if err != nil {
		log.Fatalf("Failed to create client: %v", err)
	}
	defer c.Close()

	// Example usage
	key := []byte("test-key")
	value := []byte("test-value")

	// Put
	if err := c.Put(key, value); err != nil {
		log.Fatalf("Failed to put: %v", err)
	}
	fmt.Printf("Put key: %s\n", string(key))

	// Get
	retrieved, found, err := c.Get(key)
	if err != nil {
		log.Fatalf("Failed to get: %v", err)
	}
	if found {
		fmt.Printf("Got value: %s\n", string(retrieved))
	} else {
		fmt.Printf("Key not found: %s\n", string(key))
	}

	// Delete
	if err := c.Delete(key); err != nil {
		log.Fatalf("Failed to delete: %v", err)
	}
	fmt.Printf("Deleted key: %s\n", string(key))
}
