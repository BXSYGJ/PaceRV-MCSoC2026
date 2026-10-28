# PaceRV MCSoC 2026 论文复现仓库

本仓库对应论文 **Memory Admission and Result-Retention Sizing in an RV32IMC FPGA Soft Processor**，已被 MCSoC 2026 接收。作者为 Yihang Dai、Shuyu Xu、Yinfeng Zhou、Chenglei Peng，来自南京大学。

这里提供论文实验数据、完整依赖图、预测代码、四个新工作负载的 58 次执行记录、有限窗口实验 RTL、图表与复现说明。完整的处理器和上板工程在 [PaceRV 硬件仓库](https://github.com/BXSYGJ/PaceRV)，固定版本为 `v1.0.0`。

PaceRV 包含完整外设：指令/数据缓存、共享后备 BRAM、带 FIFO 的收发 UART、GPIO、64 位定时器、中断、机器态 CSR、64 位周期/退休计数器，以及动态缓存监测。论文采用 145.125 MHz 的实现点，达到 3.442 CoreMark/MHz。

## 快速复现

安装 Python 3.10 或更新版本，在仓库根目录运行：

```powershell
python scripts/reproduce.py
```

该命令校验文件哈希，重新计算主要结果，将结果写入 `output/`。逐指令检查和数据图生成：

```powershell
python -m pip install -r requirements.txt
python scripts/reproduce.py --per-instruction --plots
```

使用 Linux 或 Windows + WSL，安装 C++20 编译器和 zlib 开发文件后，可重新执行全部 64 组依赖预测计算：

```powershell
python scripts/replay.py
```

完整用法见 [复现指南](docs/REPRODUCING.md)，论文结论与文件的对应关系见 [证据索引](docs/EVIDENCE.md)。RTL 仿真使用单独安装的 PDS 原生存储器模型；FPGA 构建、下载和串口测量见固定版本 PaceRV 仓库。

`E0/E1` 是历史数据中的注册请求/提前请求模式；`owner` 表示一条动态的已准备乘法；`K` 是准备结果允许快速使用的有效窗口。论文中的不同分母和测量区间在证据索引中分别说明。
