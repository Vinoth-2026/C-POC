### 2. Test README (`POC/test/README.md`)

Create this file in the `POC/test/` directory.

```markdown
# 5G Engine Unit Testing Framework

## Overview
This directory contains the unit testing infrastructure for the 5G Network Performance Engine. The tests are written in C using the **CUnit** framework.

The goal of these tests is to verify the functional correctness of individual modules in isolation, particularly focusing on mathematical precision, edge cases, and thread synchronization mechanisms.

## Prerequisites
To build and run these tests, you must have the CUnit development library installed on your Linux system.

On Debian/Ubuntu based systems:
```bash
sudo apt-get install libcunit1-dev