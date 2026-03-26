# Flower Exchange

This repository contains a C++17 implementation of an exchange system with
flowers as the instruments.
The exchange application reads orders, applies validation, matches with price-time priority, and writes execution reports as CSV.

## Basic Usage

The following command builds the release version and run the executable:

```bash
make run
```

Example REPL session:

```text
PROCESS input_files/example1_orders.csv
Wrote reports to output/example1_orders_reports.csv
PROCESS input_files/example2_orders.csv
Wrote reports to output/example2_orders_reports.csv
QUIT
```

## Project Structure

- `src/common`: types, validator, memory pool, thread helpers
- `src/io`: CSV reader/writer
- `src/matching`: order book and matching engine
- `src/app`: application orchestration
- `tests`: unit, integration, stress tests
- `input_files`: input csv files
- `expected_reports`: expected output csv files
- `output`: generated execution report csv files

## Accepted Order Format
The exchange accepts orders in CSV format with the following columns:
- `Client Order ID`: Identifier given by the client for the order
- `Instrument`: The flower being traded
- `Side`: Buy(=1) or Sell(=2)
- `Quantity`: Number of units to buy/sell (min:10, max:1000) as multiples of 10
- `Price`: Price per unit (min:0.01, max:1000.00) as multiples of 0.01

## Execution Report Format
The execution reports are generated in CSV format with the following columns:
- `Order ID`: The unique identifier given by the exchange for the order
- `Client Order ID`: The identifier given by the client for the order
- `Instrument`: The flower being traded
- `Side`: Buy(=1) or Sell(=2)
- `Exec Status`: New, Rejected, Pfill, Fill
- `Quantity`: Number of units executed
- `Price`: Price per unit for the executed quantity
- `Reason`: Reason for rejection if the order is rejected, otherwise empty

## Performance

- Benchmarking Machine Specs:
  - CPU: Intel Core Intel® Core™ Ultra 7 Processor 155H
  - Cache: 24MB L3
  - RAM: 16GB DDR4
  - Kernel: Linux 6.19.7-1-cachyos
  - Compiler: GCC 15.2.1
  - Build Type: Release
- Benchmarking Methodology:
  - Used `order_generator.py` to create large order files with varying sizes (10K, 100K, 1M, 10M orders)
  - Measured total processing time for each file and calculated throughput
- Performance Metrics:
  - Throughput: ~1.5M orders per second

## References
- Building Low Latency Applications with C++ by Sourav Ghosh
- C++ Concurrency in Action by Anthony Williams
- Effective Modern C++ by Scott Meyers
- [https://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c]

## AI usage disclosure
LLM(s) were used for the following purposes:
- Code review and refactoring
- Test case generation and edge case identification
- Generating trivial code snippets
- perf report analysis and optimization suggestions
