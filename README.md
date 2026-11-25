# SDIAT_GEANT4 — 宇宙线地磁场偏转、大气簇射与轨道通量模拟

> **作者**：GIN
> **版本**：基于 Geant4 11.3.0 + ROOT + Python 3.10
> **日期**：2026-10-22（总览）→ 2026-11-11（整合可衰变粒子追踪 `tracingDecay.py`）→ 2026-11-23（6 种初级核素、轨道观测点、可开关大气探测器）

---

## 1. 项目简介

这是一套用于研究**宇宙线（主要是质子和氦核）从太空进入地球磁场、在大气中产生强子簇射、并计算卫星轨道高度粒子通量**的蒙特卡洛模拟与分析系统。核心物理链条：

```
宇宙线初级粒子（质子 / He 核 / C / O / Si / Fe）
    → 在地磁场中偏转（磁刚性 / Störmer 截止问题）
    → 到达大气层并产生强子簇射
    → 产生大量次级粒子（质子、电子、γ、中子等）
    → 部分次级粒子反向逃逸出大气，到达卫星轨道高度（~400 km）
    → 统计轨道高度上的粒子通量能谱，并与 AMS-01 实测数据对比
```

项目采用**两层递进式模拟**：第一层用 GEANT4 模拟大气簇射，第二层用 Python 数值积分地磁场反推次级粒子轨迹，判断哪些能逃逸回轨道。

**本次（2026-11）新增能力**：
1. **初级粒子扩展到 6 种核素**：H、He、C、O、Si、Fe，各自一套 54 bin 能量网格（`E[6][54]`）。
2. **轨道观测点**：追踪阶段在 5 个海拔（380 / 400 / 565 / 600 / 700 km）设置非终止观测事件，记录粒子穿越这些高度时的状态，用于计算不同轨道高度的通量。
3. **可开关大气探测器**：新增宏命令 `/Geo/SphereDetector on/off`，可在不需要大气内部逐公里记录时关闭 SD4/SDinAir 以大幅提速。

---

## 2. 仓库结构

```
SDIAT_GEANT4/
├── README.md                 # 本文件（总览）
├── BUG_REPORT.md             # 代码 Bug 审查报告（含严重程度分级与修复方案）
│
├── Opposite/                 # ★ GEANT4 模拟主体 + Python 两层调度
│   ├── include/              # C++ 头文件（几何、生成器、磁场、探测器、Messenger 等）
│   ├── src/                  # C++ 源文件
│   │   ├── DetectorMessenger.cc  # ★ 新增：/Geo/lat、/Geo/lon、/Geo/SphereDetector 命令
│   │   ├── PrimaryGeneratorAction.cc # ★ 6 种核素能量网格 + 方向采样
│   │   ├── DetectorConstruction.cc  # ★ 大气探测器开关（fSDet）
│   │   ├── RunAction.cc       # ★ stderr 二进制 MAGIC 输出（EventInfo/DetectionInfo/BoundaryInfo）
│   │   └── ...
│   ├── test02.cc             # 主程序入口
│   ├── CMakeLists.txt        # 构建配置（Geant4 + ROOT）
│   ├── manager.py            # ★ Python 主控脚本（两层模拟调度）
│   ├── Buildrunmac.cc        # ★ 宏批量生成器（6 核素 × 54 能量 × 36 坐标）
│   ├── mypylib/              # Python 辅助库
│   │   ├── tracing.py        # ★ 地磁场稳定粒子径迹数值积分（含轨道观测点）
│   │   ├── tracingDecay.py   # ★ 可衰变粒子追踪 + 衰变产物生成（pyFANG/particle）
│   │   ├── RotationModel.py  # 坐标系转换（含地理↔地磁坐标 Geo2Mgeo1）
│   │   ├── myManDyna.py      # 磁力线追踪 / 磁镜像点计算
│   │   ├── igrf14.f          # IGRF14 地磁场源码（f2py 编译）
│   │   ├── wrapper.f90       # f2py 接口：igrf_wrapper / igrf_wrapper_array / rhs_particle / rhs_dparticle
│   │   └── myigrf14.*.so     # IGRF14 2025 编译模块（f2py 产物）
│   ├── combine/              # ROOT 数据合并/筛选库（C++，libmylib）
│   ├── ctable/               # Störmer 锥方向数据 + 大气分层数据（CSV）
│   ├── run/  run_command/    # 宏文件生成 + 集群批量脚本
│   ├── submit.py             # HTCondor 提交（htcondor2）
│   ├── submitcombine.py      # HTCondor 提交（数据合并）
│   ├── submitcombinep.py     # HTCondor 提交（质子数据合并）
│   ├── changecount.py        # 计算每个坐标的起始能量 bin
│   ├── run_task.sh           # 集群单个任务执行脚本
│   ├── table.txt             # 待模拟的（经度, 纬度, 起始能量bin）清单
│   ├── run.mac / test2.mac   # GEANT4 宏文件
│   ├── Spectrum.dat / data.dat
│   ├── CODE_ANALYSIS.md      # ★ 代码详解（逐模块）
│   ├── PERFORMANCE_ANALYSIS.md / _2.md / _3.md  # 性能分析（_3 为最新）
│   └── MEMORY_ISSUE_ANALYSIS.md                 # 追踪内存溢出分析与修复方案
│
└── Analysis/                 # ★ 通量分析 + 绘图（对比 AMS-01）
    ├── BuildProtonFluxOnE_Orbit_{D,U}.py     # 质子通量能谱（向下/向上）
    ├── BuildElectronFluxOnE_Orbit_{D,U}.py   # 电子通量能谱
    ├── BuildGammaFluxOnAzimuth_Orbit.py      # γ 通量方位角分布
    ├── BuildGammaFluxOnZenith_Orbit.py       # γ 通量天顶角分布
    ├── BuildGammaFluxOnCutoff.py             # γ 通量截止
    └── mypylib/             # FluxCalculation / FilterModel / dipole /
                             # ToHigherOrbit / myutils / igrf14.f 等
```

> 注：旧版宏生成器 `run-mac-gen.cc` 与 `Gsubmitfile.cc` 已被 `Buildrunmac.cc` 取代。

---

## 3. 完整工作流

### 3.1 两层递进模拟（Opposite/manager.py）

```
[1] GEANT4 Primary 模式
      ./test02 run.mac
      → 在 400 km 轨道高度注入宇宙线初级核（H/He/C/O/Si/Fe 之一）
      → 地磁场偏转 → 大气簇射
      → SD1（130 km）收集向上逃逸的粒子，通过 stderr 二进制管道传回 Python
          ↓
[2] Python 筛选 + 反推
      ParticleSelect1() 剔除 γ/中微子后，把粒子分为两类：
        ├── 稳定粒子（e±/p/p̄/原子核）→ tracing.main()
        └── 可衰变粒子（μ/π/K/n 等）→ tracingDecay.main()
              追踪到衰变弧长时用 pyFANG 生成衰变产物，产物并入下一轮
      tracing.main() / tracingDecay.main() 用 IGRF14 磁场 + scipy.solve_ivp 追踪轨迹
      → 撞到 90 km 稠密大气 → Hitlist（记录击中位置）
      → 逃逸到 400 km → Parlist（进入下一轮 Secondary）
      → 沿途在 5 个轨道观测点（380/400/565/600/700 km）记录穿越状态 → Hitlist 的 Height 列
          ↓
[3] GEANT4 Secondary 模式
      Parlist → CSV → 生成宏文件 → ./test02 <宏>
      → 每个粒子继续在大气中输运 → SD1 再收集
          ↓
[4] 循环直到不再产生可逃逸粒子（while pnum>0）
```

**关键通信机制**：GEANT4 通过 `RunAction::EndOfRunAction()` 把 `ParticleData` 结构体以二进制形式写到 `stderr`，`manager.py` 用 `subprocess.run(stderr=PIPE)` 读取。为避免被 GEANT4 自身的 `G4cerr` 日志（"G4WT…"）污染，二进制流以 8 字节 MAGIC 开头：

```
[ MAGIC 8B ][ count 4B ][ ParticleData × count ]
```

- MAGIC：`0x47494E5F50415254`（小端字节序 `b"TRAP_NIG"`，Python 端用 `stderr.find(MAGIC)` 定位）
- count：`uint32` 小端，粒子数
- `ParticleData`（11 个 double，88 字节/粒子）：

```
x, y, z, dx, dy, dz, energy, PID, t, charge, m
位置(km)   动量方向(未归一)  动能(MeV) 粒子 时间(s) 电荷  质量
```

**Hitlist 输出字段**（`DataSetting()` 后处理 + `SaveRoot()` 落盘）：在原有 `位置/速度/动能/PID/电荷/task_id/时间` 基础上，追加了每个击中位置的地磁信息：

```
PosX, PosY, PosZ, DirX, DirY, DirZ, Energy, PID, Charge, task_id, Time,
Height,           # 轨道观测点海拔（km）
Bx, By, Bz,       # IGRF14 磁场三分量（nT）
MLat, MLon        # Apex 磁纬/磁经（apexpy，2025.0）
```

### 3.2 数据合并（Opposite/combine/）

GEANT4 用 32 线程输出，每个线程一个 ROOT 文件（`*_t0.root` ~ `*_t31.root`）。`combine/` 用 ROOT `TChain` 把它们合并成单一 ROOT 文件，并按 `ParentId != 0` 过滤出次级粒子。

### 3.3 通量分析（Analysis/）

`Analysis/` 读取合并后的 ROOT 数据，用 SDEMMA 质谱（`2012-05-13_1AU_proton.dat`）作为入射 GCR 谱，计算 400 km 轨道上的质子/电子/γ 通量，并与 AMS-01 实测对比绘图。

### 3.4 集群批量运行（HTCondor）

```
table.txt（36 个经纬度坐标 + 起始能量 bin）
    → Buildrunmac.cc 为每个坐标生成 6 种核素 × 54 个能量 bin 的宏
    → submit.py（htcondor2）提交 run_task.sh → manager.py
    → submitcombine.py / submitcombinep.py 提交数据合并
```

---

## 4. 编译与运行

### 4.1 依赖

- Geant4 11.3.0（带 UI/Vis）
- ROOT（Ntuple 输出 + 数据合并）
- CMake ≥ 3.16、GCC（C++17）、GNU Fortran（f2py 编译 IGRF14）
- Python 3.10 + numpy / pandas / uproot / scipy / htcondor2 / apexpy
- 衰变追踪额外依赖：`particle`、`particletools`（PDG 质量/衰变道表）、`pyFANG`（多体相空间衰变生成）

### 4.2 编译 GEANT4 主程序

```bash
cd Opposite/build
cmake ..
make -j$(nproc)
```

构建后自动把 `run.mac`、`manager.py`、`Spectrum.dat` 等复制到 `build/`。

### 4.3 编译 IGRF14 模块（f2py）

`mypylib/myigrf14.so` 由 `igrf14.f` + `wrapper.f90` 经 f2py 编译：

```bash
cd Opposite/mypylib
f2py -c igrf14.f wrapper.f90 -m myigrf14
```

生成 `myigrf14.cpython-310-x86_64-linux-gnu.so`，提供 `igrf_wrapper`、`igrf_wrapper_array`、`rhs_particle`（6 维）、`rhs_dparticle`（7 维含弧长）四个接口。

### 4.4 编译数据合并库

```bash
cd Opposite/combine/build
cmake ..
make -j$(nproc)
# 生成 libmylib.a 和 dataAnalyseTest 可执行文件
```

### 4.5 本地运行

```bash
# 单次 GEANT4 模拟
cd Opposite/build
./test02 run.mac

# 完整两层模拟
python3 manager.py ../run.mac <能量bin序号> <保存子路径>
```

### 4.6 集群批量运行

```bash
cd Opposite
g++ Buildrunmac.cc -o Buildrunmac && ./Buildrunmac   # 生成 6 核素宏文件
python3 submit.py                                    # 提交到 HTCondor
```

---

## 5. 关键物理参数

| 参数 | 值 | 说明 |
|------|-----|------|
| 地球半径 | 6371.2 km | 硬编码在多处 |
| 大气层数 | 208 层（0–100 km） | 不规则分层（指数分层），密度/成分随纬度和高度变化 |
| 纬度划分 | 36 条带（每 5°，90°S–90°N） | |
| SD1 高度 | 130 km | 顶部截断探测器 |
| SD2 | 地面 | 底部截断探测器 |
| 初级粒子生成高度 | 400 km（`givenum`） | 卫星轨道高度 |
| 初级核素 | 6 种：H / He / C / O / Si / Fe | 各 54 个能量 bin（见下表） |
| 轨道观测点 | 380 / 400 / 565 / 600 / 700 km | Python 追踪层非终止观测事件 |
| 磁场模型 | 全局偶极（C++）/ IGRF14 2025（Python） | C++ `/field/SetType globaldipole` |
| 物理列表 | FTFP_BERT_HP | 高精度强子物理 |
| 堆栈截断能 | 5 MeV（次级粒子） | 低于此能量且非中微子则杀死 |
| 记录截断能 | SD1/SD2 ≥5 MeV，轨道观测 ≥30 MeV | |
| 线程数 | 32 | GEANT4 多线程 + Python 32 进程 |

**6 种初级核素的能量网格**（单位 GeV，各 54 bin，定义在 `PrimaryGeneratorAction.hh` 与 `Buildrunmac.cc` 的 `E[6][54]`）：

| 核素 | Z | A | 能量范围（GeV） |
|------|---|---|-----------------|
| H（质子） | 1 | 1 | 0.022 – 90.268 |
| He | 2 | 4 | 0.022 – 178.684 |
| C | 6 | 12 | 0.067 – 536.141 |
| O | 8 | 16 | 0.089 – 714.856 |
| Si | 14 | 28 | 0.156 – 1250.917 |
| Fe | 26 | 56 | 0.270 – 2319.699 |

---

## 6. 文档导航

- **[Opposite/CODE_ANALYSIS.md](Opposite/CODE_ANALYSIS.md)** —— 代码详解，逐模块介绍几何、生成器、磁场、探测器、Python 管理层。
- **[Opposite/PERFORMANCE_ANALYSIS_3.md](Opposite/PERFORMANCE_ANALYSIS_3.md)** —— 性能瓶颈分析与优化方案（**最新版，推荐阅读**）。
- **[Opposite/PERFORMANCE_ANALYSIS_2.md](Opposite/PERFORMANCE_ANALYSIS_2.md)** —— 性能分析修订版（部分描述已被 _3 修正）。
- **[Opposite/PERFORMANCE_ANALYSIS.md](Opposite/PERFORMANCE_ANALYSIS.md)** —— 性能分析初版（仅供参考）。
- **[Opposite/MEMORY_ISSUE_ANALYSIS.md](Opposite/MEMORY_ISSUE_ANALYSIS.md)** —— 追踪内存溢出根因分析与修复方案（maxstep 单位 + t_eval）。
- **[BUG_REPORT.md](BUG_REPORT.md)** —— 代码 Bug 审查报告（含严重程度分级与修复方案）。

---

## 7. 已知问题与注意事项

1. **硬编码路径**：`Buildrunmac.cc`（`/home/ams/lchen/GEANT4/test-sphere/G4py/belt0405/result/...`）、`run_task.sh`、`combine/dataAnalyseTest.cc` 中硬编码了 `/home/ams/lchen/...` 路径，迁移到新机器需修改。
2. **Störmer 数据依赖**：Primary 模式依赖 `ctable/all_<lon>_<lat>.csv`，缺失对应坐标的文件会导致失败。
3. **相对路径依赖**：`BinaryReader.cc` 依赖 `../ctable/`，需在 `build/` 目录下运行。
4. **坐标系约定**：涉及 Zenith（天顶）、GTOD（地心地固）、地理/地磁（Apex）等多套坐标的多次转换，Python/C++ 两侧需保持一致（曾出现过坐标变换 bug，已修正）。
5. **能量网格精确匹配**：Primary 模式下 `/gun/energy` 必须严格等于 `E[6][54]` 中某个 bin 值（经 `unordered_map` 精确查找），否则 `targetrow = -1` 会触发方向缓存越界。批量宏由 `Buildrunmac.cc` 生成，可保证一致。
