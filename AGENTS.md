# Yutovo Solver Agent Notes

## Project
WebSocket-based calculation backend for the Yutovo editor.

## Symbolic Integration
- `ResultType::SYMBOLIC` added in `types.h`.
- `CalculatorSolver` has `symbolic_parser` and `SolveSymbolic()`.
- `AUTO` mode tries `SYMBOLIC` last in `results_order[6]`.
- `CMakeLists.txt` links `libmpfr.a`, `libgmp.a`, `${SYMENGINE_LIBRARIES}`.

## Build
Use `-j16` maximum for building to avoid OOM kills.
