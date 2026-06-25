# Yutovo Solver Agent Notes

## Project
WebSocket-based calculation backend for the Yutovo editor.

## Symbolic Integration
- `ResultType::SYMBOLIC_REAL`, `SYMBOLIC_RATIONAL`, `SYMBOLIC_COMPLEX` added in `types.h` (old `SYMBOLIC` removed).
- `CalculatorSolver` has `symbolic_parser` and `SolveSymbolicReal()`, `SolveSymbolicRational()`, `SolveSymbolicComplex()`.
- `AUTO` mode tries symbolic types last in `results_order[8]`.
- On Linux GMP/MPFR are inherited transitively from `yutovo-calculator`; `src/CMakeLists.txt` calls `pkg_check_modules(mpfr REQUIRED IMPORTED_TARGET mpfr)` and `pkg_check_modules(gmp REQUIRED IMPORTED_TARGET gmp)` to satisfy the imported target's interface.

## Build
Use `-j16` maximum for building to avoid OOM kills.
