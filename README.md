# BookManagement (C++17)

Implementation of OOPD Assignment 2. The implementation follows the supplied
assignment PDF; `ANSWERS.md` lists the questions and design decisions.

## Build and run (WSL/Linux terminal)

From the directory containing this file:

```sh
cmake -S . -B build
cmake --build build
./build/bookmgmt_tests
./build/demo
```

The project uses C++17 and has no third-party library dependency. The tests use
standard `assert` statements.

## Contents

- `include/bookmgmt/` — public headers and umbrella header `bookmgmt.h`
- `src/` — library implementation
- `examples/demo.cpp` — runnable feature demonstration
- `tests/test_main.cpp` — assertion-based regression checks
- `ANSWERS.md` — question checklist, design decisions, and written answers

## Main extensions

Resource types: `Journal`, `EBook`, `AudioBook`, and `Thesis`. Acquisition
features: hardcover pricing, bulk pricing, configurable taxes, title quotas,
cancellation, department budgets, rollover, atomic batches, and cheapest-vendor
selection. Catalog features: author and ISBN/ISSN searches, publication-year
range searches, print lending, and electronic sessions. `Money` stores a
currency code and rejects mixed-currency arithmetic and comparisons.
