# Reproducing the experiments

## 1. Numerical reproduction

Clone this repository and run Python 3.10+. The default command uses only the Python standard library:

```powershell
git clone https://github.com/BXSYGJ/PaceRV-MCSoC2026.git
Set-Location PaceRV-MCSoC2026
python scripts/reproduce.py
```

The script checks `FILE_MANIFEST.json`, recomputes age populations, the 99% sizing decision, finite capture, admission-cycle reduction, model accuracy, leave-one-out capture and CoreMark metrics. The 19 workloads are averaged geometrically for the admission comparison. CoreMark uses 6,000 iterations and the performance-role cycle count, rather than the instrumented validation-role count.

```powershell
python -m pip install -r requirements.txt
python scripts/reproduce.py --per-instruction --plots
```

This additionally streams the 2,177,072 changed-service instruction pairs, joins the 16,396 new-workload predictions to measured instruction events, and checks the second-service ablation error records. The second-service error CSV stores nonzero errors only; exact relative-model results are recorded in the model summaries. Intermediate outputs are under `output/` and can be regenerated. The released inputs remain unchanged.

## 2. Rebuild and execute the predictor

On Linux, install a C++20-capable `g++` and zlib development headers. On Windows, use WSL Ubuntu 22.04 with the same packages. All examples below use PowerShell; no Bash script is required.

```powershell
wsl.exe --distribution Ubuntu-22.04 --exec sudo apt-get install g++ zlib1g-dev
python scripts/replay.py
```

Set `PACERV_WSL_DISTRO` if another distribution is used. The script invokes `wsl.exe --exec` directly and translates paths. `--limit 1` provides a short predictor smoke test. The complete run executes four model variants for both services, both request modes and four programs: 64 computations. Each CSV must match its original frozen SHA-256. This comparison executes the real predictor; it does not substitute stored predictions.

The full eight source graphs and command/cancellation/preparation/pipeline inventories are under `data/replay/`. Graph input hashes are tied to the original forecasts. The graph replay retains original cache outcomes and event order. The bit-6/7 schedule changes the first read beat by `2 + 2*address[6]` edges and subsequent spacing by `1 + address[7]`. The bit-8/9 schedule uses `2 + 2*address[8]` and `1 + (address[9] XOR address[8])`. Writes retain their original service.

## 3. Timed target RTL

The compiled RTL snapshots, native timing driver and read-only observers are in `src/rtl/`. They use the same complete CPU/peripheral system as the archived experiment. Request mode is a top-level parameter. The target matrix contains 50 executions; the eight original-service executions used to collect source profiles are archived separately, giving 58 in total.

Requirements: Verilator **5.032** (historical version), a C++20 compiler, make and the two PDS 2022.2-SP6.4 native simulation models. The driver advances simulation time and keeps native memory delays enabled. The models are `GTP_DRM36K_E1.v` and `GTP_GRS.v`; their exact SHA-256 values are checked by `scripts/run_rtl.py`. Install them from your licensed PDS installation rather than copying replacement behavioral RAM into the experiment.

On Windows, `--vendor` is a Windows-accessible directory containing both files; `--verilator` is the executable's Linux path inside WSL. On Linux, both are native paths.

```powershell
python scripts/run_rtl.py --vendor 'PATH_TO_PDS_MODELS' --verilator 'PATH_TO_VERILATOR' --case bit67/E0/K2/riscv-matmul16
python scripts/run_rtl.py --vendor 'PATH_TO_PDS_MODELS' --verilator 'PATH_TO_VERILATOR' --all
```

The runner rebuilds each needed model and compares measured cycles, retired instructions, instruction-stream and untainted-writeback hashes, and UART bytes with the archived run. UART timing is simulated using the recorded testbench clock parameters; the hardware operating frequency is a separate measurement. The four kernels preserve their inputs and output checks. `benchmarks/RISCV_TESTS_LICENSE` applies to upstream benchmark code. Prebuilt firmware removes a compiler-version dependency from these RTL reruns. For new firmware, the historical compiler was xPack RISC-V GCC 10.2.0-1.2 with `-march=rv32imc -mabi=ilp32 -O3 -funroll-loops`; inspect the supplied startup, linker and platform files before changing workloads.

## 4. FPGA implementation and board measurement

Clone the pinned [PaceRV release](https://github.com/BXSYGJ/PaceRV/tree/v1.0.0) separately:

```powershell
pwsh -NoProfile -File scripts/Get-PaceRV.ps1
```

It is placed under `external/PaceRV`, at the recorded commit. Follow its `docs/getting-started.md` and `docs/reproducibility.md` for PDS preparation, synthesis/place-and-route, JTAG programming and complete serial captures. The two paper points are `cpu/H3_Base_145125` and `cpu/H3_Retained_145125`. Their historical names identify the same PaceRV implementations. Use PG2L200H, 145.125 MHz and **0.150 ns clock uncertainty**. Keep LUT, FF, BRAM and DSP/APM separate. Timing slack already includes uncertainty.

The hardware repository includes full peripherals, memory capacities, firmware, timing constraints, routed reports and board references. Neither repository redistributes a PDS installer or proprietary simulation library. Place vendor inputs outside tracked files.

## 5. Verification levels

CI verifies file integrity, recalculates numerical claims, streams per-instruction records, generates analysis plots and rebuilds/runs the predictor. Timed native-memory simulation and physical board tests additionally require PDS files/hardware and are recorded separately. A data reduction, a fresh RTL execution and a new routed or board measurement are distinct evidence levels.

The broad original-suite finite-execution results are provided as per-run records and instruction-comparison reductions. The 38 original large dependency graphs are not included; their per-instruction forecast errors and frozen prediction records are included. Complete graph-level replay is provided for the four new kernels. New experiments should use new output paths and preserve the released baseline.
