package main

import (
	"encoding/base64"
	"encoding/csv"
	"flag"
	"fmt"
	"log"
	"math/rand"
	"os"
	"sync"
	"time"

	"github.com/bojand/ghz/runner"
)

var (
	concurrency = flag.Int("concurrency", 100, "Number of concurrent connections")
	total       = flag.Int("requests", 10000, "Total number of requests")
	valueSize   = flag.Int("value-size", 1024, "Size of value in bytes")
	keyCount    = flag.Int("key-count", 10000, "Number of keys to prepopulate")
	getPct      = flag.Int("get", 50, "Percentage of Get requests")
	putPct      = flag.Int("put", 30, "Percentage of Put requests")
	deletePct   = flag.Int("delete", 20, "Percentage of Delete requests")
	protoPath   = flag.String("proto", "kvstore.proto", "Path to proto file")
	protoRoot   = flag.String("proto-root", ".", "Import root for proto")
	serverAddr  = flag.String("server", "localhost:50051", "gRPC server address")
	csvOutput   = flag.String("csv", "benchmark.csv", "Output CSV file path")
)

var keyPool []string

type Result struct {
	Method    string
	LatencyMs float64
	Error     error
}

func randomBytes(n int) string {
	b := make([]byte, n)
	rand.Read(b)
	return base64.StdEncoding.EncodeToString(b)
}

func pickKey() string {
	return keyPool[rand.Intn(len(keyPool))]
}

func generateMixedRequest() (string, map[string]interface{}) {
	op := rand.Intn(100)
	switch {
	case op < *getPct:
		return "kvstore.KeyValueStore.Get", map[string]interface{}{
			"key": pickKey(),
		}
	case op < *getPct+*putPct:
		return "kvstore.KeyValueStore.Put", map[string]interface{}{
			"key":   pickKey(),
			"value": randomBytes(*valueSize),
		}
	default:
		return "kvstore.KeyValueStore.Delete", map[string]interface{}{
			"key": pickKey(),
		}
	}
}

func preloadKeys() {
	log.Println("Preloading keys...")
	for _, k := range keyPool {
		_, err := runner.Run(
			"kvstore.KeyValueStore.Put",
			*serverAddr,
			runner.WithProtoFile(*protoPath, []string{*protoRoot}),
			runner.WithTotalRequests(1),
			runner.WithData(map[string]interface{}{
				"key":   k,
				"value": randomBytes(*valueSize / 2),
			}),
		)
		if err != nil {
			log.Fatalf("Preload failed: %v", err)
		}
	}
	log.Println("Preload complete.")
}

func worker(wg *sync.WaitGroup, jobs <-chan int, results chan<- Result) {
	defer wg.Done()
	for range jobs {
		method, data := generateMixedRequest()
		start := time.Now()
		_, err := runner.Run(
			method,
			*serverAddr,
			runner.WithProtoFile(*protoPath, []string{*protoRoot}),
			runner.WithTotalRequests(1),
			runner.WithData(data),
			runner.WithInsecure(true),
			runner.WithTimeout(10*time.Second),
		)
		latency := time.Since(start).Seconds() * 1000 // ms
		if err != nil {
			results <- Result{Method: method, LatencyMs: latency, Error: err}
		} else {
			results <- Result{Method: method, LatencyMs: latency, Error: nil}
		}
	}
}

func writeCSV(filePath string, allResults []Result) error {
	f, err := os.Create(filePath)
	if err != nil {
		return err
	}
	defer f.Close()

	w := csv.NewWriter(f)
	defer w.Flush()

	w.Write([]string{"Method", "LatencyMs", "Error"})
	for _, r := range allResults {
		errStr := ""
		if r.Error != nil {
			errStr = r.Error.Error()
		}
		w.Write([]string{
			r.Method,
			fmt.Sprintf("%.2f", r.LatencyMs),
			errStr,
		})
	}
	return nil
}

func main() {
	flag.Parse()
	rand.Seed(time.Now().UnixNano())

	if *getPct+*putPct+*deletePct != 100 {
		log.Fatalf("Get, Put, and Delete percentages must sum to 100")
	}

	for i := 0; i < *keyCount; i++ {
		keyPool = append(keyPool, randomBytes(8))
	}

	preloadKeys()

	log.Println("Running benchmark with mixed RPCs...")
	jobs := make(chan int, *total)
	results := make(chan Result, *total)

	var wg sync.WaitGroup
	for i := 0; i < *concurrency; i++ {
		wg.Add(1)
		go worker(&wg, jobs, results)
	}

	for i := 0; i < *total; i++ {
		jobs <- i
	}
	close(jobs)
	wg.Wait()
	close(results)

	all := []Result{}
	for r := range results {
		all = append(all, r)
	}

	log.Println("Writing CSV output to:", *csvOutput)
	if err := writeCSV(*csvOutput, all); err != nil {
		log.Fatalf("Failed to write CSV: %v", err)
	}

	fmt.Println("Benchmark complete. CSV output saved to", *csvOutput)
}
