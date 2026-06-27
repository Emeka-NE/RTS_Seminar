# AI Usage Protocol

Records of all significant AI interactions during milestone preparation.
Format: JSON, schema from `ai_usage_protocol_template_20260517.json`.

| File | Milestone | Entries | Key findings |
|------|-----------|---------|-------------|
| M1/ai_protocol_m1.json | Technical understanding | 8 | Error: AI said PIP impl = "medium"; Buttazzo Table 7.5 says "hard". Corrected. |
| M2/ai_protocol_m2.json | Contextualization | 6 | Error: AI said OSEK uses PIP by default; actual default is HLP/IPC. Qualified. |
| M3/ai_protocol_m3.json | Critical evaluation | 7 | Corrected α_i calculation for 2-semaphore chained blocking case. |
| M4/ai_protocol_m4.json | Paper drafting | 5 | Error: AI said PIP "eliminates" inversion; corrected to "bounds". |

## Entry Fields
Each entry contains: `id`, `timestamp`, `parent_id`, `objective`, `ai_tool{name,model}`, `prompt`, `ai_output_summary`, `verification{actions[],level}`, `evaluation{status,issues[],usefulness}`, `integration{used_in_work,section,usage_type,direct_text_reused}`, `reflection`.
