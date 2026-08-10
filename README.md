# 5G Network Performance Engine Simulator (PoC)

## Overview
This is a Proof of Concept (PoC) application simulating a 5G Telecommunication Performance Engine. It demonstrates a multi-threaded pipeline for collecting Network Key Performance Indicators (KPIs), managing data in a synchronized in-memory queue, performing real-time Service Level Agreement (SLA) analysis, and generating performance reports.

The primary goals of this PoC are to demonstrate:
1.  **Multi-threaded Producer-Consumer Architecture:** Using pthreads for concurrent data acquisition and processing.
2.  **Thread Safety:** Implementing robust synchronization using mutexes and condition variables.
3.  **Robust Error Handling:** A centralized, thread-safe logging system for runtime faults.
4.  **MISRA C Alignment:** Adherence to safe coding standards, including explicit-width types and safe I/O operations.

## Directory Structure
*   `bin/`: Compiled executable binaries.
*   `config/`: Input configuration files (e.g., credentials).
*   `include/`: Public header files (`.h`).
*   `logs/`: Application runtime logs and generated reports.
*   `obj/`: Intermediate object files (`.o`).
*   `src/`: Production source code (`.c`).
*   `test/`: Unit testing source code and infrastructure.

## Prerequisites
This application is designed for a Linux environment. You need the following installed:
*   GCC (GNU Compiler Collection)
*   GNU Make
*   POSIX Threads library (`pthread`)
*   CUnit Development Library (required for testing only)

## Getting Started

### 1. Build the Application
To compile the main application, run the following command from the project root:

```bash
make
```

This produces `bin/performance_engine`. Run it with:

```bash
./bin/performance_engine
```

Log in with the credentials in `config/Credentials.txt` (default: username
`vinoth`, password `12345`).

### 2. Build and Run the Test Suite

```bash
make run_tests
```

This detects whether `libcunit1-dev` is installed and links against it if
so. If it is **not** installed (e.g. no network access to fetch it), the
Makefile automatically falls back to a small, documented CUnit-API-compatible
shim in `test/mocks/` so the same test sources still compile and run for
real — no test code needs to change either way. Current status: 5 suites,
26 tests, 130 assertions, 0 failures.

### 3. Other Makefile targets

```bash
make clean      # remove build artifacts and logs
make debug      # unoptimized debug build
make valgrind   # Memcheck the built app (prints "NOT EXECUTED - TOOL UNAVAILABLE" if valgrind isn't installed)
make helgrind   # Helgrind the built app (same fallback behavior)
make cppcheck   # static analysis (same fallback behavior)
make coverage   # gcov line-coverage report (same fallback behavior)
```

### 4. Further documentation

- `docs/VERIFICATION_REPORT.md` — full audit: architecture, every defect
  found and fixed, memory/thread-safety analysis, and an honest accounting
  of which verification tools actually ran versus which were unavailable.
- `docs/MISRA_DEVIATIONS.md` — known/likely MISRA-C:2012 deviations and
  their rationale (not a substitute for a real MISRA tool run).
