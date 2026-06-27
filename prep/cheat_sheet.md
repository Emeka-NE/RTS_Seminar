# PIP Cheat Sheet — Oral Exam Preparation
**Topic:** Priority Inheritance Protocol (Resource Access Protocols)
**Source:** Buttazzo, Hard Real-Time Computing Systems, 3rd ed., Ch.7

---

## Core Definitions (with page refs)

| Term | Definition | Page |
|------|-----------|------|
| Real-time system (Kopetz) | Correctness depends on logical result AND the physical instant it's produced | Slide 7 |
| Priority inversion | High-priority task blocked by low-priority holder while medium-priority tasks run freely | p.207 |
| Nominal priority Pᵢ | Fixed priority assigned by RM/DM rule | p.209 |
| Active priority pᵢ | Current (possibly boosted) priority | p.209 |
| Direct blocking | τᵢ blocked because lower-prio task holds resource τᵢ needs | p.216 |
| Push-through blocking | τᵢ blocked because lower-prio task was boosted above Pᵢ | p.216-217 |
| Ceiling C(Sk) | Max nominal priority of tasks using Sk | p.214 |
| γᵢ | Set of CS that can block τᵢ: {Z_{j,k} | Pj < Pᵢ, C(Sk) ≥ Pᵢ} | p.221 |
| α_i = min(l_i, s_i) | Max # of CS that block τᵢ (Theorem 7.2) | p.219 |

---

## PIP Algorithm (Buttazzo §7.6.4, pp. 224-225)

**pi_wait(s):**
1. If s is free → lock it
2. Else → block caller; add to s.queue; **inherit**: holder gets max(holder.prio, caller.prio); apply transitively up the chain

**pi_signal(s):**
1. Unlock s; wake highest-priority waiter (if any)
2. **Restore**: holder's prio = max(nominal, max prio of remaining blocked tasks)

**Key equation (Eq 7.8):**
`p_j(R_k) = max{P_j, max{P_h | τ_h blocked on R_k}}`

---

## Lemmas & Theorems (all in Buttazzo §7.6)

| # | Statement | Key condition | Page |
|---|-----------|--------------|------|
| Lemma 7.1 | S_k causes push-through blocking to τᵢ if S_k is used by some τ_h (P_h > P_i) and some τ_j (P_j < P_i) | Both a higher AND lower priority task use S_k | p.218 |
| Lemma 7.2 | Transitivity only occurs with NESTED critical sections | Nesting required | p.218 |
| Lemma 7.3 | τᵢ blocked at most l_i times | l_i = # lower tasks that can block τᵢ | p.219 |
| Lemma 7.4 | τᵢ blocked at most s_i times | s_i = # semaphores | p.219 |
| **Theorem 7.2** | τᵢ blocked at most α_i = min(l_i, s_i) critical sections | — | p.219 |
| Lemma 7.5 | Z_{j,k} blocks τᵢ only if P_j < P_i ≤ C(S_k) | Ceiling must be ≥ Pi | p.221 |

---

## Blocking Formula (Buttazzo Eqs 7.9-7.11)

```
B_i^l = max sum of CS durations along l_i longest-blocking-task dimension
B_i^s = max sum of CS durations along s_i semaphore dimension
B_i   = min(B_i^l, B_i^s)

Note: Buttazzo uses (δ_{j,k} - 1) in discrete time; continuous: full δ_{j,k}
Note: This bound is NOT tight (Lemma 7.4 pessimism admitted on p.222)
Algorithm complexity: O(mn²)
```

---

## Schedulability Tests (Buttazzo §7.9)

**Liu-Layland with blocking (Eq 7.19) — SUFFICIENT ONLY:**
```
For each i: Σ_{h<i}(C_h/T_h) + (C_i+B_i)/T_i  ≤  i·(2^{1/i}−1)
```

**RTA — EXACT (Eq 7.22):**
```
R_i = C_i + B_i + Σ_{h<i} ⌈R_i/T_h⌉·C_h   (iterate to fixed point)
Task schedulable iff R_i ≤ D_i
```

---

## Application Example (Task Set)

| Task | C | T  | D  | Prio | CS on R? | δ |
|------|---|----|----|------|----------|---|
| τ₁   | 1 | 6  | 6  | 1    | YES      | 1 |
| τ₂   | 2 | 8  | 8  | 2    | NO       | — |
| τ₃   | 4 | 24 | 24 | 3    | YES      | 4 |

**Results:** B₁=4, B₂=4, B₃=0
**RTA:** R₁=5≤6 ✓, R₂=8≤8 ✓, R₃=8≤24 ✓ — all schedulable

**Trace with PIP:** τ₃ at P₃, τ₁ blocks t=2, τ₃ inherits P₁, τ₂ blocked by push-through, τ₃ CS ends t=4, τ₁ unblocked. Blocking = 2 ≤ B₁=4 ✓

---

## Five-Protocol Comparison (Table 7.5, p.248)

| Property       | NPP | HLP | **PIP** | PCP | SRP |
|----------------|-----|-----|---------|-----|-----|
| Deadlock-free  | ✓   | ✓   | **✗**   | ✓   | ✓   |
| Chained block  | ✗   | ✗   | **YES** | ✗   | ✗   |
| Transparent    | ✓   | ✗   | **✓**   | ✗   | ✗   |
| Max blockings  | 1   | 1   | **αᵢ**  | 1   | 1   |
| Impl. effort   | Easy| Easy| **Hard**| Med | Easy|
| Dynamic prio   | ✗   | ✗   | ✗       | ✗   | **✓**|

---

## Limitations (for critical questions)

1. **No deadlock prevention:** Two tasks acquiring two semaphores in reverse order → lock cycle. Fix: total ordering by semaphore ID, or use PCP.
2. **Chained blocking:** τ₁ accesses n semaphores each held by different lower tasks → blocked n times. PCP prevents this.
3. **Hard to implement:** Transitive chain traversal in `pi_wait` must be atomic. Linux bounds chain depth (RTMUTEX_DEPTH_LIMIT=48).
4. **WCET inputs:** B_i only as good as δ_{j,k}. Abella 2015: "none of the surveyed WCET methods is fully trustworthy."
5. **Uniprocessor only:** IPI latency on multiprocessors breaks PIP's timing assumptions.

---

## UPPAAL Model Summary

- **Templates:** Task (Idle, Ready, Running, CriticalSection, Blocked), ResourceManager (Free, Held, Granting), Observer
- **Shared vars:** `int[1,9] act_prio[3]`, `int[-1,2] holder`, `int[0,20] block_dur[3]`
- **Channels:** `lock_req[N]!`, `lock_ack[N]?`, `unlock_rel[N]!`
- **Key queries:**
  - `A[] not deadlock` — no lock cycle possible (1 resource, consistent order)
  - `A[] (blocking_tau1 imply not (tau2.Running and not blocked_tau2))` — PIP prevents inversion
  - `A[] (block_dur[0]<=4 && block_dur[1]<=4 && block_dur[2]==0)` — blocking within bounds
  - `E<> (act_prio[2]==1)` — inheritance is reachable (non-vacuous)

---

## Likely Oral Exam Questions and Key Answers

**Q: What is the difference between direct blocking and push-through blocking?**
A: Direct blocking: τᵢ waits because a lower-priority task holds its semaphore. Push-through: τᵢ is delayed because a lower-priority task was boosted above Pᵢ, causing it to preempt later. Both are bounded by PIP (Theorem 7.2).

**Q: PIP doesn't prevent deadlock. Why not, and what is the fix?**
A: PIP only raises priorities — it never denies a lock on a free semaphore. So if τ₁ holds S_a and requests S_b (held by τ₂), and τ₂ holds S_b and requests S_a, a cycle forms. PIP cannot detect it. Fix: PCP's lock-grant rule (task τ_i can only lock S_k if its priority exceeds the max ceiling of all currently locked semaphores).

**Q: Why does the Liu-Layland test fail for τ₂ but RTA says it's schedulable?**
A: LL is a sufficient but not necessary condition. It can say "INCONCLUSIVE" when the task set IS schedulable. RTA is exact — it computes the actual worst-case response time accounting for preemptions precisely, not just the utilization bound.

**Q: Why is PIP rated "Hard" to implement when NPP and SRP are "Easy"?**
A: The transitive inheritance chain in `pi_wait` must be traversed atomically. Each step must re-read the semaphore holder and boost it, then check if that holder is itself blocked, and so on. This chain can be arbitrarily long in theory, adding non-deterministic latency to a kernel operation that should be O(1).

**Q: When would you choose PIP over PCP?**
A: When: (1) transparency is needed (legacy code cannot add ceiling declarations), (2) deadlock is provably absent by design (semaphores acquired in total order), (3) overhead of PCP's pre-check is unacceptable. The Mars Pathfinder fix used `PTHREAD_PRIO_INHERIT` — one config line, no code change.

**Q: Is the blocking bound B_i tight?**
A: No. Buttazzo admits this on p.222. B_i^l can sum two CS from the same semaphore, which Lemma 7.4 says cannot simultaneously block τᵢ. The exact algorithm (Rajkumar, O(2^n)) exists but is impractical for large n.
