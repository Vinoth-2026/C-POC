# MISRA-C:2012 Deviation Record

**Scope:** This project is written close to MISRA-C:2012 principles where
practical, but has NOT been checked by an actual MISRA-C static analysis
tool (no such tool was available in this build environment; see
`VERIFICATION_REPORT.md`). This document is therefore a description of
known/likely deviations and the rationale for each, not a certified
compliance report. Do not claim MISRA compliance based on this file alone.

| Rule | Category | Module | Deviation | Rationale / Disposition |
|------|----------|--------|-----------|--------------------------|
| 17.1 | Advisory | `KPI_Collection.c` | Use of `sleep()`/`usleep()` in KPI sampling workers, and in `latency_server`'s connection-retry logic. | Required to space out the two counter samples used for CPU/throughput deltas and to simulate realistic timing. Bounded, deterministic durations only; no unbounded blocking (accept/recv paths use `select()` timeouts). **Deviation accepted.** |
| 17.1 | Advisory | `KPI_Collection.c`, `Analytic.c` | Use of `<math.h>`-adjacent floating point arithmetic and standard library `fscanf`/`sscanf`/`fopen`/`fclose`. | Required for `/proc` parsing and file I/O; every call's return value is checked and every failure path is logged and handled defensively. **Deviation accepted.** |
| 17.1 | Advisory | `Report.c` | Use of `fseek`/`ftell`/backward `SEEK_END` seek, `strftime`, `localtime_r`. | Required to locate the most recent `HISTORICAL_DATA` tag without re-reading the entire (potentially large) report file, and to produce thread-safe timestamps. All return values are checked. **Deviation accepted.** |
| 17.1 | Advisory | `DataCollection.c` | Use of `fscanf` for replaying `network_log.txt` during `rebuild_dll()`. | Required to parse the project's existing on-disk log format. Return value is checked (`rc == 6`); malformed lines are logged and skipped rather than causing undefined behavior. **Deviation accepted.** |
| 18.4 | Required | `Analytic.c` | (Historical, now removed) pointer arithmetic into a `DLL` node's embedded `Record`. | The original code took `&(temp->R)` from a struct marked `__attribute__((packed))`, which risks constructing a misaligned pointer to `Record`'s `double`/`long` members (undefined behavior in C, and flagged by GCC as `-Waddress-of-packed-member`). **Resolved, not deviated:** `__attribute__((packed))` was removed from `DLL` and `AnalyticsSummary` (neither struct is serialized byte-for-byte; packing bought nothing), and the traversal loop now copies each `Record` by value instead of aliasing through a pointer. |
| 10.1 / 10.3 | Required | `Analytic.c`, `KPI_Collection.c`, `Report.c` | Explicit-width accumulator types (`U32`/`U64`/`F32`) with explicit casts before every arithmetic/comparison operation mixing signed/unsigned or different widths. | Implemented as designed; this satisfies Rule 10.1/10.3 rather than deviating from it. Documented here because the pattern recurs throughout the codebase and is intentional. |
| 21.6 | Advisory | Whole project | Use of `<stdio.h>` (`printf`/`fprintf`/`fopen` family). | This is a console application whose entire purpose is to print status/menus and persist logs/reports to text files; the standard I/O library is fundamental to the requirements, not incidental. **Deviation accepted.** |
| 8.9 / magic numbers | Advisory | `main.c`, `login.c` | A few small literal constants remain in menu/loop bounds (e.g. `for (i = 1; i <= 3; i++)` login retry count, buffer size `16` for menu input). | Left as local literals rather than named constants because they are single-use, self-explanatory in context, and promoting every such literal to a named macro would reduce readability for a codebase this size. Flagged here rather than silently deviated from. |
| 15.5 (single point of exit) | Advisory | `main.c` | `goto cleanup;` used for centralized shutdown from multiple menu branches and from thread-creation failure paths. | This is the standard, MISRA-tolerated C idiom for guaranteed single-path cleanup (join producer thread, free queue, close logger) without duplicating that sequence at every exit point. **Deviation accepted per Rule 15.5's own allowance for a single forward jump to a common cleanup label.** |

## Known non-deviations (defects that were fixed, not deviated)

The following were not MISRA deviations but outright defects found during
this audit and corrected (see `VERIFICATION_REPORT.md` for full detail):

- Missing `latency_server()` definition (would not compile).
- `Record_Native_Short`/`unsigned short` values passed to `%u` in `printf`
  without a cast (undefined-behavior-adjacent format mismatch under strict
  `-Wformat=2`).
- `uint64_t` values scanned/printed with `%llu` instead of a
  platform-portable `PRIu64`/`SCNu64`.
- `get_menu_choice()` returning a single boolean, causing an infinite
  busy-loop on stdin EOF/read-error instead of terminating.
- `get_Credentials()` leaving caller-owned buffers unterminated on `fgets`
  failure, risking a read of uninitialized/non-terminated stack memory in
  `validate_Credentials()`.
- `read_previous_report()`'s 512-byte tail-read window was smaller than one
  real report entry (~889 bytes), so the historical merge feature silently
  never found any history to merge with; window enlarged and searching for
  the *last* tag match instead of the *first*.
