package benchmark

import (
	"encoding/base64"
	"encoding/csv"
	"encoding/json"
	"fmt"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"sort"
	"strings"
	"time"
)

const (
	serverAddress = "localhost:50051"
	concurrency   = 50
	totalRequests = 1000
)

// Config holds the benchmark configuration
type Config struct {
	ServerAddress string
	Concurrency   int
	TotalRequests int
	ProtoFile     string
	Tag           string
}

// BenchmarkResult represents the structure of ghz JSON output
type BenchmarkResult struct {
	Name       string  `json:"name"`
	EndReason  string  `json:"endReason"`
	Date       string  `json:"date"`
	Count      int     `json:"count"`
	Total      float64 `json:"total"`
	Average    float64 `json:"average"`
	Fastest    float64 `json:"fastest"`
	Slowest    float64 `json:"slowest"`
	RPS        float64 `json:"rps"`
	ErrorCount int     `json:"errorCount"`
	ErrorRate  float64 `json:"errorRate"`
	Details    []struct {
		Timestamp string  `json:"timestamp"`
		Latency   float64 `json:"latency"`
		Error     string  `json:"error"`
		Status    string  `json:"status"`
	} `json:"details"`
}

// DefaultConfig returns the default benchmark configuration
func DefaultConfig() Config {
	return Config{
		ServerAddress: serverAddress,
		Concurrency:   concurrency,
		TotalRequests: totalRequests,
		ProtoFile:     "../../proto/kvstore.proto", // Relative path from tests/benchmark
	}
}

func generateTestData() ([]string, []string) {
	keys := make([]string, totalRequests)
	values := make([]string, totalRequests)

	for i := 0; i < totalRequests; i++ {
		keys[i] = fmt.Sprintf("key-%d", i)
		values[i] = fmt.Sprintf("value-%d", i)
	}

	return keys, values
}

func encodeBase64(s string) string {
	return base64.StdEncoding.EncodeToString([]byte(s))
}

// calculatePercentile calculates the nth percentile from a sorted slice of latencies
func calculatePercentile(latencies []float64, percentile float64) float64 {
	if len(latencies) == 0 {
		return 0
	}
	index := int(float64(len(latencies)-1) * percentile / 100)
	return latencies[index]
}

func saveResultsToCSV(results []BenchmarkResult, config Config) error {
	// Create results directory if it doesn't exist
	if err := os.MkdirAll("results", 0755); err != nil {
		return fmt.Errorf("failed to create results directory: %v", err)
	}

	// Generate timestamp for filename
	timestamp := time.Now().Format("2006-01-02_15-04-05")
	filename := fmt.Sprintf("benchmark_results_%s", timestamp)
	if config.Tag != "" {
		filename = fmt.Sprintf("%s_%s", filename, config.Tag)
	}
	csvFilename := fmt.Sprintf("results/%s.csv", filename)

	// Create CSV file
	file, err := os.Create(csvFilename)
	if err != nil {
		return fmt.Errorf("failed to create CSV file: %v", err)
	}
	defer file.Close()

	writer := csv.NewWriter(file)
	defer writer.Flush()

	// Write header
	header := []string{
		"Timestamp", "Operation", "Total Requests", "Concurrency",
		"Total Time (s)", "Average Latency (ms)", "Fastest (ms)", "Slowest (ms)",
		"RPS", "Error Count", "Error Rate",
		"P10 (ms)", "P25 (ms)", "P50 (ms)", "P75 (ms)", "P90 (ms)", "P95 (ms)", "P99 (ms)",
	}
	if err := writer.Write(header); err != nil {
		return fmt.Errorf("failed to write CSV header: %v", err)
	}

	// Write data rows
	now := time.Now().Format(time.RFC3339)
	for _, result := range results {
		// Extract latencies and sort them
		latencies := make([]float64, len(result.Details))
		for i, detail := range result.Details {
			latencies[i] = detail.Latency
		}
		sort.Float64s(latencies)

		// Calculate percentiles
		p10 := calculatePercentile(latencies, 10)
		p25 := calculatePercentile(latencies, 25)
		p50 := calculatePercentile(latencies, 50)
		p75 := calculatePercentile(latencies, 75)
		p90 := calculatePercentile(latencies, 90)
		p95 := calculatePercentile(latencies, 95)
		p99 := calculatePercentile(latencies, 99)

		row := []string{
			now,
			result.Name,
			fmt.Sprintf("%d", result.Count),
			fmt.Sprintf("%d", config.Concurrency),
			fmt.Sprintf("%.2f", result.Total/1_000_000), // seconds
			fmt.Sprintf("%.2f", result.Average/1000),    // ms
			fmt.Sprintf("%.2f", result.Fastest/1000),    // ms
			fmt.Sprintf("%.2f", result.Slowest/1000),    // ms
			fmt.Sprintf("%.2f", result.RPS),
			fmt.Sprintf("%d", result.ErrorCount),
			fmt.Sprintf("%.2f", result.ErrorRate),
			fmt.Sprintf("%.2f", p10/1000),
			fmt.Sprintf("%.2f", p25/1000),
			fmt.Sprintf("%.2f", p50/1000),
			fmt.Sprintf("%.2f", p75/1000),
			fmt.Sprintf("%.2f", p90/1000),
			fmt.Sprintf("%.2f", p95/1000),
			fmt.Sprintf("%.2f", p99/1000),
		}
		fmt.Printf("row: %v\n", row)
		if err := writer.Write(row); err != nil {
			return fmt.Errorf("failed to write CSV row: %v", err)
		}
	}

	fmt.Printf("Results saved to %s\n", csvFilename)
	return nil
}

func runBenchmark(config Config, name, method string, data interface{}) (*BenchmarkResult, error) {
	outputFile := filepath.Join("results", fmt.Sprintf("%s_benchmark.json", name))

	// Create results directory if it doesn't exist
	if err := os.MkdirAll("results", 0755); err != nil {
		return nil, fmt.Errorf("failed to create results directory: %v", err)
	}

	// Convert data to JSON
	jsonData, err := json.Marshal(data)
	if err != nil {
		return nil, fmt.Errorf("failed to marshal data: %v", err)
	}

	cmd := exec.Command("ghz",
		"--insecure",
		"--proto", config.ProtoFile,
		"--call", fmt.Sprintf("kvstore.KeyValueStore.%s", method),
		"-d", string(jsonData),
		"-n", fmt.Sprintf("%d", config.TotalRequests),
		"-c", fmt.Sprintf("%d", config.Concurrency),
		"-o", outputFile,
		"--format", "json",
		"--timeout", "0", // No timeout
		"--skipFirst", "0", // Don't skip any requests
		"--rps", "0", // No rate limiting
		config.ServerAddress,
	)

	// Create a pipe for stdout
	stdout, err := cmd.StdoutPipe()
	if err != nil {
		return nil, fmt.Errorf("failed to create stdout pipe: %v", err)
	}

	// Create a pipe for stderr
	stderr, err := cmd.StderrPipe()
	if err != nil {
		return nil, fmt.Errorf("failed to create stderr pipe: %v", err)
	}

	// Start the command
	if err := cmd.Start(); err != nil {
		return nil, fmt.Errorf("failed to start command: %v", err)
	}

	// Read stdout and stderr
	var stdoutBuf, stderrBuf strings.Builder
	go func() {
		io.Copy(&stdoutBuf, stdout)
	}()
	go func() {
		io.Copy(&stderrBuf, stderr)
	}()

	// Wait for the command to complete
	if err := cmd.Wait(); err != nil {
		return nil, fmt.Errorf("command failed: %v\nstderr: %s", err, stderrBuf.String())
	}

	// Read and parse the JSON result
	resultData, err := os.ReadFile(outputFile)
	if err != nil {
		return nil, fmt.Errorf("failed to read result file: %v", err)
	}

	var result BenchmarkResult
	if err := json.Unmarshal(resultData, &result); err != nil {
		return nil, fmt.Errorf("failed to parse result: %v\njson data: %s", err, string(resultData))
	}

	// Clean up the JSON file after reading
	if err := os.Remove(outputFile); err != nil {
		fmt.Printf("Warning: failed to remove JSON file %s: %v\n", outputFile, err)
	}

	return &result, nil
}

// RunBenchmarks executes all benchmarks with the given configuration
func RunBenchmarks(config Config) error {
	// Generate test data
	keys, values := generateTestData()

	var results []BenchmarkResult

	// Benchmark Put operations
	putData := struct {
		Key   string `json:"key"`
		Value string `json:"value"`
	}{
		Key:   encodeBase64(keys[0]),
		Value: encodeBase64(values[0]),
	}
	putResult, err := runBenchmark(config, "put", "Put", putData)
	if err != nil {
		return fmt.Errorf("failed to run Put benchmark: %v", err)
	}
	results = append(results, *putResult)

	// Benchmark Get operations
	getData := struct {
		Key string `json:"key"`
	}{
		Key: encodeBase64(keys[0]),
	}
	getResult, err := runBenchmark(config, "get", "Get", getData)
	if err != nil {
		return fmt.Errorf("failed to run Get benchmark: %v", err)
	}
	results = append(results, *getResult)

	// Benchmark Delete operations
	deleteData := struct {
		Key string `json:"key"`
	}{
		Key: encodeBase64(keys[0]),
	}
	deleteResult, err := runBenchmark(config, "delete", "Delete", deleteData)
	if err != nil {
		return fmt.Errorf("failed to run Delete benchmark: %v", err)
	}
	results = append(results, *deleteResult)

	// Save all results to CSV
	if err := saveResultsToCSV(results, config); err != nil {
		return fmt.Errorf("failed to save results to CSV: %v", err)
	}

	return nil
}
