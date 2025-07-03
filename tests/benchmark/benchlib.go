package benchmark

import (
	"encoding/base64"
	"encoding/csv"
	"fmt"
	"log"
	"math/rand"
	"os"
	"path/filepath"
	"sync"
	"time"

	"github.com/bojand/ghz/runner"
)

// Config defines parameters for running the benchmark.
type Config struct {
	ServerAddress string
	Concurrency   int
	TotalRequests int
	ProtoFile     string
	// Tag is appended to the results CSV filename if not empty.
	Tag string

	// Optional tuning knobs with reasonable defaults
	ValueSize int
	KeyCount  int
	GetPct    int
	PutPct    int
	DeletePct int
}

// Result captures latency for a single RPC.
type Result struct {
	Method    string
	LatencyMs float64
	Error     error
}

var keyPool []string

func randomBytes(n int) string {
	b := make([]byte, n)
	rand.Read(b)
	return base64.StdEncoding.EncodeToString(b)
}

func pickKey() string {
	return keyPool[rand.Intn(len(keyPool))]
}

func generateMixedRequest(cfg *Config) (string, map[string]interface{}) {
	op := rand.Intn(100)
	switch {
	case op < cfg.GetPct:
		return "kvstore.KeyValueStore.Get", map[string]interface{}{
			"key": pickKey(),
		}
	case op < cfg.GetPct+cfg.PutPct:
		return "kvstore.KeyValueStore.Put", map[string]interface{}{
			"key":   pickKey(),
			"value": randomBytes(cfg.ValueSize),
		}
	default:
		return "kvstore.KeyValueStore.Delete", map[string]interface{}{
			"key": pickKey(),
		}
	}
}

func preloadKeys(cfg *Config) error {
	log.Println("Preloading keys...")
	for _, k := range keyPool {
		_, err := runner.Run(
			"kvstore.KeyValueStore.Put",
			cfg.ServerAddress,
			runner.WithProtoFile(cfg.ProtoFile, []string{filepath.Dir(cfg.ProtoFile)}),
			runner.WithTotalRequests(1),
			runner.WithData(map[string]interface{}{
				"key":   k,
				"value": randomBytes(cfg.ValueSize / 2),
			}),
			runner.WithInsecure(true),
			runner.WithTimeout(10*time.Second),
		)
		if err != nil {
			return fmt.Errorf("preload failed: %w", err)
		}
	}
	log.Println("Preload complete.")
	return nil
}

func worker(cfg *Config, wg *sync.WaitGroup, jobs <-chan int, results chan<- Result) {
	defer wg.Done()
	for range jobs {
		method, data := generateMixedRequest(cfg)
		start := time.Now()
		_, err := runner.Run(
			method,
			cfg.ServerAddress,
			runner.WithProtoFile(cfg.ProtoFile, []string{filepath.Dir(cfg.ProtoFile)}),
			runner.WithTotalRequests(1),
			runner.WithData(data),
			runner.WithInsecure(true),
			runner.WithTimeout(10*time.Second),
		)
		latency := time.Since(start).Seconds() * 1000 // ms
		if err != nil {
			results <- Result{Method: method, LatencyMs: latency, Error: err}
		} else {
			results <- Result{Method: method, LatencyMs: latency}
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

	if err := w.Write([]string{"Method", "LatencyMs", "Error"}); err != nil {
		return err
	}
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

// RunBenchmarks executes the benchmark with the provided configuration and
// writes CSV output to the results directory.
func RunBenchmarks(cfg Config) error {
	// fill defaults
	if cfg.ValueSize == 0 {
		cfg.ValueSize = 1024
	}
	if cfg.KeyCount == 0 {
		cfg.KeyCount = 10000
	}
	if cfg.Concurrency == 0 {
		cfg.Concurrency = 100
	}
	if cfg.TotalRequests == 0 {
		cfg.TotalRequests = 10000
	}
	if cfg.GetPct == 0 && cfg.PutPct == 0 && cfg.DeletePct == 0 {
		cfg.GetPct = 50
		cfg.PutPct = 30
		cfg.DeletePct = 20
	}

	if cfg.GetPct+cfg.PutPct+cfg.DeletePct != 100 {
		return fmt.Errorf("Get, Put, and Delete percentages must sum to 100")
	}

	rand.Seed(time.Now().UnixNano())
	keyPool = nil
	for i := 0; i < cfg.KeyCount; i++ {
		keyPool = append(keyPool, randomBytes(8))
	}

	if err := preloadKeys(&cfg); err != nil {
		return err
	}

	log.Println("Running benchmark with mixed RPCs...")
	jobs := make(chan int, cfg.TotalRequests)
	results := make(chan Result, cfg.TotalRequests)

	var wg sync.WaitGroup
	for i := 0; i < cfg.Concurrency; i++ {
		wg.Add(1)
		go worker(&cfg, &wg, jobs, results)
	}

	for i := 0; i < cfg.TotalRequests; i++ {
		jobs <- i
	}
	close(jobs)
	wg.Wait()
	close(results)

	all := []Result{}
	for r := range results {
		all = append(all, r)
	}

	ts := time.Now().Format("2006-01-02_15-04-05")
	name := fmt.Sprintf("benchmark_results_%s", ts)
	if cfg.Tag != "" {
		name += "_" + cfg.Tag
	}
	name += ".csv"
	outPath := filepath.Join("results", name)
	log.Println("Writing CSV output to:", outPath)
	if err := writeCSV(outPath, all); err != nil {
		return fmt.Errorf("failed to write CSV: %w", err)
	}
	log.Println("Benchmark complete. CSV output saved to", outPath)
	return nil
}
