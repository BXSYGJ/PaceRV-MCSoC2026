# PaceRV: MCSoC 2026 reproducibility artifact

Code, experimental records, dependency profiles and figures for **Memory Admission and Result-Retention Sizing in an RV32IMC FPGA Soft Processor**, accepted at MCSoC 2026.

**Yihang Dai, Shuyu Xu, Yinfeng Zhou and Chenglei Peng — Nanjing University.**

This paper studies how accepting memory requests earlier changes the population of delayed multiplications and the validity window needed to retain their prepared results. The artifact connects the instruction-level measurements, finite-window predictions, new-workload validation and physical implementation results.

The complete processor and board release is [PaceRV v1.0.0](https://github.com/BXSYGJ/PaceRV/tree/v1.0.0), pinned to commit `b3ff0f886bb878d8ebac44366f3182eb6cbb5241`. PaceRV is a single-issue, five-stage RV32IMC FPGA **SoC with full peripherals**: caches and shared backing BRAM, UART TX/RX with FIFOs, GPIO, a 64-bit timer, interrupts, machine CSRs, 64-bit execution counters and live cache monitoring. The paper's operating point is **145.125 MHz and 3.442 CoreMark/MHz**.

[中文说明](README.zh-CN.md) · [Reproduction guide](docs/REPRODUCING.md) · [Claim-to-evidence map](docs/EVIDENCE.md) · [License and third-party materials](THIRD_PARTY_NOTICES.md)

## Quick start

Python 3.10+ is sufficient to verify the archive and recompute the main numerical results. Run these commands from the repository root in PowerShell 7 or another terminal:

```powershell
python scripts/reproduce.py
```

The results appear in `output/results.json` and `output/implementation.csv`. For instruction-level checks and regenerated analysis plots:

```powershell
python -m pip install -r requirements.txt
python scripts/reproduce.py --per-instruction --plots
```

To rebuild the frozen C++ predictor and repeat all 64 model/service/profile combinations, use Linux with C++20 and zlib, or Windows with WSL Ubuntu 22.04:

```powershell
python scripts/replay.py
```

Each output is compared **byte for byte** with the frozen forecast. Eight full original-service dependency profiles are included, covering four new kernels in both request modes. This repeats a source-profiled forecast; each kernel supplies its own source execution.

## What is included

| Material | Location | Reproduction |
|---|---|---|
| Admission comparison and original retention accounting | `data/admission/` | Recompute the 18.2% geometric-mean cycle reduction and residual demand |
| 152 finite executions and instruction comparisons | `data/finite/` | Recompute recovery, selected windows and cycle residuals |
| Changed-service predictions and 2,177,072 paired instruction errors | `data/forecast/` | Recalculate endpoint and lifetime accuracy |
| Independent bit-8/9 service experiment | `data/second-service/` | Check model ablation, capture and elapsed-cycle accuracy |
| Four new kernels, all 58 execution records, and complete source graphs | `data/unseen/`, `data/replay/` | Recompute results and rerun the native predictor |
| Actual compiled target RTL and timed observers | `src/rtl/` | Rerun the 50 target executions using user-installed PDS memory models |
| Kernel sources and platform hooks | `benchmarks/` | Inspect the preserved algorithms, datasets and output checks |
| Leave-one-out and target sensitivity | `data/leave-one-out/`, `data/sensitivity/` | Inspect transfer and sizing decisions |
| PaceRV/VexRiscv implementation records | `data/implementation/` | Recompute CoreMark/MHz and CoreMark/MHz/kLUT |
| Matched VexRiscv generator, RTL and peripheral adapters | `comparison/vex/` | Inspect the exact compared configuration and its upstream license |
| Five paper figures, editable SVG and PDF | `paper-figures/` | Inspect the exact camera-ready artwork |

The generated analysis plots use the published data and portable fonts. The camera-ready artwork is preserved separately in `paper-figures/`.

## Hardware and full RTL experiments

[PaceRV](https://github.com/BXSYGJ/PaceRV) provides the complete board projects, firmware, timing reports, board captures and hardware instructions. This repository adds paper-specific prediction and finite-window experiments. See [the reproduction guide](docs/REPRODUCING.md) for prerequisites and commands for new-computer use, timed native-memory simulation and PDS implementation.

The common-clock comparison reports PaceRV board cycles and VexRiscv native RTL cycles separately. CoreMark/MHz/kLUT is computed as `(CoreMark/MHz) / (LUT / 1000)`. Conditional cycle improvements are **non-additive**. A validity window controls fast-path use; its expiry does not release the stored product.

## Paper and citation

Publication metadata and the camera-ready identity are recorded in [paper.json](paper.json). DOI and arXiv links will be added when available. The paper PDF is distributed through its publication/preprint channel. Use [CITATION.cff](CITATION.cff) to cite this artifact.

## Provenance and license

`FILE_MANIFEST.json` identifies every released file. `provenance.json` records original and public-file hashes. Historical JSON paths are replaced with neutral references; numerical results and original source/input hashes are preserved. These relocated records remain historical observations, and reruns write only to `output/`.

Author-created code is under the [MIT license](LICENSE). Experimental data are available under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Paper artwork retains its authors' copyright and is provided for scholarly reproduction with attribution. Benchmark and other third-party notices are preserved in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). PDS, its simulation models and other proprietary software are installed separately.
