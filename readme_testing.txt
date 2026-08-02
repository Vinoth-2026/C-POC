

Step 1: Compile the necessary source modules (production logic):
You must compile the standard application modules required for the test into standard object files (.o)[cite: 2]. The Analytic unit test requires the DataCollection and Analytic modules.

bash
# CFLAGS should strictly match your production build configuration, including include paths.
gcc -Wall -I. -c DataCollection.c
gcc -Wall -I. -c Analytic.c



Step 2: Compile the test infrastructure for the specific module:
Compile test_main.c (which sets up CUnit/Registry) and the specific module test file into object files[cite: 1, 3].

bash
# Note: You do not need the special mock flag used for KPI testing here.
gcc -Wall -I. -c test_main.c
gcc -Wall -I. -c test_Analytic.c



Step 3: Link the objects into a module-specific executable:
Link the standard application objects (MOD_OBJ), the specific test objects, and the required CUnit and standard math (-lm) libraries[cite: 1, 3].

bash
# Output executable is named run_analytic_tests
gcc -Wall -o run_analytic_tests test_main.o test_Analytic.o Analytic.o DataCollection.o -lcunit -lm



Step 4: Execute the test:

bash
./run_analytic_tests



This single execution will run only the tests contained within the test_Analytic.c suite, fulfilling your requirement for separate, per-module testing[cite: 2].

 Applying this Strategy to Other Modules (KPI_Collection)

You apply this identical modular process to test other files. The only modification is substituting the specific test source/object files and handling the simulator stub/mock flag for test_KPI.c[cite: 3].

Example: Standalone Test for KPI module

bash
# 1. Compile production objects (KPI also needs DataCollection)
gcc -Wall -I. -c DataCollection.c
gcc -Wall -I. -c KPI_Collection.c

# 2. Compile test objects (Note the critical -DUNIT_TESTING_KPI flag)
gcc -Wall -I. -c test_main.c
gcc -Wall -I. -DUNIT_TESTING_KPI -c test_KPI.c

# 3. Link (lpthread is required for KPI module logic)
gcc -Wall -o run_kpi_tests test_main.o test_KPI.o KPI_Collection.o DataCollection.o -lcunit -lpthread -lm

# 4. Execute
./run_kpi_tests

Here are the commands to run the Overall Integration Test (the end-to-end flow from test_Integration.c) and the final commands for compiling the Main Application.

### Build and Run Overall Integration Test

This standalone executable, run_integration_tests, substitutes main.c. It links every source module (excluding login.c) to verify the sequential Harvest -> Process -> Analyze -> Report Export data flow. It does not require pthreads or the special stub flag, as it stimulates the pure mathematical integration on a deterministic vector.

Step 1: Modular Compilation (all sources, excluding login/main)

bash
# Compile all integrated production logic into standard objects
# Native types and packed structure definitions are resolved correctly.
gcc -Wall -I. -c DataCollection.c
gcc -Wall -I. -c Analytic.c
gcc -Wall -I. -c Report.c
gcc -Wall -I. -c KPI_Collection.c # The standard, dynamic simulation module



Step 2: Compile the test infrastructure for integration

bash
# Compile test_main.c (CUnit setup) and the specific integration test file
gcc -Wall -I. -c test_main.c
gcc -Wall -I. -c test_Integration.c



Step 3: Modular Linking

bash
# Output executable is named run_integration_tests
# Links ALL application objects plus CUnit and standard libraries.
gcc -Wall -o run_integration_tests test_main.o test_Integration.o Analytic.o DataCollection.o Report.o KPI_Collection.o -lcunit -lpthread -lm



Step 4: Execute the Integration Test

bash
./run_integration_tests



This execution will run only the complete End-to-End Core Integration Suite vector defined in test_Integration.c.

---

### Final Application Compilation (main.c)

This builds the final Performance Engine Application. It is not a test executable; it is the final binary that coordinates the concurrent threads to simulation-driven KPIs. It uses all standard production objects and links the standard user entry point (main.c)[cite: 1, 2].

Step 1: Modular Compilation (all sources)

bash
# Standard compilation of all application modules including login.c and main.c
gcc -Wall -I. -c login.c
gcc -Wall -I. -c KPI_Collection.c # standard dynamic simulation logic
gcc -Wall -I. -c DataCollection.c # sequential DLL/File IO
gcc -Wall -I. -c Analytic.c       # pure SLA math/Contiguous traversal
gcc -Wall -I. -c Report.c         # pure cumulative merge/fseek logic
gcc -Wall -I. -c main.c           # final user menu loop



Step 2: Modular Linking

bash
# Final Application is named performance_engine
# Links ALL application objects, excluding CUnit.
# lpthread and lm are critical for concurrent simulation and math logic.
gcc -Wall -o performance_engine main.o KPI_Collection.o DataCollection.o Analytic.o Report.o login.o -lpthread -lm



Execution (Run the Performance Engine Application):

bash
# Executes the final optimized binary for POC validation
./performance_engine

