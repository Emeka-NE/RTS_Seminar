# C++ PIP Simulator

**File:** `pip_simulator.cpp`
**Language:** C++17
**Reference:** Buttazzo §7.6.4 (pi_wait/pi_signal pseudocode, pp.224-225)

## Task Set
- τ₁: C=1, T=4, D=4, prio=1, uses S_R (CS len=1)
- τ₂: C=2, T=6, D=6, prio=2, no shared resource
- τ₃: C=4, T=16, D=16, prio=3, uses S_R (CS len=4)

## Build & Run
```
make        # builds ./pip_simulator
make run    # builds and runs
make clean  # removes binary
```

## Expected Output
Two scenarios (WITHOUT PIP, WITH PIP) over t=0..19.
- Without PIP: τ₂ preempts τ₃ at priority inversion window; τ₁ blocked 5 units.
- With PIP: τ₃ inherits P₁; τ₂ blocked; τ₁ blocked only 3 units ≤ B₁=4.
All tasks meet deadlines under PIP.

## Key Functions
- `inherit_up()`: transitive priority propagation (Buttazzo Eq 7.8)
- `restore_prio()`: priority restoration after pi_signal (§7.6.4 p.225)
- `pi_wait()`: semaphore acquisition with inheritance
- `pi_signal()`: semaphore release with restoration and waiter wakeup
