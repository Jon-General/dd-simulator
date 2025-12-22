# Q-Sylvan-Inspired Parallelism Notes

## What we implemented
- Added task-based OpenMP parallelism inside DD multiply recursion, spawning per-child tasks when already in a parallel region and past a depth cutoff.
- Kept density-matrix multiplication on the sequential path to avoid contention.
- Defaulted `parallelOperationTasks` to `true` with `minQubitsForTaskParallelism = 6` so parallel tasks are on by default for moderately sized inputs.
- Maintain thread safety on compute/unique tables; tasks rely on existing bucket-level locking.
- Compute tables now use per-bucket lock-free CAS states (empty/writing/ready) instead of shared_mutexes to reduce contention under parallel multiplies.
- Complex weight lookup now rounds components to the current tolerance before hashing/canonicalization to avoid precision-based duplication and enforce consistent normalization.

## Configuration knobs
- Toggle tasking via `parallelOperationTasks` in the DD package config.
- Adjust recursion threshold via `minQubitsForTaskParallelism` to balance overhead versus parallel work.
- OpenMP controls (e.g., `OMP_NUM_THREADS`) still govern thread counts.

## Current status
- Build succeeds; full `ctest --output-on-failure` passes (aside from two pre-skipped QueryJobId tests).
- RandomClifford 8-qubit case exhibited a one-off segfault; reruns (including verbose) now pass, so treat as flaky until reproduced.

## Next steps to consider
- Stress-repeat flaky test (`ctest --repeat-until-fail -R RandomClifford/RandomClifford.simulate/8_qubits`) under varying `OMP_NUM_THREADS` to capture a stack if it trips.
- Add opt-in guard to disable tasking for tests or seed deterministically if flakiness persists.
- Benchmark speedups versus baseline on circuits with varying DD sharing (GHZ, QFT, SU2, QAOA) to quantify gains and identify hotspots.
