# Paper claims and evidence

The artifact targets the accepted camera-ready scientific results. `paper.json` binds its PDF and TeX hashes; the repository adds a code/data availability sentence without changing experimental values.

| Paper result | Inputs | Computation / scope |
|---|---|---|
| 18.2% lower geometric-mean Embench cycles | `data/admission/workloads.csv` | Pair registered and early `T0_cycles` by workload; geometric mean of 19 ratios |
| 375,063 → 4,373 additional opportunities | `data/finite/results.json`, `data/admission/workloads.csv` | Count delayed prepared instructions; snapshot age pairs total 1,049,602 |
| 18- / 28-edge windows for 99% capture | `data/finite/results.json` | Smallest integer K≥2 serving 99% of each delayed population |
| Exact recovery in 152 finite executions | `data/finite/results.json` | Per-workload predicted versus measured additional fast consumption |
| 8,708,288 finite–unbounded instruction comparisons | `data/finite/results.json` | Sum matched retired prepared instructions over 152 comparisons |
| 0.595-edge endpoint MAE and 2,177,072 exact lifetimes | `data/forecast/endpoint-metrics.csv`, `paired-owner-errors.csv.gz` | Full executions, 1,088,536 instructions per admission mode; lifetime and endpoint accuracy are separate |
| Second-service K=12, 320,000 exact endpoint pairs | `data/second-service/` | Bit-8/9 service, registered `matmult-int`; compare five timing models |
| 58 new-workload executions and 16,396 paired instructions | `data/unseen/`, `data/replay/` | Eight source profiles, 16 unbounded targets, 34 finite targets; source-profiled transfer |
| Leave-one-out 5/7 registered, 2/2 early | `data/leave-one-out/results.json` | Positive-demand held-out programs; pooled capture 98.9% / 99.98% |
| Target sensitivity | `data/sensitivity/thresholds.csv` | Fractional targets and absolute budgets from the same age distribution |
| 3.442 CoreMark/MHz at 145.125 MHz | `data/implementation/pacerv.json` | V0 performance cycles, 6,000 iterations; board evidence in pinned PaceRV release |
| Matched VexRiscv control | `data/implementation/vex.json` | Native RTL cycles and routed resources, same 145.125-MHz clock; upstream commit `680756065e9e6fc50d8c3d6c58191a16e867d822` |

`scripts/reproduce.py` writes machine-readable recalculations. `scripts/replay.py` regenerates complete new-kernel forecast CSVs from the original graphs. The three generated analysis figures replot population/capture, finite prediction and implementation data. Five exact paper figures are preserved under `paper-figures/`.

Historical records use `E0` for registered requests and `E1` for early requests. An `owner` is a dynamic prepared multiplication, matched by retirement identity, instruction and operands. Preparation-to-consumption age is counted in clock edges; K bounds fast-path validity. Full-execution populations and snapshot populations have different denominators. Cancellation and instruction survival checks are retained in the records.

Cycle estimates overstate the registered finite result by 80 cycles, the second-service result by two and the new matrix kernel by four. These are retained alongside exact opportunity predictions. Early admission leaves zero additional retention demand in 17 of the original 19 programs. A zero-demand control has undefined fractional capture, rather than 100% capture.

PaceRV board measurements and VexRiscv RTL measurements are labeled separately. Routed LUT differences include synthesis/placement changes. The artifact does not infer isolated logic cost by subtracting unmatched layouts, and no conditional percentages are added together.
