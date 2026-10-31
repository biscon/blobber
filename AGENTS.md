# Blobber project guidelines

This is a C++17/raylib 2D grid-based dungeon engine. Preserve its prepared-image
rendering approach, fixed 1920×1080 presentation, and discrete movement unless
the task explicitly changes them.

## Design before implementation

- At the start of a task, read the relevant code and [system design](docs/system_design.md). Choose appropriate abstractions, data structures, and algorithms before implementing system changes. If the document doesn't exist yet, create it.
- Document new or changed system designs in `docs/system_design.md` before implementation, and update the document to match the final result. Explain responsibilities, interfaces, ownership, data layout, and important performance tradeoffs in proportion to the change.
- Give detailed designs their own Markdown files under `docs/` and link them from `docs/system_design.md`. Keep that document a useful overview and index. Clearly distinguish proposed designs from implemented systems.
- Prefer small, useful abstractions with clear boundaries. Generalize around concrete needs; avoid speculative frameworks, unnecessary layers, or exhaustive design documents for trivial fixes. Routine fixes can reference an existing design without creating a new one.

## Performance and memory

- Choose algorithms and data layouts around the actual workload, access patterns, and expected scale. Prefer contiguous storage, locality, batching, and bounded work where appropriate.
- Avoid trips through the system memory allocator during steady-state frame processing. Preallocate and reuse containers, pools, arenas, scratch storage, and upload buffers as appropriate. Watch for hidden allocations in container growth, temporary strings, callbacks, and per-object heap storage.
- Loading, scene preparation, diagnostics, and image export may allocate outside the normal frame path. Keep these phases explicit. Test engine-owned frame processing for allocation regressions; do not claim that graphics drivers or third-party internals never allocate.
- Define capacities, lifetimes, and exhaustion behaviour for bounded storage. Handle growth deliberately outside critical frame work where possible; do not silently replace allocation stalls with unbounded memory use.
- Prefer typed handles to long-lived pointers for resources and objects managed by a system. Use generation checks when slots can be recycled, and define invalid-handle behaviour. Short-lived pointers and references are appropriate for direct access when ownership and lifetime are clear; do not add handle lookups inside tight loops unnecessarily.

## Modularity and implementation quality

- Keep application entry points such as `main` focused on startup, shutdown, and orchestration. Put subsystem implementation in cohesive source/header files, and split growing files when distinct responsibilities make them difficult to navigate. Use practical judgment: related types and functions can share a file; avoid rigid one-class-per-file rules or fragmentation that makes the code harder to follow.
- Never let OOP conventions, inheritance hierarchies, design patterns, or abstraction purity dictate a slower implementation. Use simple structs, functions, composition, or classes according to the problem. Introduce indirection or virtual dispatch only when it provides a concrete benefit at an acceptable cost.
- Keep systems cohesive and interfaces small. Separate world data/loading, movement, input, asset preparation, scene composition, UI, platform resources, and application orchestration. Add persistence or entity systems when there is a concrete need, not speculatively.
- Keep ownership and lifetimes explicit. Use RAII for resource management where appropriate; performance is not a reason to accept leaks, dangling references, races, or undefined behaviour.
- Prefer straightforward, maintainable code and existing facilities when they meet the performance requirements. Avoid cleverness and custom infrastructure without a demonstrated need.
- Validate changes with focused checks appropriate to the affected behaviour. For rendering changes, inspect deterministic captures and check the raylib/OpenGL integration when a desktop is available. Document material limitations and keep design notes current.

## Resource use and verification cost

- Keep the user's desktop responsive. Never saturate the processor, exhaust memory
  or fill temporary storage with builds, tests, captures or benchmarks.
- Run only one resource-intensive job at a time. This limit applies across build
  configurations, test processes, sanitizers, asset preparation and background jobs.
  Default to at most two build workers total and one test process; reduce further
  when necessary. Never use unbounded `-j` or derive worker counts from all CPUs.
- Run lengthy commands at low CPU and I/O priority (`nice -n 15` and, on Linux,
  `ionice -c 3`). Keep internal library thread counts bounded too. Check available
  memory and scratch space before costly work; put large scratch output on a disk
  with space instead of filling `/tmp` or the workspace. Do not remove unrelated files.
- Use focused tests during development. Choose tests from the changed behavior
  and its callers, and state that selection; a passing count is not a reason to
  run another configuration. Do not rerun the full Debug/Release,
  graphics/headless, sanitizer and GPU matrix after minor edits. Run broader checks
  once at a meaningful integration checkpoint, only for affected configurations;
  rerun passing checks only when subsequent changes or failures justify it.
- Benchmarks and capacity stress are opt-in investigations, not routine correctness
  checks. State the concrete performance question and bounded workload first. Do
  not run them merely because a historical slice or checklist ran them. Prefer a
  short representative check, and stop once it answers the question.
- Treat a slow routine test as a problem to investigate. Reduce repeated fixture
  generation, oversized data and redundant work while preserving the failure or
  invariant being tested. Keep intentional expensive tests explicitly labelled
  and outside the ordinary development loop. Report skipped or interrupted work
  honestly; do not rerun expensive work just to reproduce a passing count.

## Testing

- Aim for thorough, meaningful coverage of our behaviour and invariants: normal use, boundary cases, failure paths, resource lifetimes, and interactions between our systems. Add regression tests for bugs where they provide lasting value.
- Test our code and our integration with dependencies, not the internal correctness of third-party libraries. Avoid trivial tests, assertions that merely repeat the implementation, and tests added only to increase counts or coverage percentages.
- Keep routine tests fast. For expensive computations, use small representative maps, generated materials, and selected movement/visibility cases. Preserve the conditions that exercise the behaviour under test; a reduced workload must not eliminate the bug or boundary case being checked.
- Keep larger stress tests and performance benchmarks separate from the routine correctness suite. Run them when scale, synchronization, or performance is relevant to the change; small tests do not establish production-scale performance.
- Tests must own their fixtures. Use deterministic generated data or small dedicated fixtures maintained with the tests, and isolated temporary directories for file output. Do not depend on game models, materials, textures, world saves, or other content that may change or disappear.
- Prefer assertions about observable results and invariants over implementation details. Use fixed seeds and justified numerical tolerances where appropriate; do not assume identical floating-point results across GPU devices without a defined guarantee.
- Keep CPU correctness and allocation tests runnable without a window. Exercise GPU resource lifecycle and presentation separately when a desktop is available. Report unavailable hardware or skipped checks explicitly rather than treating unexecuted tests as passing.
- Record the relevant test strategy in each system design and run the checks appropriate to a change before considering it complete.

## Dependencies

Prefer compatible system packages when available. Keep a pinned, reproducible
fallback for small dependencies such as raylib so a clean CLion/CMake checkout
builds predictably. Do not install system packages or change toolchains
unnecessarily; follow the execution environment's permission rules when needed.

## Desktop verification restriction

- Do not control the user's mouse or keyboard, switch window focus or desktops,
  or run desktop input automation. This is an explicit user instruction.
- Use command-line/headless checks. Leave interactive desktop review to the user;
  report it as unverified rather than attempting automation.

- For the approved dungeon-first UI revision, the user authorizes running the game
  and exporting application-owned screenshots. Prefer hidden capture windows and
  in-process scripted input; never inject OS input or select/control other windows.
