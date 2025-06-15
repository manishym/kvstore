package main

import (
	"flag"
	"fmt"
	"log"

	"github.com/yourusername/SpdkKeyValueStore/tests/benchmark"
)

func main() {
	// Parse command line flags
	serverAddr := flag.String("server", "localhost:50051", "gRPC server address")
	concurrency := flag.Int("concurrency", 50, "Number of concurrent requests")
	totalRequests := flag.Int("requests", 1000, "Total number of requests")
	protoFile := flag.String("proto", "../../proto/kvstore.proto", "Path to proto file")
	tag := flag.String("tag", "", "Tag to identify this benchmark run")
	flag.Parse()

	// Create benchmark configuration
	config := benchmark.Config{
		ServerAddress: *serverAddr,
		Concurrency:   *concurrency,
		TotalRequests: *totalRequests,
		ProtoFile:     *protoFile,
		Tag:           *tag,
	}

	// Run benchmarks
	if err := benchmark.RunBenchmarks(config); err != nil {
		log.Fatalf("Benchmark failed: %v", err)
	}

	fmt.Println("Benchmarks completed. Check the results directory for detailed results.")
}
