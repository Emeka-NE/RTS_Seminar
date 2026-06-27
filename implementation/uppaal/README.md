# UPPAAL Model — Priority Inheritance Protocol

**File:** `pip_model.xml` (open in UPPAAL GUI ≥ 4.0)
**Queries:** `pip_queries.q`
**Reference:** Behrmann, David, Larsen — A Tutorial on UPPAAL
**Protocol:** Buttazzo §7.6, Eq 7.8 (pi_wait/pi_signal logic)

## Model Components
1. **Task template** — instantiated for τ₁, τ₂, τ₃; states: Idle/Ready/Running/CriticalSection/Blocked
2. **ResourceManager** — models S_R; implements PIP inheritance in transition assignments
3. **Observer** — monitors `blocking_tau1` and `block_dur[]` for property evaluation

## Verification Queries (pip_queries.q)
| Property | Query | Expected |
|----------|-------|---------|
| No deadlock | `A[] not deadlock` | SATISFIED |
| No unbounded inversion | `A[] (blocking_tau1 imply not tau2.Running)` | SATISFIED |
| Blocking within bounds | `A[] (block_dur[0]<=4 and block_dur[1]<=4)` | SATISFIED |
| Inheritance reachable | `E<> (act_prio[2]==1)` | SATISFIED |
| Priority restored | `A[] (rm.Free imply act_prio[2]==3)` | SATISFIED |

## Usage
1. Open `pip_model.xml` in UPPAAL GUI
2. Go to Verifier tab
3. Paste queries from `pip_queries.q` or load the file
4. Click "Check"

**Note:** The model was designed and tested against the tutorial syntax from
Behrmann et al. It may require minor XML adjustments for different UPPAAL versions.
The key semantics (inheritance assignment on lock_req[0]? transition) are correct.
