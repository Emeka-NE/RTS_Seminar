// pip_simulator.cpp
// Priority Inheritance Protocol demonstration
// Buttazzo, Hard Real-Time Computing Systems, 3rd ed., §7.6
// Compile: g++ -std=c++17 -Wall -Wextra -o pip_simulator pip_simulator.cpp

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

// ----------------------------------------------------------------
// Types
// ----------------------------------------------------------------
static const int NONE = -1;

enum class S { IDLE, READY, RUN, BLOCK };

struct Task {
    int id; const char* nm;
    int C, T, D, nom;         // config
    int cs_enter_after;       // exec units before CS entry (-1 = no CS)
    int cs_len;
    // runtime (reset each instance)
    S   state = S::IDLE;
    int prio, rem;            // active priority, remaining compute
    int cs_rem;               // remaining CS units
    bool has_lock;            // currently holds sem 0
    bool entered_cs;          // already attempted entry this instance
    int  next_rel, abs_dl;
    int  blk_start, max_blk;
    bool missed;
    int  exec_done;           // units executed this instance (for CS timing)
};

struct Sem { const char* nm; int holder; std::vector<int> q; };

static bool PIP;
static std::vector<Task> TT;
static std::vector<Sem>  SS;

// ----------------------------------------------------------------
// PIP kernel (Buttazzo §7.6.4, pp. 224-225)
// ----------------------------------------------------------------
static void inherit(int hid, int req_prio);

static void restore(int tid) {
    if (!PIP) return;
    auto& t = TT[tid]; int best = t.nom;
    for (auto& s : SS)
        if (s.holder == tid)
            for (int w : s.q) best = std::min(best, TT[w].prio);
    if (best != t.prio)
        printf("     [PIP] %s prio %d->%d\n", t.nm, t.prio, best);
    t.prio = best;
}

static void inherit(int hid, int req_prio) {
    if (!PIP) return;
    auto& h = TT[hid];
    if (req_prio >= h.prio) return;
    printf("     [PIP] %s inherits prio %d (was %d)\n", h.nm, req_prio, h.prio);
    h.prio = req_prio;
    // Transitivity: if h is itself blocked, propagate
    for (auto& s : SS) {
        auto it = std::find(s.q.begin(), s.q.end(), hid);
        if (it != s.q.end() && s.holder != NONE)
            inherit(s.holder, req_prio);
    }
}

// Returns true=acquired, false=blocked
static bool pi_wait(int tid, int sid, int t) {
    auto& s = SS[sid]; auto& task = TT[tid];
    if (s.holder == NONE) {
        s.holder = tid; task.has_lock = true;
        printf("     %s acquires %s\n", task.nm, s.nm);
        return true;
    }
    task.state = S::BLOCK; task.blk_start = t;
    s.q.push_back(tid);
    printf("     %s BLOCKED on %s (holder=%s)\n", task.nm, s.nm, TT[s.holder].nm);
    inherit(s.holder, task.prio);
    return false;
}

static void pi_signal(int tid, int sid, int t) {
    auto& s = SS[sid]; TT[tid].has_lock = false;
    if (s.q.empty()) {
        s.holder = NONE;
        printf("     %s releases %s (no waiters)\n", TT[tid].nm, s.nm);
    } else {
        auto it = std::min_element(s.q.begin(), s.q.end(),
            [](int a, int b){ return TT[a].prio < TT[b].prio; });
        int w = *it; s.q.erase(it); s.holder = w;
        TT[w].state = S::READY; TT[w].has_lock = true;
        // waiter now holds lock, restore its cs_rem
        TT[w].cs_rem = TT[w].cs_len - (TT[w].cs_len - TT[w].cs_rem);
        // actually the waiter blocked *before* executing any CS unit, so full cs_len
        int dur = t - TT[w].blk_start;
        TT[w].max_blk = std::max(TT[w].max_blk, dur);
        printf("     %s releases %s -> %s unblocked (%d units blocked)\n",
               TT[tid].nm, s.nm, TT[w].nm, dur);
    }
    restore(tid);
}

// ----------------------------------------------------------------
// Scheduler
// ----------------------------------------------------------------
static int best_runner() {
    int b = NONE, bp = 9999;
    for (int i = 0; i < (int)TT.size(); i++)
        if (TT[i].state != S::IDLE && TT[i].state != S::BLOCK
            && TT[i].rem > 0 && TT[i].prio < bp)
            { bp = TT[i].prio; b = i; }
    return b;
}

// ----------------------------------------------------------------
// Simulate
// ----------------------------------------------------------------
static void simulate(bool pip, const char* label, int sim_time) {
    PIP = pip;
    printf("\n%s\nSCENARIO: %s\n%s\n\n",
           std::string(60,'=').c_str(), label,
           std::string(60,'=').c_str());

    // Init tasks
    // tau1: C=1,T=4,D=4,nom=1, CS after 0 exec units, len=1
    // tau2: C=2,T=6,D=6,nom=2, no CS
    // tau3: C=4,T=16,D=16,nom=3, CS after 0 exec units, len=4
    TT = {
        {0,"τ1",1,4,4,1, 0,1},
        {1,"τ2",2,6,6,2,-1,0},
        {2,"τ3",4,16,16,3,0,4},
    };
    for (auto& t : TT) {
        t.next_rel = 0; t.abs_dl = t.D; t.prio = t.nom;
        t.rem = 0; t.cs_rem = 0; t.has_lock = false;
        t.entered_cs = false; t.blk_start = -1; t.max_blk = 0;
        t.missed = false; t.exec_done = 0;
    }
    SS = { {"S_R", NONE, {}} };

    int runner = NONE;
    std::vector<std::string> gantt;

    for (int t = 0; t < sim_time; t++) {

        // Releases
        for (int i = 0; i < (int)TT.size(); i++) {
            auto& tk = TT[i];
            if (t == tk.next_rel && tk.state == S::IDLE) {
                tk.state = S::READY; tk.rem = tk.C; tk.prio = tk.nom;
                tk.has_lock = false; tk.entered_cs = false;
                tk.blk_start = -1; tk.cs_rem = tk.cs_len;
                tk.exec_done = 0;
                tk.abs_dl = t + tk.D;
                printf("  t=%2d %s released (dl=%d)\n", t, tk.nm, tk.abs_dl);
            }
        }

        // Select runner (preemption)
        int nxt = best_runner();
        if (nxt != runner && nxt != NONE && runner != NONE
            && TT[runner].state == S::RUN && TT[runner].rem > 0) {
            TT[runner].state = S::READY;
            printf("  t=%2d PREEMPT %s -> %s\n", t, TT[runner].nm, TT[nxt].nm);
        }
        runner = nxt;

        // CS entry attempt
        if (runner != NONE) {
            auto& rt = TT[runner];
            if (rt.cs_enter_after >= 0 && !rt.entered_cs && !rt.has_lock
                && rt.exec_done == rt.cs_enter_after
                && rt.state != S::BLOCK) {
                rt.entered_cs = true;
                bool got = pi_wait(runner, 0, t);
                if (!got) {
                    runner = best_runner();
                }
            }
        }

        // Execute 1 unit
        std::string slot = "IDLE";
        if (runner != NONE && TT[runner].state != S::BLOCK) {
            auto& rt = TT[runner];
            rt.state = S::RUN;
            slot = rt.nm;
            rt.rem--;
            rt.exec_done++;

            // CS progress
            if (rt.has_lock && rt.cs_rem > 0) {
                rt.cs_rem--;
                if (rt.cs_rem == 0) {
                    printf("  t=%2d", t+1);
                    pi_signal(runner, 0, t+1);
                }
            }

            // Completion
            if (rt.rem == 0) {
                bool miss = (t+1 > rt.abs_dl);
                if (miss) rt.missed = true;
                printf("  t=%2d %s done%s\n", t+1, rt.nm,
                       miss ? " *** DEADLINE MISS ***" : "");
                rt.state = S::IDLE;
                rt.next_rel += rt.T;
                runner = NONE;
            }
        }
        gantt.push_back(slot);

        // Deadline overrun check (while task still pending)
        for (auto& tk : TT)
            if (tk.state != S::IDLE && tk.rem > 0 && t >= tk.abs_dl && !tk.missed) {
                printf("  t=%2d *** DEADLINE MISS: %s ***\n", t, tk.nm);
                tk.missed = true;
            }
    }

    // Print Gantt
    printf("\nGantt (t=0..%d):\n", sim_time-1);
    printf("t:  "); for (int t=0;t<sim_time;t++) printf("%3d",t); printf("\n");
    printf("    "); for (auto& s:gantt) printf("%3s",s.c_str()); printf("\n");

    // Stats
    int Bth[] = {4,4,0};
    printf("\nBlocking: (continuous-time)\n");
    for (int i=0;i<(int)TT.size();i++)
        printf("  %s: observed=%d  B%d_bound=%d  %s\n",
               TT[i].nm, TT[i].max_blk, i+1, Bth[i],
               TT[i].max_blk<=Bth[i]?"OK":"EXCEEDS BOUND");
    printf("Deadline status: ");
    bool any=false;
    for (auto& t:TT) if(t.missed){printf("%s ",t.nm);any=true;}
    if(!any) printf("all met");
    printf("\n");
}

// ----------------------------------------------------------------
int main() {
    printf("PIP Simulator — Buttazzo §7.6 demonstration\n");
    printf("tau1(C=1,T=4,D=4,prio=1) tau2(C=2,T=6,D=6,prio=2) tau3(C=4,T=16,D=16,prio=3)\n");
    printf("tau1 and tau3 share S_R; tau3 CS length=4. tau2 does not use S_R.\n");
    printf("Theoretical: B1=4, B2=4(push-through Lemma7.1), B3=0\n\n");

    simulate(false, "WITHOUT PIP (plain semaphore)", 20);
    simulate(true,  "WITH PIP (priority inheritance, Buttazzo §7.6)", 20);
    return 0;
}
