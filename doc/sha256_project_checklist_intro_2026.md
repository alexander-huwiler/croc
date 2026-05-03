# SHA-256 Project Checklist for VLSI2 Intro 2026

This checklist maps our current Croc SHA-256 work to the grading points from the
`01_Intro.pdf` deck, using the 2026 intro slides as the source of truth.

## Source Requirements

From the intro deck:

- final project in groups of two
- start from the Croc SoC used in the exercises
- adapt and improve the design
- submit by `30 June 2026, 10:00 Zurich time`
- timely submission worth `3.00` points
- bonuses:
  - `+0.50` one-page outline with block diagram
  - `+0.25` standard simulation pass
  - `+0.50` DRC and LVS
  - `+1.00` technical content of the report
  - `+0.50` quality of the report
  - `+0.25` design chosen for manufacturing

## Status Tracker

| Item | Points | Status | Evidence / Notes |
|---|---:|---|---|
| Timely online submission | 3.00 | In progress | Need final packaged submission by `30 June 2026, 10:00 Zurich time`. |
| One-page outline with block diagram | +0.50 | Drafted | See `doc/sha256_project_outline_intro_2026.md`. |
| Standard simulation pass | +0.25 | Partially satisfied | SHA tests and `test_soc_ctrl` pass in Verilator; still need to be ready for the course's chosen simulation. |
| DRC and LVS | +0.50 | Not started | Requires full backend flow and clean signoff. |
| Technical content of the report | +1.00 | Strong start | Accelerator, integration, multiblock support, and benchmark are already implemented. Final report still needs synthesis/backend evidence and polished narrative. |
| Quality of the report | +0.50 | Not started | Figures, tables, captions, and final writeup quality still need focused work. |
| Design chosen for manufacturing | +0.25 | Stretch goal | Will depend on technical merit, clean implementation, and convincing documentation. |

## What We Have Already Done

- Implemented a SHA-256 accelerator in `rtl/sha256/sha256_accel.sv`.
- Integrated it into Croc `user_domain` at `0x2000_1000`.
- Added MMIO software support in `sw/lib/inc/sha256_accel.h` and
  `sw/lib/src/sha256_accel.c`.
- Verified the one-block `"abc"` test vector in `sw/test/test_sha256_accel.c`.
- Verified chained multi-block hashing in `sw/test/test_sha256_accel_multiblock.c`.
- Added a benchmark in `sw/test/test_sha256_accel_bench.c`.
- Measured about `14.4x` speedup on the current one-block benchmark.
- Fixed the `crt0.S` startup regression that was breaking bare-metal execution in
  Verilator.
- Established a working WSL `Ubuntu-22.04` verification flow without relying on Docker.

## What Still Matters Most

1. Synthesis numbers:
   get area and timing results for the SHA block and its integration cost.
2. Backend viability:
   push the design through the OpenROAD flow and identify floorplan or timing issues.
3. Report material:
   capture architecture, register map, benchmark methodology, synthesis results,
   and design tradeoffs in a clean narrative.
4. Physical signoff:
   work toward DRC/LVS-clean output if the project scope and schedule allow it.

## Immediate Next Actions

1. Run synthesis on the SHA-256 accelerator integration and record area/timing.
2. Decide whether the current software-padding model is sufficient for the report,
   or whether to extend the hardware interface.
3. Start collecting report figures early: block diagram, register map, benchmark
   table, and later synthesis/backend screenshots.
4. Sync with the teammate and split ownership across RTL, verification, and report work.
