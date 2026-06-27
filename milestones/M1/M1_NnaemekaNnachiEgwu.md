# M1 — Technical Understanding
**Topic:** Priority Inheritance Protocol (Resource Access Protocols)
**Author:** Nnaemeka Nnachi-Egwu (nnaemeka.nnachi-egwu@stud.hshl.de)
**Course:** Real-Time Systems Seminar, HSHL — Prof. Dr. Stefan Henkler
**Primary source:** G. C. Buttazzo, *Hard Real-Time Computing Systems*, 3rd ed., Springer, 2011, Chapter 7 (pp. 205–249).

> Working notes. Bullets, tables, pseudocode, annotated equations. Not polished prose, not IEEE format.
> Every numbered theorem/lemma/equation below was checked against the uploaded Buttazzo text.

---

## 0. Notation (professor's convention + Buttazzo §7.3)

From the introduction slides — use throughout:
- Job `J_i`: `a_i` arrival, `C_i` computation, `d_i` absolute deadline, `s_i` start, `f_i` finish. [Intro slide 24]
- Periodic task `τ_i`: `φ_i` phase, `C_i`, `T_i` period, `D_i` relative deadline; k-th instance at `φ_i+(k−1)T_i`; usually `T_i = D_i`. [Intro slide 25]
- Semaphore: `wait(s)` locks, `signal(s)` unlocks; critical section = code under mutual exclusion. [Intro slides 29–30]
- Three-state model: READY →(scheduling)→ RUN →(preemption)→ READY; RUN →(wait on busy resource)→ WAITING →(signal)→ READY. [Intro slide 31]

Buttazzo §7.3 resource-protocol notation (p. 209–210):
- `P_i` = fixed **nominal** priority (e.g. assigned by RM). `p_i` = **active** (dynamic) priority, `p_i ≥ P_i`, initially `p_i = P_i`.
- `B_i` = maximum blocking time `τ_i` can experience.
- `z_{i,k}` = a critical section of `τ_i` guarded by `S_k`; `Z_{i,k}` = the **longest** such section; `δ_{i,k}` = its duration.
- `z_{i,h} ⊂ z_{i,k}` = nested (h entirely inside k).
- `σ_i` = set of semaphores used by `τ_i`.
- `σ_{i,j}` = semaphores that can block `τ_i`, used by lower-priority `τ_j`.
- `γ_{i,j} = {Z_{j,k} | (P_j < P_i) and (S_k ∈ σ_{i,j})}`  — Eq (7.1)
- `γ_i = ⋃_{j:P_j<P_i} γ_{i,j}` — Eq (7.2)

**Standing assumptions** (Buttazzo §7.3, p. 210): tasks have distinct priorities, listed `τ_1` highest; tasks do not self-suspend except on locked semaphores; critical sections are properly nested; semaphores are binary (one task at a time in a CS).

---

## 1. The priority inversion problem (§7.2, pp. 206–208)

**Two-task baseline (Fig. 7.3, p. 207).** `τ_1` high, `τ_2` low, share `R_k`. `τ_2` locks `S_k`, `τ_1` preempts, `τ_1` hits `wait(S_k)` → blocked, `τ_2` resumes and finishes its CS, then `signal(S_k)` unblocks `τ_1`.
- Max blocking here = duration of `τ_2`'s critical section. **This blocking is unavoidable** — direct consequence of mutual exclusion. (p. 207)

**Three-task scenario — the actual failure (Fig. 7.4, p. 208).** Three tasks `τ_1, τ_2, τ_3`, decreasing priority. `τ_1` and `τ_3` share `R` (binary semaphore `S`). `τ_2` shares **nothing** with `τ_1` or `τ_3`.
- `t0`: `τ_3` starts, locks `S`.
- `t2`: `τ_1` arrives, preempts `τ_3` inside its CS.
- `t3`: `τ_1` does `wait(S)` → **blocked** (direct blocking). `τ_3` continues in CS.
- `t4`: `τ_2` arrives, preempts `τ_3` (higher priority, no shared resource) and runs freely.
- **Priority inversion interval = [t3, t6]** — highest-priority `τ_1` waits for lower-priority `τ_2` and `τ_3`. (p. 208)

**Key sentence to internalize (p. 208):** the max blocking of `τ_1` depends not only on `τ_3`'s CS length but also on `τ_2`'s WCET — and on every intermediate task that can preempt `τ_3`. → **In general the duration of priority inversion is unbounded.**

Under EDF this is sometimes called *deadline inversion* (footnote 1, p. 208).

→ This unbounded inversion is the exact correctness failure PIP is designed to bound.

---

## 2. Terminology — types of blocking (§7.6.1 / §7.3)

- **Critical section** — code executed under mutual exclusion (`wait`…`signal`). (§7.1, p. 205)
- **Blocked** — a task waiting for an exclusive resource held by another task. (§7.1, p. 205)
- **Direct blocking** — a higher-priority task tries to acquire a resource already held by a lower-priority task. *Necessary* for consistency. (§7.6.1, p. 216)
- **Push-through blocking** — a medium-priority task is blocked by a low-priority task that has **inherited** a higher priority from a task it directly blocks. *Necessary* to avoid unbounded inversion. (§7.6.1, p. 216)
- **Ceiling blocking** — third blocking type, introduced only by **PCP** (not PIP): a task is blocked at lock-attempt because its priority is not above the highest ceiling of semaphores locked by others. (§7.7, p. 229)
- **Preemption blocking** — blocking induced because a high-priority task's *arrival* (not its resource request) is what triggers the priority raise; characteristic of arrival-time protocols (NPP, HLP). PIP avoids this by deferring the blocking condition to the *access* instant (§7.5 → §7.6 transition, p. 214).

The five protocols presented in Chapter 7 (p. 209): **NPP, HLP (=Immediate Priority Ceiling), PIP, PCP, SRP**.

---

## 3. PIP algorithm (§7.6.1, pp. 214–217)

Proposed by **Sha, Rajkumar, Lehoczky [SRL90]**. Idea: when `τ_i` blocks one or more higher-priority tasks it temporarily **inherits** the highest priority among the blocked tasks, so medium-priority tasks cannot preempt it and prolong the blocking. (p. 214)

**Protocol definition (p. 215):**
1. Tasks scheduled by **active** priority `p_i`. Ties → FCFS.
2. `τ_i` does `wait` on `R_k`: if `R_k` held by lower-priority `τ_j`, `τ_i` is **blocked by `τ_j`**; else `τ_i` enters its CS.
3. When `τ_i` blocks on a semaphore, it **transmits its active priority** to the holder `τ_j`; `τ_j` resumes its CS with `p_j = p_i`. In general, at every instant:

   **`p_j(R_k) = max{ P_j , max_h { P_h | τ_h is blocked on R_k } }`  — Eq (7.8)**

4. When `τ_j` does `signal` (exits CS): unlock, wake highest-priority blocked task. Update `p_j`: **if no other task is blocked by `τ_j` → `p_j := P_j` (nominal); else `p_j` := highest priority of tasks still blocked by it** (Eq 7.8).
5. **Inheritance is transitive**: if `τ_3` blocks `τ_2` and `τ_2` blocks `τ_1`, then `τ_3` inherits `τ_1`'s priority via `τ_2`.

**Trigger / inherit / restore summary:**
| event | what happens |
|---|---|
| HP task blocks on resource held by LP task | inheritance **triggers** |
| priority **inherited** | the highest active priority among tasks currently blocked on that resource (Eq 7.8) |
| holder executes `signal(s)` | **restore**: `p_j` → `P_j` if nobody else blocked by it, else → next highest blocked priority |
| holder itself blocked on another semaphore | **transitive** propagation up the chain (only possible with nested CS — Lemma 7.2) |

**Worked schedule (Fig. 7.8, p. 216), same task set as Fig. 7.4:**
- Until `t3`: identical to the no-protocol case (no inheritance yet).
- `t3`: `τ_1` blocked by `τ_3` → `τ_3` inherits `P_1`, runs remaining CS at top priority [t3,t5].
- `t4`: `τ_2` **cannot** preempt `τ_3` (τ_3 now at P_1) → no extra interference on `τ_1`.
- CS exit: `τ_1` awakened, `τ_3` restored to `P_3`; `t5` CPU → `τ_1`; `τ_2` only starts at `t6`.

Nested-CS example (Fig. 7.9, p. 216–217): shows a holder does **not** always resume the priority it entered with. Transitive example (Fig. 7.10, p. 217–218).

---

## 4. Theorems & lemmas in §7.6 (exact Buttazzo numbers)

> Verified against the text. (Theorem 7.1 belongs to **HLP** §7.5, included for context.)

- **Theorem 7.1 (HLP, §7.5.1, p. 213):** Under HLP a task `τ_i` can be blocked at most for the duration of a single critical section in `γ_i`. *(contrast: PIP allows up to `α_i` sections.)*

- **Lemma 7.1 (§7.6.2, p. 218):** `S_k` can cause **push-through** blocking to `τ_i` only if `S_k` is accessed both by a task with priority < `P_i` and a task with priority > `P_i`.

- **Lemma 7.2 (§7.6.2, p. 218):** **Transitive** inheritance can occur only in the presence of **nested** critical sections.

- **Lemma 7.3 (§7.6.2, p. 219):** If `l_i` lower-priority tasks can block `τ_i`, then `τ_i` can be blocked for at most `l_i` critical sections (one per lower task), regardless of #semaphores used by `τ_i`.

- **Lemma 7.4 (§7.6.2, p. 219):** If `s_i` distinct semaphores can block `τ_i`, then `τ_i` can be blocked for at most `s_i` critical sections (one per semaphore), regardless of #critical sections used.

- **Theorem 7.2 (Sha, Rajkumar, Lehoczky) (§7.6.2, p. 219):** Under PIP, `τ_i` can be blocked for at most the duration of **`α_i = min(l_i, s_i)`** critical sections. *(follows from Lemmas 7.3 + 7.4.)*

- **Lemma 7.5 (§7.6.3, p. 221):** With no nested CS, `z_{j,k}` of `τ_j` (guarded by `S_k`) can block `τ_i` only if **`P_j < P_i ≤ C(S_k)`**, where `C(S_k) = max_i {P_i | S_k ∈ σ_i}` — Eq (7.12).

- **Lemma 7.6 (PCP, §7.7.2, p. 230):** (context) Under PCP, if `τ_k` is preempted in CS `Z_a` by `τ_i` entering `Z_b`, then `τ_k` cannot inherit a priority ≥ that of `τ_i` until `τ_i` completes. *(This is the property that gives PCP its single-blocking + deadlock-freedom guarantees.)*

---

## 5. Worst-case blocking time computation (§7.6.3, pp. 219–223)

Exact `B_i` needs a combinatorial search over `α_i` sections in `γ_i`. Buttazzo gives a **simpler upper bound** (not tight):

- Per lower task (Lemma 7.3):
  **`B_i^l = Σ_{j:P_j<P_i} max_k { δ_{j,k} − 1 | Z_{j,k} ∈ γ_i }`  — Eq (7.9)**
- Per semaphore (Lemma 7.4):
  **`B_i^s = Σ_{k=1..m} max_j { δ_{j,k} − 1 | Z_{j,k} ∈ γ_i }`  — Eq (7.10)**
- Combine (Theorem 7.2):
  **`B_i = min(B_i^l , B_i^s)`  — Eq (7.11)**

`C(S_k) = max_i {P_i | S_k ∈ σ_i}` — Eq (7.12). With ceilings the terms become
`B_i^l = Σ_{j=i+1..n} max_k{δ_{j,k}−1 | C(S_k) ≥ P_i}` and `B_i^s = Σ_{k=1..m} max_{j>i}{δ_{j,k}−1 | C(S_k)≥P_i}`.

- The `−1` is a discrete-time artifact: `Z_{j,k}` must start strictly *before* `τ_i` arrives to block it (p. 213, 221).
- Algorithm (Fig. 7.11, p. 223): **complexity `O(m·n²)`**. Bounds are **not tight** (may sum two CS on the same semaphore, impossible by Lemma 7.4; Rajkumar [Raj91] gives an exact exponential-search algorithm).

**Buttazzo's own worked example (Table 7.1, p. 222) — verify I can reproduce it:**
4 tasks, 3 semaphores `S_a(P_1), S_b(P_1), S_c(P_2)`. `δ` table:
| | Sa | Sb | Sc |
|---|---|---|---|
| τ1 | 1 | 2 | 0 |
| τ2 | 0 | 9 | 3 |
| τ3 | 8 | 7 | 0 |
| τ4 | 6 | 5 | 4 |

- `B1^l = 8+7+5 = 20`, `B1^s = 7+8 = 15` → **B1 = 15**
- `B2^l = 7+5 = 12`, `B2^s = 7+6+3 = 16` → **B2 = 12**
- `B3^l = 5`, `B3^s = 5+4+3 = 12` → **B3 = 5**
- `B4^l = B4^s = 0` → **B4 = 0**

(`B2^l` adds two sections guarded by the same semaphore — this is exactly *why* the bound is pessimistic, p. 224.) ✔ reproduced.

---

## 6. Schedulability with blocking (§7.9, pp. 246–247)

All Chapter-4 tests extend by **inflating `C_i` with `B_i`**. Necessary-and-sufficient tests become **only sufficient** under blocking.

- **RM / Liu–Layland (Eq 7.19):** for all `i`: `Σ_{h:P_h>P_i} C_h/T_h + (C_i+B_i)/T_i ≤ i(2^{1/i} − 1)`
- **EDF (Eq 7.20):** `Σ_{h:P_h>P_i} C_h/T_h + (C_i+B_i)/T_i ≤ 1`
- **Hyperbolic (Eq 7.21):** `Π_{h:P_h>P_i}(C_h/T_h + 1) · ((C_i+B_i)/T_i + 1) ≤ 2`
- **Response Time Analysis (Eq 7.22):** `R_i^{(0)} = C_i+B_i`; `R_i^{(s)} = C_i + B_i + Σ_{h:P_h>P_i} ⌈R_i^{(s−1)}/T_h⌉ C_h` (exact).
- **Workload (Eq 7.23), Processor Demand with blocking function B(L) (Eqs 7.24–7.25, Baruah [Bar06])** — for EDF.

---

## 7. Comparison table — five protocols

**Authoritative source: Buttazzo Table 7.5 (p. 248).** I reorganized the requested columns; raw Table 7.5 values in the last three columns are verbatim.

| Protocol | Priority assign. | # blocking | Pessimism | Blocking instant | Prevents deadlock | Prevents chained blocking | Blocking-bound tightness | Implementation / transparency |
|---|---|---|---|---|---|---|---|---|
| **NPP** | any | 1 | high | on arrival | **YES** | YES | loose (over-blocks; long CS hurt all HP tasks) | easy / transparent |
| **HLP** (Imm. Priority Ceiling) | fixed | 1 | medium | on arrival | **YES** | YES | medium (blocks at activation, even if CS not taken) | easy / **not** transparent (needs ceilings) |
| **PIP** | fixed | **α_i = min(l_i,s_i)** | **low** | **on access** | **NO** | **NO** (chained blocking possible) | loose upper bound (Eq 7.11, `O(mn²)`, not tight) | **hard** / transparent |
| **PCP** | fixed | 1 | medium | on access | **YES** | **YES** | medium | medium / not transparent |
| **SRP** | any | 1 | medium | on arrival | **YES** | YES | medium | easy / not transparent |

Notes:
- "Transparency" (Table 7.5, p. 247): NPP and PIP need **no** task-code change (same primitives as classical semaphores) → attractive for COTS RTOS (e.g. VxWorks). HLP/PCP/SRP need an extra system call to declare ceilings.
- PIP is the **only** protocol in the table that does **not** prevent deadlock and **does** allow multiple/chained blocking (`α_i` sections instead of 1). That is the price of its low pessimism and transparency.

---

## 8. Application-example numbers (forward link to Deliverable 4) — verified

Task set: `τ_1`(C=1,T=D=4), `τ_2`(C=2,T=D=6), `τ_3`(C=2,T=D=8). `τ_1`&`τ_3` share `R`; `τ_3` CS length `δ_{3,R}=2`. RM order `τ_1>τ_2>τ_3`. `C(S_R)=P_1`.
- `B_1 = 2` (direct blocking by `τ_3` on `R`).
- `B_2 = 2` (push-through: `R` used by `τ_1>τ_2` and `τ_3<τ_2` → Lemma 7.1).
- `B_3 = 0` (lowest priority).
- `U = 0.25+0.333+0.25 = 0.833`.
- LL test (Eq 7.19): `τ_1` passes (0.75≤1), `τ_2` FAILS (0.917 > 0.828), `τ_3` FAILS (0.833 > 0.780) → **LL bound (sufficient only) cannot certify it.**
- Exact RTA (Eq 7.22): `R_1=3≤4`, `R_2=6≤6`, `R_3=6≤8` → **SCHEDULABLE.**
- → Demonstrates: blocking inflates `C_i`; the utilization bound is pessimistic; exact analysis is needed. (Numbers checked by script.)

---

## 9. Open questions / things still unclear (flagged)

1. **`δ−1` vs full `δ`:** Buttazzo's tabular example subtracts 1 (discrete-time, CS must start *before* arrival). For a continuous-time hand trace (Deliverable 4) is it cleaner to drop the `−1` and use full CS length? — *Working decision:* use full length `B=δ` for the continuous trace, note the discrete `δ−1` variant. (need to keep consistent across paper + UPPAAL + C++.)
2. **Spec said "WCBT formula from §7.9".** Actually the `B_i` *formula* is §7.6.3 (Eqs 7.9–7.11); §7.9 has the *schedulability tests* that consume `B_i`. — flag this in M4 so I cite the right section.
3. **`α_i = min(l_i, s_i)` — which dominates here?** In Deliverable 4: `l_1`=1 (only τ_3 below shares R), `s_1`=1 (only S_R). So `α_1=1`. Confirm `B` via both Eq 7.9 and 7.10 give 2.
4. **Transitivity needs nested CS (Lemma 7.2).** The base 3-task example has *no* nesting → no transitivity there. So to exercise transitive inheritance in UPPAAL I need a *second*, nested scenario (Fig. 7.10 style). — decide whether UPPAAL model covers the simple case only or also nesting.
5. **Does the deadlock counterexample (Fig. 7.13) belong in M1 or only M3?** It's a PIP limitation → M3. Keep here only as a pointer.
6. **POSIX mapping** (`PTHREAD_PRIO_INHERIT`) and OSEK/AUTOSAR — Buttazzo doesn't cover these; need them for M2 application-domains. Confirm against POSIX docs, not memory.

---

## 10. One-line takeaways
- PIP **bounds** priority inversion to `α_i = min(l_i,s_i)` critical sections (Thm 7.2) by **transitive priority inheritance** (Eq 7.8).
- It is **transparent** (no code change) and low-pessimism, but does **not** prevent **deadlock** (Fig 7.13) or **chained blocking** (Fig 7.12) — that's what PCP/SRP add.
- Blocking bound is a loose `O(mn²)` upper bound (Fig 7.11), feeding the inflated-`C_i` schedulability tests of §7.9.
