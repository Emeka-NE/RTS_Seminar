# M3 — Critical Evaluation and Transfer
**Topic:** Priority Inheritance Protocol (Resource Access Protocols)
**Author:** Nnaemeka Nnachi-Egwu | nnaemeka.nnachi-egwu@stud.hshl.de
**Primary source:** Buttazzo, Ch. 7 pp. 205–249. Secondary: [WCET], [GP], [Ko].

> Working notes. Structured critique, counterexamples, transfer analysis. Not final prose.

---

## 1. Concrete strengths — textual evidence from Buttazzo

### S1. Bounds priority inversion duration (Theorem 7.2, p. 219)
- Without a protocol, the inversion window [t3, t6] in Fig. 7.4 is *unbounded* — any number of medium-priority tasks can extend it. PIP bounds it to at most α_i = min(l_i, s_i) critical sections.
- **Buttazzo textual evidence:** "when task τ_i blocks one or more higher-priority tasks, it temporarily assumes (inherits) the highest priority of the blocked tasks. This prevents medium-priority tasks from preempting τ_i." (§7.6, p. 214)

### S2. Source transparency — no code change required (Table 7.5, p. 247–248)
- `pi_wait(s)` / `pi_signal(s)` are drop-in replacements for `wait(s)` / `signal(s)`. The task *code* does not change; only kernel behavior changes.
- **Buttazzo:** "A transparent protocol (like NPP and PIP) can be implemented without modifying the task code... legacy applications developed using classical semaphores (prone to priority inversion) can also be executed under a transparent protocol." (§7.10, p. 247)
- PCP and SRP are *not* transparent — they require task code to declare resource ceilings via a new system call.

### S3. Low pessimism — blocks only at actual resource contention (Table 7.5, §7.5→§7.6 transition)
- HLP blocks a task at its activation time when it *attempts to preempt*, even if the resource in contention is in a branch of the code that is never actually taken at runtime.
- PIP defers the blocking condition "at the entrance of a critical section rather than at the activation time" (p. 214). This removes the conditional-branch pessimism.
- **Consequence:** blocking time under PIP is often *smaller in practice* than the theoretical worst-case suggests, even if the worst-case bound (Eq 7.11) is higher than HLP's.

### S4. Transitive inheritance handles nested critical sections correctly
- For three tasks with a nested CS chain (Fig. 7.10, p. 217–218), priority propagates transitively up the blocking chain. This prevents an intermediate holder from being preempted while holding a resource its blocker depends on.
- **Buttazzo:** "Priority inheritance is transitive; that is, if a task τ_3 blocks a task τ_2, and τ_2 blocks a task τ_1, then τ_3 inherits the priority of τ_1 via τ_2." (§7.6.1, p. 215)

### S5. Feeds directly into schedulability analysis (§7.9, Eqs 7.19–7.22)
- B_i from Eq (7.11) can be plugged directly into Liu–Layland, RTA, or Hyperbolic tests. This is compatible with the rest of the fixed-priority scheduling framework.

---

## 2. Limitations — precise technical terms with counterexamples

### L1. Does NOT prevent deadlock (§7.6.5, pp. 226–227)

**Minimal counterexample** (Buttazzo Fig. 7.13, p. 226):
```
Tasks: τ_1 (high), τ_2 (low). Semaphores: S_a, S_b.
τ_1 code: wait(S_a); wait(S_b); ... signal(S_b); signal(S_a)
τ_2 code: wait(S_b); wait(S_a); ... signal(S_a); signal(S_b)
```
Timeline:
- t1: τ_2 locks S_b.
- t2: τ_1 preempts τ_2.
- t3: τ_1 locks S_a (free). Then tries wait(S_b) at t4 → blocked. τ_2 inherits P_1.
- t5: τ_2 (running at P_1) tries wait(S_a) → **blocked** — deadlock.

**Why PIP cannot prevent it:** PIP only boosts the holder's priority; it never *denies* a lock request on a free semaphore. The deadlock arises from the lock-acquisition *order*, not from priority levels. Buttazzo: "the deadlock does not depend on the Priority Inheritance Protocol but is caused by an erroneous use of semaphores." (p. 227). The fix is **total ordering** of semaphore acquisition, or replacing PIP with PCP.

**Severity assessment:** In a safety-critical system, a deadlock without a watchdog/recovery mechanism causes an infinite blocking — more dangerous than even unbounded priority inversion. Systems requiring certified deadlock freedom *cannot* rely on PIP alone.

---

### L2. Does NOT prevent chained blocking (§7.6.5, p. 225)

**Minimal counterexample** (Buttazzo Fig. 7.12, pp. 225–226):
```
Tasks: τ_1 (high), τ_2 (medium), τ_3 (low).
τ_1 needs S_a then S_b (sequentially).
τ_2 holds S_b.
τ_3 holds S_a.
Suppose at the time τ_1 arrives: τ_3 is in critical section with S_a, τ_2 preempted τ_3, 
then τ_2 locked S_b and is in its CS.
```
When τ_1 arrives and runs:
- wait(S_a) → blocked by τ_3 (direct blocking, τ_3 inherits P_1).
- τ_3 exits S_a, τ_1 resumes.
- wait(S_b) → blocked by τ_2 (second blocking).
- **Result: τ_1 blocked TWICE**, for the duration of *two* critical sections.

In the worst case, if τ_1 accesses n distinct semaphores each held by a different lower-priority task, τ_1 is blocked n times (p. 225). This is the chained blocking: Buttazzo's α_i = min(l_i, s_i) from Theorem 7.2 is the exact formal bound on this chain length.

**Severity assessment:** PCP prevents this because its lock-grant rule ensures that once τ_1 enters its first critical section, no lower-priority task can hold a semaphore that τ_1 needs (Theorem 7.4). PIP lacks this preemptive admission mechanism.

---

### L3. Transitive inheritance increases kernel implementation complexity

- When a holder τ_j is itself blocked on another semaphore, the inheritance chain must be followed transitively: the kernel must traverse the `lock` field of each TCB up the chain and boost each intermediate holder (§7.6.4, p. 224).
- Buttazzo explicitly states PIP is **"hard"** to implement (Table 7.5, p. 248) — the only protocol in the table with that rating.
- The `pi_wait(s)` pseudocode (§7.6.4, p. 224) includes "If such a task is blocked on another semaphore, the transitivity rule is applied." — this requires an unbounded traversal in the worst case (chain length bounded only by the nesting depth of critical sections).
- **Implementation risk:** kernel traversal during a locked-semaphore operation must itself be atomic — increasing interrupt latency. In practice (e.g. Linux futex implementation with `FUTEX_WAIT_REQUEUE_PI`), chain depth is bounded (RTMUTEX_DEPTH_LIMIT = 48 in Linux kernel), which means PIP's formal guarantees may not fully hold on the actual implementation.

---

## 3. Worst-case blocking bound: tight or pessimistic?

### The bound (Eq 7.11): B_i = min(B_i^l, B_i^s)
- **Not tight.** Buttazzo explicitly states: "the blocking factors derived by this algorithm are not tight" (§7.6.3, p. 222). Reason: B_i^l may sum two CS guarded by the *same semaphore* (impossible per Lemma 7.4); B_i^s may sum two CS from the *same task* (impossible per Lemma 7.3).
- Exact computation: Rajkumar [Raj91] gives an exhaustive-search algorithm — exponential complexity O(2^n) in the worst case. Not practical for large task sets.
- Algorithm (Fig. 7.11) is O(mn²). This is feasible for small embedded systems (n,m in the tens), but becomes a concern as task set size grows.

### The δ_{j,k} assumption and WCET
- Eq (7.11) requires δ_{j,k} — the exact duration of each critical section. These are in turn *WCET* estimates for the code inside the critical section.
- **WCET paper critique [WCET]:** Abella et al. (2015) demonstrate that "none of the surveyed [WCET] methods is fully trustworthy on all accounts." Key issues:
  - **Hardware complexity:** caches, pipelines, and speculative execution make static timing analysis (SDTA) increasingly inaccurate for modern hardware.
  - **Measurement-based methods (MBDTA):** may miss worst-case inputs.
  - **Multicore shared resources** (shared caches, memory buses) create timing interference that is very difficult to bound tightly.
- **Consequence for PIP:** if δ_{j,k} is underestimated (i.e., the critical section's WCET is larger than assumed), the computed B_i is too small, and the schedulability test (Eq 7.19/7.22) becomes unsound — a task declared schedulable may actually miss its deadline.
- This is not a flaw in PIP per se, but it means the entire B_i computation pipeline is only as trustworthy as its WCET inputs. For safety-critical certification, every δ_{j,k} must itself be conservatively computed.

---

## 4. Scope limitation: PIP is uniprocessor-only

- **Formal scope:** Buttazzo Chapter 7 explicitly assumes a single processor (§7.3, p. 209). All proofs of Lemmas 7.1–7.5 and Theorem 7.2 rely on the following invariant: *at any time, at most one task is in a critical section for a given resource*, because only one task runs at a time and the semaphore is binary. This invariant does not hold on a multiprocessor.

- **What breaks on multiprocessors:**
  - Two tasks on two processors can simultaneously attempt `wait(S_k)`. The first succeeds and holds the resource; the second blocks. But the blocked task is now running on a different processor than the holder. Boosting the holder's priority means the holder's scheduler on *its processor* must be notified via an inter-processor interrupt (IPI), which adds non-deterministic latency.
  - Tasks can migrate between processors (under Global scheduling), so the blocking chain may involve tasks on different processors at different times — the chain traversal in `pi_wait` becomes distributed and requires atomic reads/writes across CPUs.

- **Reference [GP]:** Gracioli et al. (2013) implement and measure Global-EDF and Partitioned-EDF in a real RTOS. Their experiments show that OS implementation aspects (IPI latency, scheduling overhead, cache coherence delay) significantly impact schedulability on multiprocessors. The overhead model they develop (Table 1 of [GP]) has no equivalent in Buttazzo's uniprocessor analysis — PIP's blocking bound formula would need to be extended with migration delays and IPI costs.

- **Existing multiprocessor extensions:** MPCP (Multiprocessor PCP, Rajkumar 1990), FMLP (Flexible Multiprocessor Locking Protocol), MSRP (Multiprocessor SRP). These are out of scope for this seminar but ground the point that PIP on multiprocessors requires substantial redesign.

---

## 5. Transfer analysis

### Where PIP works (suitable scenarios)
1. **Single-processor embedded RTOS with few shared resources and no nesting.** Nesting = 0 means Lemma 7.2 eliminates transitive inheritance complexity; α_i = min(l_i, s_i) is small; B_i is easily computable and bounded.
2. **Legacy POSIX codebase** migrating to real-time constraints: `PTHREAD_PRIO_INHERIT` is a one-line change in the mutex initialization code. No restructuring of lock ordering required.
3. **Automotive ECU on a single core** (OSEK/AUTOSAR): bounded blocking time is provable; critical section WCETs for low-complexity automotive SW are well-characterised; deadlock prevented by design convention (total semaphore ordering by resource ID).
4. **Scenarios where deadlock is provably impossible:** if all tasks acquire semaphores in a fixed total order (e.g. by semaphore ID), deadlock is impossible by design. PIP then only needs to handle the remaining chained-blocking case, which Thm 7.2 bounds.

### Where PIP is unsuitable (transfer failures)
1. **Multi-core SoCs or multi-processor systems:** protocol semantics break (see §4 above). Use MPCP, FMLP, or SRP+partitioned-scheduling instead.
2. **Systems that need certified deadlock freedom** (DO-178C, IEC 61508, ISO 26262 high ASIL): PIP alone is not sufficient. PCP or SRP is required, or a static analysis tool must be used to prove absence of lock-cycle patterns.
3. **Highly nested critical sections:** transitive inheritance chains become long; each chain traversal in `pi_wait` increases kernel lock-holding time and interrupt latency. Implementation complexity is rated "hard" (Table 7.5).
4. **Dynamic-priority (EDF) systems:** PIP was designed for fixed-priority scheduling. Spuri's EDF extension [Spu95] exists (footnote 2, p. 209) but is not in Buttazzo Ch. 7. For EDF systems, SRP is the standard answer.
5. **Systems where critical-section WCETs cannot be reliably bounded** (modern hardware with caches, pipelines, shared memory buses — see WCET paper [WCET]): the B_i computation is unsound if δ_{j,k} inputs are unreliable. In such cases the entire schedulability analysis must be qualified with additional measurement-based validation.

---

## 6. Proposed extensions — concrete and achievable

### E1. POSIX validation experiment
- Implement the 3-task scenario from Deliverable 4 using `pthread` with and without `PTHREAD_PRIO_INHERIT`, running under `SCHED_FIFO` on a Linux real-time system.
- Measure actual blocking time for τ_1 in both cases. Compare to theoretical B_1 = 2.
- This would confirm that Eq (7.11)'s bound is indeed conservative in practice (the actual blocking is often less than α_i · max_δ).

### E2. UPPAAL verification of transitive inheritance edge cases
- Extend the UPPAAL model (see Deliverable 3) to include a second shared resource and nested critical sections.
- Verify Lemma 7.2 (transitivity only with nesting) and the three-step transitive chain of Fig. 7.10.
- Check that the model replicates the deadlock scenario of Fig. 7.13, verifying that A[] not deadlock *fails* without total semaphore ordering.

### E3. Tight blocking-time computation
- Implement Rajkumar's [Raj91] exponential-search algorithm for small task sets (n ≤ 8) in Python or C++.
- Compare its results against Eq (7.11)'s pessimistic upper bound on the Table 7.1 example and on the Deliverable 4 task set.
- This would quantify the pessimism gap in practice.

### E4. Multiprocessor PIP analysis
- Formalise the additional overhead terms (IPI latency, migration delay) needed to extend B_i to a dual-core scenario.
- Use [GP] paper's overhead model as starting point; derive a modified blocking formula and check whether τ_1, τ_2, τ_3 from Deliverable 4 are still schedulable on 2 processors with Global-EDF + MPCP.

---

## 7. Self-checklist (M3 checklist from seminar_milestone_checklist.json)
- [x] Concrete strengths, not generic advantages (§1: Thm 7.2, transparency, low pessimism)
- [x] Concrete limitations with formal counterexamples (§2: deadlock Fig 7.13, chained blocking Fig 7.12, transitive chain complexity)
- [x] Scalability/runtime: O(mn²) algorithm, transitive chain traversal overhead, kernel lock time (§3, §2.L3)
- [x] Realistic embedded systems applicability (§5: single-core RTOS ✓, multicores ✗, automotive ✓)
- [x] Suitable scenario identified (§5)
- [x] Unsuitable scenario identified (§5: multicore, EDF, certified deadlock-freedom)
- [x] Meaningful extensions proposed (§6: POSIX experiment, UPPAAL nesting, tight bound, multiprocessor)
