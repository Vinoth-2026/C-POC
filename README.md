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