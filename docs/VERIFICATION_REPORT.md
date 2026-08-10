# Verification Report — 5G Telecom Performance Engine (POC)

This report covers the audit, defect fixes, and verification performed on
the uploaded project. It follows the structure requested: architecture,
changes made, safety analyses, and honest tool-by-tool results (including
tools that could not be run in this environment).

---

## 1. Environment constraints (read this first)

This build/test environment has `gcc` (13.3.0) and `make`, but **no network
access**, so the following could not be installed:

| Tool | Status |
|---|---|
| Valgrind / Memcheck | **NOT EXECUTED — TOOL UNAVAILABLE** |
| Helgrind | **NOT EXECUTED — TOOL UNAVAILABLE** |
| Cppcheck | **NOT EXECUTED — TOOL UNAVAILABLE** |
| A real MISRA-C static analyzer (e.g. Polyspace, PC-lint, Cppcheck's MISRA addon) | **NOT EXECUTED — TOOL UNAVAILABLE** |
| CUnit (`libcunit1-dev`) | **NOT AVAILABLE** — a minimal, documented, API-compatible shim (`test/mocks/`) was written so the test suite could still be compiled and *actually run*. The Makefile auto-detects and prefers real CUnit if it's present on the build machine; nothing in the test sources needs to change either way. |

Everything reported below as "PASS", "0 warnings", or "N assertions, 0
failed" was genuinely compiled/run in this sandbox. Nothing here is
fabricated. `make valgrind`, `make helgrind`, and `make cppcheck` are wired
up and will run for real the moment those tools are available — right now
they each print the same honest `NOT EXECUTED - TOOL UNAVAILABLE` message
instead of pretending to pass.

---

## 2. Architecture overview

A single-process, multi-threaded C console app simulating a 5G network
performance monitor:

- **`login.c`** — reads username/password from stdin, checks them against
  `config/Credentials.txt` (plaintext, space-delimited `user pass` lines).
- **`KPI_Collection.c`** — spawns 4 worker threads per sampling cycle (CPU,
  memory, throughput, latency), each reading a different data source
  (`/proc/stat`, `/proc/meminfo`, `/proc/net/dev`, and a local TCP
  loopback probe on port 8080) and joins all 4 before returning one
  populated `Record`.
- **`DataCollection.c`** — a shared, mutex+condvar-protected doubly linked
  list ("the queue") holding `Record`s, plus append-only logging of every
  record to `logs/network_log.txt` and the ability to rebuild the in-memory
  queue by replaying that log.
- **`Analytic.c`** — per-record SLA/threshold alerting, and traversal of
  the shared queue to compute aggregate statistics (`AnalyticsSummary`).
- **`Report.c`** — merges a new `AnalyticsSummary` with the most recent
  historical one (if any) found at the tail of `logs/performance_report.txt`,
  and appends a combined report.
- **`ErrorLog.c`** — a thread-safe, mutex-protected logger writing to
  `logs/system_error.log`.
- **`main.c`** — coordinates: logs in, starts a producer thread that loops
  `get_KPI → store_data → analyze_latest_record → enqueue`, and runs a
  console menu (display queue / compute+export report / rebuild queue /
  exit) as the consumer.

**Threading model:** one long-lived producer thread; the main thread is the
sole consumer/menu driver. `queue_mutex` protects `front`/`rear`/`count`;
`queue_cond` signals the consumer when `dequeue()` is waiting on an empty
queue. `log_mutex` (in `ErrorLog.c`) is independent and protects only the
log file handle. KPI collection itself spawns 4 short-lived worker threads
per cycle that are joined before `get_KPI()` returns — they don't outlive
the call and share no state with each other (each writes to a distinct
caller-owned output slot).

**Ownership:** every `DLL` node is `malloc`'d in `enqueue()`, owned by the
queue while linked in, and `free()`'d either by `free_queue()` or by
whoever calls `dequeue()` (its Javadoc-style comment in `DataCollection.h`
says as much, and `test_DataCollection.c` frees the node it dequeues).
`get_time()` in `DataCollection.c` mallocs a timestamp string that
`store_data()` always frees.

---

## 3. Complete file/function inventory

```
src/Analytic.c        analyze_latest_record, generate_analytics_summary
src/DataCollection.c  store_data, enqueue, dequeue, queue_display,
                       free_queue, rebuild_dll
                       (static) display, get_time, log_data_to_file,
                                 read_data_from_file
src/ErrorLog.c        ErrorLog_Init, ErrorLog_Cleanup, ErrorLog_Write
                       (static) get_timestamp, level_to_string
src/KPI_Collection.c  get_KPI
                       (static) get_cpu_time, get_cpu_utilization,
                                 get_memory_usage, get_throughput_packetloss,
                                 calculate_throughput_packetloss,
                                 latency_client, latency_server, get_latency
src/Report.c          export_performance_report
                       (static) read_previous_report
src/login.c           get_Credentials, validate_Credentials, login_attempt
src/main.c            main, producer_thread_function
                       (static) print_menu, get_menu_choice
```

Dependency order (lowest-level first): `Typedefs.h` → `ErrorLog.c` →
`DataCollection.c` → `KPI_Collection.c` → `Analytic.c` → `Report.c` →
`login.c` → `main.c`.

---

## 4. Defects found and fixed

1. **Corrupted headers (build-breaking).** Every literal capital "M" had
   been silently stripped from `Typedefs.h`, `login.h`, `Analytic.h`,
   `DataCollection.h`, `KPI_Collection.h` (`MAX`→`AX`,
   `MEMORY_WARN_THRESHOLD`→`EORY_WARN_THRESHOLD`,
   `NUM_CONSUMERS`→`NU_CONSUERS`, etc.), while the `.c` files still
   referenced the correct names. **Fixed:** all 6 headers rewritten clean.
2. **Missing `latency_server()` definition (build-breaking).**
   `get_latency()` called it, but it was never defined anywhere.
   **Fixed:** implemented a real TCP loopback latency probe — binds/listens
   on `127.0.0.1:8080`, times the interval to receiving the client's
   payload, and every blocking call (`select`/`accept`/`recv`) is guarded
   by a timeout so the thread can never hang indefinitely.
3. **`test_Analytic.c` used `rec->field` on a value type `Record rec;`**
   (build-breaking). **Fixed:** changed to `rec.field` / `&rec`.
4. **Original `Makefile` would have produced duplicate-symbol link errors**
   for any whitebox test file that `#include`s a `.c` source directly
   (`test_KPI_Collection.c` already did this) because the corresponding
   production `.o` was still linked into the test binary too. **Fixed:**
   `WHITEBOX_SRCS` list now excludes those `.o` files from the test link.
5. **`test_KPI_Collection.c` redefined `/proc` path macros *before*
   including the header that (re-)defines them**, so the header's
   `#define` would win back over the test's path stubs — the mocking never
   actually worked. **Fixed:** header included first, override applied
   after, guard prevents the header's second inclusion (via the `.c`
   file) from re-winning.
6. **Format-string/type mismatches:** `uint64_t` values passed to `%llu`
   in `fscanf`/`sscanf`/`fprintf` (undefined on platforms where
   `uint64_t` is `unsigned long`, not `unsigned long long`). **Fixed:**
   switched to portable `PRIu64`/`SCNu64` from `<inttypes.h>` everywhere.
7. **`Record_Native_Short` (`unsigned short`) values passed to a `%u`
   format specifier without a cast** — default argument promotion makes
   this a plain (signed) `int`, not `unsigned int`, which `-Wformat=2`
   correctly flags. **Fixed:** explicit `(unsigned int)` casts added.
8. **Missing `_POSIX_C_SOURCE`/`_DEFAULT_SOURCE` feature-test macros**
   under `-std=c11` caused implicit-declaration warnings (and, without
   `-Wall`, silent UB) for `localtime_r`/`usleep`. **Fixed:** defined
   globally in the Makefile.
9. **Unaligned-pointer risk:** `Analytic.c` took `&(temp->R)` from a
   `DLL` node marked `__attribute__((packed))`, risking a misaligned
   pointer to `Record`'s `double`/`long` members (UB in C; flagged by
   `-Waddress-of-packed-member`). **Fixed:** removed `packed` from `DLL`
   and `AnalyticsSummary` (neither is serialized byte-for-byte — packing
   bought nothing) and switched the traversal to copy each `Record` by
   value.
10. **Infinite busy-loop on stdin EOF/read failure (runtime defect, not
    just a compile issue).** `get_menu_choice()` returned a single boolean;
    on EOF, `fgets()` returns `NULL` on every subsequent call with no
    blocking, so the menu loop's `continue` spun at 100% CPU forever.
    Reproduced with `timeout 10 ./performance_engine` — it had to be
    SIGKILLed (exit 124). **Fixed:** introduced a 3-state
    `MenuInputResult` (`OK`/`INVALID`/`EOF`) so EOF now triggers the same
    graceful shutdown path as choosing "Exit". Re-verified: exits cleanly
    (code 0) immediately.
11. **Uninitialized/unterminated buffer risk in `login.c`.**
    `get_Credentials()` left the caller's `username`/`password` buffers
    untouched when `fgets()` failed; since `main.c` never initialized those
    stack buffers, an immediate EOF during login could pass a
    non-null-terminated garbage buffer into `validate_Credentials()`'s
    `strcmp`/`strncpy` calls. **Fixed:** both buffers are now forced to `""`
    on `fgets` failure, and `main.c`'s declarations were changed to
    `= {0}` as defense in depth.
12. **`Report.c`'s history-merge silently never worked.**
    `read_previous_report()` read only the last **512 bytes** of the report
    file to find the last `HISTORICAL_DATA|` tag, but one real report entry
    is **~889 bytes** — so the tag was always outside the read window and
    every run after the first was treated as if no history existed
    (confirmed via a standalone repro: 889 bytes/entry, tag at the front).
    Additionally, even with a bigger window, the code used `strstr()`'s
    *first* match rather than the *last*, which would pick a stale entry
    if two ever landed in the window. **Fixed:** widened the tail window to
    4096 bytes (with headroom, documented) and changed the search to find
    the last match. **Verified against the real running app:** two
    consecutive sessions now correctly show `records:1` then `records:2`
    in the merged tag (previously would have stayed at `records:1`
    forever).
13. **Two test-suite-only defects** (not present in production code):
    `test_analyze_latency_alerts`/`test_analyze_packet_loss_alerts`
    exercised `analyze_latest_record()` against a `Record rec;` with only
    one field initialized, reading uninitialized stack memory for the
    other fields (visible as garbage like `-339720768 ms` in the alert
    output). **Fixed:** zero-initialized (`= {0}`). A memory leak
    (`malloc`'d `Record` never freed) in `test_socket_failure_handling`
    was also fixed, and its rationale comment updated now that
    `latency_server` genuinely works.
14. **Missing test coverage.** `login.c` and `Report.c` had zero unit
    tests. **Fixed:** added `test/test_login.c` (8 tests covering
    `validate_Credentials`: match, wrong password, unknown user,
    multi-line file, malformed line, missing file, NULL args, and a
    prefix-username false-match guard) and `test/test_Report.c` (5 tests
    covering `export_performance_report`: NULL input, empty summary,
    first write, merge-with-history — the test that caught defect #12 —
    and an unwritable-path failure case using a real `EISDIR` condition).

None of the above were "silenced" — every one is an actual behavioral or
build fix, verified by rebuilding and re-running.

---

## 5. Memory-management analysis (static, since Valgrind is unavailable)

- Every `malloc`/`calloc` call site checks its return value
  (`enqueue`'s `DLL` node, `get_time`'s timestamp buffer).
- Every `DLL` node has exactly one owner at a time (the queue, or whoever
  called `dequeue()`), and exactly one `free()` call on every path
  (`free_queue()`'s loop, `dequeue()`'s caller contract, and the mock
  queue-population helper in `test_Analytic.c`).
- `get_time()`'s timestamp string is freed by `store_data()` on both the
  success and `NULL` paths (the `NULL` path just uses a literal instead
  and never allocates).
- All `fopen()` call sites are paired with `fclose()` on every return path,
  including early-return error paths (checked file-by-file in
  `DataCollection.c`, `KPI_Collection.c`, `Report.c`, `login.c`,
  `ErrorLog.c`).
- `realloc()` is not used anywhere in this codebase.
- No use of a pointer after `free()`, no double-free, and no
  stack-buffer overflow were found in manual review; all fixed-size stack
  buffers (`errMsg[64/128]`, `timestamp[19/32/64]`, `line[MAX_LINE]`, etc.)
  are written via bounded `snprintf`/`fgets`/`strftime`/`strncpy` with
  explicit size arguments, and every `strncpy` destination is explicitly
  NUL-terminated afterward.

This is a thorough static review, not a substitute for actually running
Memcheck — `make valgrind` will run it for real as soon as the tool is
available.

---

## 6. Thread-safety analysis (static, since Helgrind is unavailable)

| Shared object | Owner | Readers | Writers | Protected by | Lifetime |
|---|---|---|---|---|---|
| `front`, `rear`, `count` | `DataCollection.c` | producer, consumer (menu), all whitebox test threads | same | `queue_mutex` | Program lifetime |
| `queue_cond` | `DataCollection.c` | `dequeue()` (wait) | `enqueue()` (signal) | used together with `queue_mutex` | Program lifetime |
| `log_file` (static) | `ErrorLog.c` | `ErrorLog_Write` | `ErrorLog_Init`/`Cleanup` | `log_mutex` | Between `Init` and `Cleanup` |
| Per-cycle KPI locals (`cpu_util`, `memory_usage`, `rx`/`tx`, `latency`) | `get_KPI()`'s stack frame | the 4 worker threads it joins | same | **not mutex-protected, but not racy**: each worker writes to a distinct address, and `get_KPI()` calls `pthread_join()` on all 4 before reading any of them, giving a happens-before edge. | One `get_KPI()` call |
| `processing_active` | `main.c` | producer thread's `while` condition | main thread (menu) | `volatile`, single-writer/single-reader flag, not a mutex — acceptable for this simple stop-flag pattern, though a formally stricter design would use `atomic_int` or protect it with the queue mutex. | Program lifetime |

No lock-order problem exists because the codebase only ever holds one
mutex at a time (`queue_mutex` or `log_mutex`, never both nested). Every
`pthread_mutex_lock()` has a matching `pthread_mutex_unlock()` on every
return path (verified per-function). Mutexes are all statically
initialized (`PTHREAD_MUTEX_INITIALIZER`) and never destroyed while in use
(no `pthread_mutex_destroy()` calls at all, which is fine given they live
for the whole process). All threads (producer, KPI workers, latency
client/server) are joined before the program exits or before the resources
they touch are freed.

`make helgrind` will run this for real as soon as the tool is available.

---

## 7. Cppcheck / MISRA-C findings

**NOT EXECUTED — TOOL UNAVAILABLE** for an automated Cppcheck or MISRA-C
checker run; `make cppcheck` is wired up and will run
`cppcheck --enable=all --std=c11 --inconclusive --force` for real once the
tool is installed.

In its place, `docs/MISRA_DEVIATIONS.md` documents every *known or likely*
deviation found during manual review, with rationale, plus a list of
outright defects (not deviations) that were fixed rather than deviated
from. Please do not treat this as a certified MISRA compliance statement.

---

## 8. Unit-test summary

```
$ make run_tests
...
=== Summary: 130 assertion(s) checked, 0 failed ===
```

5 suites, 26 test functions, 130 assertions, **0 failures**, reproducible
across repeated runs (checked 3x in a row). Breakdown:

| Suite | Tests | What's covered |
|---|---|---|
| `Analytic_Module_Suite` | 4 | SLA latency/packet-loss alerting, summary math (avg/min/max/violations) with overflow-safe accumulators, empty-queue behavior |
| `DataCollection_Module_Suite` | 3 | concurrent `enqueue` under contention + DLL integrity, `dequeue` blocking/signaling via `queue_cond`, `get_time()` resource management |
| `KPI_Collection_Module_Suite` | 4 | end-to-end `get_KPI()` math (including a helper thread that rewrites the `/proc` stubs mid-cycle so the two-sample delta logic is exercised correctly — see defect note below), `/proc/stat` and `/proc/meminfo` parser robustness (valid/malformed/missing-file), the TCP latency probe path completing without hanging |
| `Login_Module_Suite` (**new**) | 8 | `validate_Credentials`: match, wrong password, unknown user, multi-line file, malformed line, missing file, NULL args, prefix-username guard |
| `Report_Module_Suite` (**new**) | 5 | `export_performance_report`: NULL input, empty summary, first write, merge-with-history (caught defect #12), unwritable path |

**Test-design note:** `get_cpu_utilization()` and
`calculate_throughput_packetloss()` both take two `/proc` samples ~1s apart
and report the *delta*. The original `test_get_KPI_math_precision` wrote a
single static mock file and expected a specific non-zero delta — which is
impossible against an unchanging file (delta is always zero). This was a
test-design bug, not a production bug; fixed by seeding a zero baseline and
using a helper thread to advance the mock files partway through the SUT's
sampling window, which is what a real, changing `/proc` counter would do.

**Because `libcunit1-dev` isn't installed here**, these ran against the
bundled `test/mocks/mini_cunit` shim (documented deviations from real
CUnit are listed in its header). The Makefile prefers real CUnit
automatically if present — no test source changes needed either way.

---

## 9. Compiler warnings/results

Both the application and test binaries compile and link with **zero
warnings and zero errors** under the complete requested flag set:

```
-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
-Wformat=2 -Wundef -Wstrict-prototypes -Wmissing-prototypes
-Wold-style-definition -Wwrite-strings -Wcast-qual -Wpointer-arith -Werror
```

No warnings were suppressed with pragmas, casts-to-silence, or
`-Wno-*` flags — every one was fixed at the root cause (see Section 4).

---

## 10. Stress / runtime robustness results

All of the following were actually executed in this sandbox:

- **Repeated full execution** (login → display → analyze/export → rebuild
  → display → analyze/export → exit), 3x in a row: consistent, clean exit
  code 0 each time.
- **3x failed login attempts** (wrong credentials): correctly reports
  "Login Failed" and exits without hanging or crashing.
- **Immediate EOF on stdin** (no input at all): correctly falls through
  the bounded 3-attempt login retry and exits (code 0).
- **EOF mid-menu-loop** (valid login, then closed stdin): previously hung
  forever in a busy-loop (had to be killed by `timeout`, exit 124) — now
  exits gracefully immediately (code 0) after the fix in Section 4, item 10.
- **Two consecutive full sessions**, each producing one KPI sample and
  exporting a report: confirmed the report's `records:` count genuinely
  accumulates (`1` → `2`) across runs, verifying the Section 4 item 12 fix
  against the real application, not just the unit test.
- **Repeated `make clean && make all && make test`**: deterministic, clean
  build every time.

---

## 11. Remaining known limitations

- Valgrind/Helgrind/Cppcheck/a real MISRA tool were not run — see Section 1.
  Everything reported as "clean" from static/manual review should still be
  independently confirmed with those tools when available; `make valgrind`,
  `make helgrind`, and `make cppcheck` are ready to do so.
- `get_Credentials()`/`login_attempt()` (interactive stdin prompting) are
  not unit-tested — they'd need a stdin-injection or `dependency-injectable`
  I/O redesign to test without spawning a subprocess; out of scope for this
  pass given the existing architecture. `validate_Credentials()`, which
  contains all of the actual parsing/comparison logic, is fully tested.
  `main.c`'s menu loop and thread orchestration are exercised only via the
  manual stress runs in Section 10, not via CUnit, for the same reason.
  `latency_server()`/`latency_client()`/`get_latency()` are exercised only
  indirectly through `get_KPI()`, not with dedicated unit tests isolating
  socket edge cases (e.g. a genuinely occupied port 8080) — that would need
  a proper socket-mocking layer.
- `processing_active` is a plain `volatile int`, not a `_Atomic`/mutex
  protected flag. In practice this is a single-writer/single-reader stop
  flag and is safe as used, but a stricter MISRA/concurrency review would
  likely flag `volatile` as insufficient for cross-thread synchronization
  guarantees on all platforms and recommend `_Atomic` or protecting it with
  `queue_mutex`.
- `MAX_QUEUE_SIZE` in `Typedefs.h` is defined but not enforced anywhere —
  the queue can grow unbounded if records are produced faster than
  consumed. This was true in the original code and was left unchanged
  since bounding it would change functional behavior beyond a safety fix;
  flagged here for awareness.
- A handful of cosmetic indentation inconsistencies remain in
  `DataCollection.c`/`Report.c` as an artifact of programmatically
  stripping the original files' extremely repetitive/corrupted comment
  blocks (see Section 4 context) — purely cosmetic, does not affect
  behavior or any compiler/analysis result.
