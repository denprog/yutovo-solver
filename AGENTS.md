# Yutovo Solver Agent Notes

## Project
WebSocket-based calculation backend for the Yutovo editor.

## Symbolic Integration
- `ResultType::SYMBOLIC_REAL`, `SYMBOLIC_RATIONAL`, `SYMBOLIC_COMPLEX` added in `types.h` (old `SYMBOLIC` removed).
- `CalculatorSolver` has `symbolic_parser` and `SolveSymbolicReal()`, `SolveSymbolicRational()`, `SolveSymbolicComplex()`.
- `AUTO` mode tries symbolic types last in `results_order[8]`.
- `CMakeLists.txt` links `libmpfr.a`, `libgmp.a`, `${SYMENGINE_LIBRARIES}`.

## Build
Use `-j16` maximum for building to avoid OOM kills.
