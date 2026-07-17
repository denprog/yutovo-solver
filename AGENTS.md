# Yutovo Solver Agent Notes

## Project
WebSocket-based calculation backend for the Yutovo editor.

## Symbolic Integration
- `ResultType::SYMBOLIC_REAL`, `SYMBOLIC_RATIONAL`, `SYMBOLIC_COMPLEX` added in `types.h` (old `SYMBOLIC` removed).
- `CalculatorSolver` has `symbolic_parser` and `SolveSymbolicReal()`, `SolveSymbolicRational()`, `SolveSymbolicComplex()`.
- `AUTO` mode tries symbolic types last in `results_order[8]`.
- On Linux GMP/MPFR are inherited transitively from `yutovo-calculator`; `src/CMakeLists.txt` calls `pkg_check_modules(mpfr REQUIRED IMPORTED_TARGET mpfr)` and `pkg_check_modules(gmp REQUIRED IMPORTED_TARGET gmp)` to satisfy the imported target's interface.

## Code Style
- Comments inside functions: keep on a single line if they fit, if a comment is a single sentence, start with a lowercase letter and omit the trailing period.
- Do not add a space after `//`.
- Keep expressions that fit on a single line inside `if`, `for`, `while`, and function calls on one line; only split them when they are too long.
- When splitting an expression across lines, indent the continuation by 4 spaces.

## Build
- Use `-j16` maximum for building to avoid OOM kills.
- For Emscripten builds activate the toolchain first: `source ~/emsdk/emsdk_env.sh`.
- Web build directories are `build_web/debug` and `build_web/release`.
- Native Linux build directories are `build/debug` and `build/release`.
