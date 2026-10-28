# License and attribution

## Author-created code

The root MIT license covers author-created Python, PowerShell, C++ and RTL in this artifact, subject to retained third-party notices. The complete board release has its own license and notices in [PaceRV](https://github.com/BXSYGJ/PaceRV).

## Experimental data

Author-created measurement records, dependency profiles, predictions and derived tables under `data/` are licensed under **Creative Commons Attribution 4.0 International**: https://creativecommons.org/licenses/by/4.0/. Attribute Yihang Dai, Shuyu Xu, Yinfeng Zhou and Chenglei Peng, and cite the MCSoC paper. The supplied firmware is subject to its source-code licenses, rather than the data license.

## Paper figures

The authors retain copyright in `paper-figures/`. These editable figures and matching PDFs are supplied for attributed scholarly reproduction of this paper. The root MIT license does not relicense the paper or its artwork. Plotting code and independently generated analysis plots are separated from the exact camera-ready artwork.

## Public RISC-V kernels

The four kernel ports derive from the public `riscv-tests` benchmark sources vendored by the pinned Ibex checkout. Original algorithm, dataset and copyright/license notices are retained. See `benchmarks/RISCV_TESTS_LICENSE` and each source header. The original matrix kernel's atomic multicore harness is replaced by a single-core platform entry point; the arithmetic kernel and supplied 16×16 inputs are preserved. Platform hooks and startup/linker code are included beside the kernels.

## Pango Design Suite

PDS, native Pango simulation models, device libraries and programming tools are **not distributed** by this repository. Obtain the original files from your PDS installation. `scripts/run_rtl.py` checks the two required model hashes before compiling. Proprietary files must remain outside tracked source.

## VexRiscv

The comparison records identify upstream [SpinalHDL/VexRiscv](https://github.com/SpinalHDL/VexRiscv), commit `680756065e9e6fc50d8c3d6c58191a16e867d822`. Measurement records do not transfer ownership of that implementation. Any included generated RTL and configuration source retains upstream licensing, as identified beside those files.

Python/Matplotlib/NumPy, Verilator, GCC, zlib, Git and PowerShell are installed separately and retain their respective licenses. No tool executable is bundled.
