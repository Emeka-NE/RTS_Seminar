# M2 — Scientific Contextualization and Tradeoff Analysis
**Topic:** Priority Inheritance Protocol (Resource Access Protocols)
**Author:** Nnaemeka Nnachi-Egwu | nnaemeka.nnachi-egwu@stud.hshl.de
**Primary source:** Buttazzo, Ch. 7 (pp. 205–249). Secondary: Kopetz [Ko]; WCET paper [WCET]; Global/Partitioned scheduling paper [GP].

> Working notes format. Structured bullets, comparison tables, annotations. Not final prose.

---

## 1. Where PIP sits in the resource-access protocol landscape

### Chronological order (Buttazzo §7.1 overview, p. 209)
```
NPP (Non-Preemptive Protocol)
 └─ simple; over-blocks; transparent
HLP / IPC (Highest Locker Priority = Immediate Priority Ceiling)
 └─ better than NPP; still arrival-time blocking; needs ceiling declaration
PIP (Priority Inheritance Protocol)   ← our topic
 └─ Sha, Rajkumar, Lehoczky [SRL90]
 └─ first protocol to defer blocking to access time; bounded inversion
PCP (Priority Ceiling Protocol)
 └─ same authors [SRL90]; extends PIP to add deadlock freedom
SRP (Stack Resource Policy)
 └─ Baker [Bak91]; extends PCP to multi-unit resources + EDF
```

### Conceptual position
- **NPP/HLP** solve priority inversion by *raising priority at arrival*. Consequence: a task is blocked even if it never actually accesses the contended resource during that instance (unnecessary blocking, Fig. 7.6 p. 211 and the unpredictable branch argument §7.5 p. 214).
- **PIP** defers blocking to the *access instant* — no blocking until `wait(S_k)` actually hits a locked semaphore. This removes arrival-time pessimism (§7.5→§7.6 transition, p. 214).
- **PCP/SRP** then add a lock-grant restriction to enforce at most one blocking, deadlock freedom, and (SRP) stack sharing.

---

## 2. What PIP solves over NPP and HLP

### PIP advantage over NPP
- NPP: every task entering *any* CS raises to max system priority (Eq 7.3) — all higher-priority tasks are blocked regardless of resource overlap. Buttazzo calls this "unnecessary blocking" (Fig. 7.6, p. 211).
- PIP: priority is only raised when a *specific* higher-priority task is actually blocked. A task τ_1 that shares *no* resource with τ_3 is never push-through blocked (by design, since Lemma 7.1 requires the semaphore to be accessed by a task with P > P_i to create push-through blocking).

### PIP advantage over HLP
- HLP blocks tasks at *activation/preemption time* based on ceiling of the resource being entered, not based on actual resource conflict (p. 213–214). Scenario: if a CS is guarded only by a conditional branch (either branch may or may not access the resource), HLP blocks the higher-priority task even when the branch without the resource is taken at runtime.
- PIP blocks only at `wait(S_k)` — "postponing the blocking condition at the entrance of a critical section" (p. 214). This removes conditional-branch pessimism.
- Both NPP and HLP block with a **single blocking** maximum (Theorem 7.1). PIP allows up to **α_i = min(l_i, s_i)** blockings — so PIP can cause *more* blocking in the worst case, but each blocking is tighter (it only occurs when resource contention is genuine).

### PIP limitations compared to PCP
| property | PIP | PCP |
|---|---|---|
| prevents deadlock | **NO** (§7.6.5, Fig. 7.13) | YES (Theorem 7.3) |
| prevents chained blocking | **NO** (§7.6.5, Fig. 7.12) | YES (Theorem 7.4: at most 1 CS) |
| max blocking per task | α_i critical sections | 1 critical section |
| arrival-time blocking | no | no |
| transparency (no code change) | **YES** | no (needs ceiling API) |
| implementation complexity | hard (Table 7.5) | medium (Table 7.5) |

### PIP limitations compared to SRP
- SRP extends PCP to: multi-unit resources, EDF (dynamic priorities), stack sharing. PIP is strictly fixed-priority and single-unit.
- SRP blocks at preemption time (like NPP/HLP) but with tighter ceilings based on multi-unit availability — less pessimistic than NPP.

---

## 3. Kopetz perspective: why priority inversion is a correctness failure

**Kopetz definition (taught in slide 7 of RTS_Introduction.pdf):**
> "A real-time computer system is a computer system in which the correctness of the system behaviour depends not only on the logical results of the computation, but also on the *physical instant* at which these results are produced."

**Implication for priority inversion (not just a performance problem):**
- In the Fig. 7.4 scenario, τ_1 has the highest priority precisely because it has the tightest deadline. Missing τ_1's deadline because an unbounded number of medium-priority tasks ran freely (τ_2, τ_2', τ_2''...) during the inversion window is a *correctness failure* by the Kopetz definition: the physical instant of τ_1's result production is wrong, not merely late.
- This is not recoverable: a missed hard deadline in a hard real-time system (e.g. brake actuator, flight control) is a system failure, not just a degradation.
- "Professor said: real-time means predictable, not fast — result after deadline is wrong, not just late." [Slide 13]
- EDF is optimal *only when there is no resource competition* (slide 48). Priority inversion is exactly the case where EDF's optimality proof breaks — the presence of shared resources destroys the theoretical guarantee. PIP (or a stronger protocol) is necessary to restore schedulability analysis validity.

---

## 4. Application domains where PIP is actually used

### POSIX — `pthread_mutexattr_setprotocol`
- POSIX.1-2008 (IEEE Std 1003.1) standardises `PTHREAD_PRIO_INHERIT` as the PIP mode for mutexes.
- Usage:
  ```c
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
  pthread_mutex_init(&mutex, &attr);
  ```
- Supported by: Linux (with POSIX real-time extensions, `-lrt`), VxWorks, QNX, µC/OS.
- Why PIP and not PCP? Transparency — PIP needs no off-line resource declarations. Legacy code using plain `pthread_mutex` can be migrated to `PTHREAD_PRIO_INHERIT` without changing the locking structure.

### Automotive — OSEK/AUTOSAR
- OSEK OS (1995) and its successor AUTOSAR OS (for classical platform) both include a resource access mechanism called the "OSEK ceiling priority protocol" (equivalent to HLP/IPC in Buttazzo terminology). However, vendor extensions and some AUTOSAR profiles support PIP-like inheritance on shared resource objects.
- In safety-critical ECUs (e.g. airbag, ABS), bounded blocking time is a certification requirement (ISO 26262, ASIL levels). PIP's bounded α_i blocking (Thm 7.2) together with Response Time Analysis (Eq 7.22) provides the required timing guarantee.
- The Mars Pathfinder incident (1997) is the textbook case: priority inversion caused an IPC bus reset and full system reboot. The vehicle used VxWorks; the fix was enabling `PTHREAD_PRIO_INHERIT` on the affected mutex at runtime.

---

## 5. Where PIP is insufficient

### Multiprocessor systems
- Buttazzo states Chapter 7 explicitly addresses **uniprocessor** systems (§7.3, p. 209: "we consider a set of n periodic tasks... a set of m shared resources").
- On a multiprocessor, two tasks can be simultaneously running on two processors and both try to acquire the same semaphore. The "only one holder at a time" property still holds (binary semaphore), but:
  - The task blocked on a remote processor cannot easily inherit priority to the holder on a different processor — requires inter-processor communication to boost the holder's scheduling decision on its local processor.
  - Priority inheritance on multiprocessors has been studied (MPCP, FMLP, etc.) but is not part of Buttazzo Ch. 7 and remains an active research area.
- **Reference:** Gracioli et al. [GP] compare Global-EDF and Partitioned-EDF in a real RTOS — this paper grounds the context: shared-resource protocols must be extended or redesigned when scheduling migrates across processors. PIP as defined in [SRL90] / Buttazzo §7.6 does not transfer unchanged.

### Distributed systems
- In a distributed system, resources may reside on different nodes with no shared memory and no common clock. Semaphore operations become message-passing operations with non-deterministic latency. The concept of "inheriting the priority of a blocked task" requires the blocked task's priority to be communicated to the holder's node, which introduces additional timing uncertainty.
- Kopetz [Ko] discusses distributed system time constraints — the physical instant of result production is even harder to guarantee when the result must traverse a network.

### Systems requiring deadlock freedom
- As shown in §7.6.5 (Fig. 7.13, p. 226–227), PIP does not prevent deadlock when two tasks acquire two semaphores in reverse order. For safety-critical systems where deadlock is catastrophic and cannot be detected-and-recovered in bounded time, PCP or SRP must be used instead.

---

## 6. Tradeoff analysis

| Tradeoff dimension | NPP | HLP | PIP | PCP | SRP |
|---|---|---|---|---|---|
| Implementation effort | Low | Low | **High** (kernel mod) | Medium | Low |
| Source transparency | ✓ | ✗ | **✓** | ✗ | ✗ |
| Blocking pessimism | High | Medium | **Low** | Medium | Medium |
| Max # of blockings | 1 | 1 | **α_i = min(l_i,s_i)** | 1 | 1 |
| Deadlock freedom | ✓ | ✓ | **✗** | ✓ | ✓ |
| Chained blocking | No | No | **Possible** | No | No |
| Blocking time bound tightness | Loose (max CS of any lower task) | Medium | **Loose upper bound, Eq 7.11** | Tighter (1 CS) | Same as PCP |
| Dynamic priorities | ✗ | ✗ | **✗** | ✗ | **✓** (SRP) |

**Key design choice insight:**
- Choose PIP when: legacy codebase (transparency), resource-usage pattern has little nesting (limiting α_i), deadlock is provably impossible by design (total semaphore ordering), and implementation budget allows kernel modification.
- Choose HLP/PCP when: single blocking is needed for certification, code change is acceptable, resources can be declared statically.
- Choose SRP when: EDF scheduling, multi-unit resources, or stack sharing is needed.

---

## 7. Justification for each reference beyond Buttazzo

| Reference | What it contributes that Buttazzo does not |
|---|---|
| Kopetz [Ko] | The *formal definition* of real-time system (physical instant) taught in slide 7. Grounds the correctness argument: missing a deadline is not a performance problem but a logical error. Buttazzo uses it but does not formally derive it. Also: distributed system perspective on why PIP does not extend to distributed contexts. |
| WCET paper — Abella et al. 2015 [WCET] | Buttazzo's blocking-time formula (Eq 7.11) assumes δ_{j,k} values are exactly known. The WCET paper (Table I taxonomy: SDTA, MBDTA, SPTA, MBPTA, HYPTA) shows that deriving trustworthy WCET bounds is itself a difficult, open problem — especially for modern hardware with caches, pipelines, and multicores. This puts the "plug in B_i and check schedulability" workflow in its realistic context: the B_i inputs are themselves estimates. |
| Gracioli et al. 2013 [GP] | Buttazzo does not address multiprocessor scheduling. This paper provides the context for PIP's scope limitation: it implements and evaluates Global-EDF and Partitioned-EDF in a real RTOS (EPOS/LITMUSRT on dual/quad-core hardware) and shows that real-time guarantees on multiprocessors require substantially different OS infrastructure. PIP as a uniprocessor protocol does not provide correct semantics when tasks can migrate across processors. |

---

## 8. Open questions for M3 and M4

1. **WCET assumption in blocking computation:** Eq 7.11 assumes δ_{j,k} is exactly known. When is this a safe assumption? [→ M3 critical evaluation; cite WCET paper]
2. **PIP in POSIX: is the inheritance always applied transitively?** POSIX.1-2008 specifies `PTHREAD_PRIO_INHERIT` but the transitivity requirement varies by implementation. Linux applies transitivity up to a bounded chain depth. [→ flag in M4]
3. **Mars Pathfinder: was it pure PIP?** Some sources say the fix was to "enable priority inheritance on the meteorological bus mutex." Need to verify this is exactly PIP (and not PCP or ceiling-based). [→ cite primary VxWorks docs; use as application example caveat in M3]
4. **SRP stack sharing** is an elegant advantage not available in PIP. Worth mentioning in M3 as a concrete extension path.
