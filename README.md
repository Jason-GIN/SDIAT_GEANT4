# SDIAT_GEANT4 — 宇宙线地磁场偏转、大气簇射与轨道通量模拟

> **作者**：GIN
> **版本**：基于 Geant4 11.3.0 + ROOT + Python 3.10
> **日期**：2026-10-22（总览）

---

## 1. 项目简介

这是一套用于研究**宇宙线（主要是质子和氦核）从太空进入地球磁场、在大气中产生强子簇射、并计算卫星轨道高度粒子通量**的蒙特卡洛模拟与分析系统。核心物理链条：

```
宇宙线初级粒子（质子 / He 核）
    → 在地磁场中偏转（磁刚性 / Störmer 截止问题）
    → 到达大气层并产生强子簇射
    → 产生大量次级粒子（质子、电子、γ、中子等）
    → 部分次级粒子反向逃逸出大气，到达卫星轨道高度（~400 km）
    → 统计轨道高度上的粒子通量能谱，并与 AMS-01 实测数据对比
```

项目采用**两层递进式模拟**：第一层用 GEANT4 模拟大气簇射，第二层用 Python 数值积分地磁场反推次级粒子轨迹，判断哪些能逃逸回轨道。

---

## 2. 仓库结构

```
SDIAT_GEANT4/
├── README.md                 # 本文件（总览）
│
├── Opposite/                 # ★ GEANT4 模拟主体 + Python 两层调度
│   ├── include/              # C++ 头文件（几何、生成器、磁场、探测器等）
│   ├── src/                  # C++ 源文件
│   ├── test02.cc             # 主程序入口
│   ├── CMakeLists.txt        # 构建配置（Geant4 + ROOT）
│   ├── manager.py            # ★ Python 主控脚本（两层模拟调度）
│   ├── mypylib/              # Python 辅助库
│   │   ├── tracing.py        # ★ 地磁场粒子径迹数值积分
│   │   ├── RotationModel.py  # 坐标系转换函数
│   │   ├── myManDyna.py      # 磁力线追踪 / 磁镜像点计算
│   │   ├── myigrf14.*.so     # IGRF14 地磁场编译模块（2025 年）
│   │   └── igrf14.*.so       # IGRF14（旧版，曾修正坐标转换 bug）
│   ├── combine/              # ROOT 数据合并/筛选库（C++，libmylib）
│   ├── ctable/               # Störmer 锥方向数据 + 大气分层数据（CSV）
│   ├── run/  run_command/    # 宏文件生成 + 集群批量脚本
│   ├── result/               # 输出 ROOT 文件（HitCharge / seconddata / smac / TraceData）
│   ├── submit.py             # HTCondor 提交（htcondor2）
│   ├── submitcombine.py      # HTCondor 提交（数据合并）
│   ├── submitcombinep.py     # HTCondor 提交（质子数据合并）
│   ├── run-mac-gen.cc        # 生成 54 个能量 bin 的宏文件
│   ├── Gsubmitfile.cc        # 生成 JobWork_*.jdf（旧版提交方式）
│   ├── changecount.py        # 计算每个坐标的起始能量 bin
│   ├── run_task.sh           # 集群单个任务执行脚本
│   ├── table.txt             # 待模拟的（经度, 纬度, 起始能量bin）清单
│   ├── run.mac / test2.mac   # GEANT4 宏文件
│   ├── Spectrum.dat / data.dat
│   ├── CODE_ANALYSIS.md      # ★ 代码详解（逐模块）
│   └── PERFORMANCE_ANALYSIS.md / PERFORMANCE_ANALYSIS_2.md  # 性能分析
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

---

## 3. 完整工作流

### 3.1 两层递进模拟（Opposite/manager.py）

```
[1] GEANT4 Primary 模式
      ./test02 run.mac
      → 在 400 km 轨道高度注入宇宙线质子
      → 地磁场偏转 → 大气簇射
      → SD1（130 km）收集向上逃逸的粒子，通过 stderr 二进制管道传回 Python
          ↓
[2] Python 筛选 + 反推
      ParticleSelect() 选出质子/电子/核子
      tracing.main() 用 IGRF14 磁场 + scipy.solve_ivp 反向追踪轨迹
      → 到达 400 km 的粒子 → Parlist
      → 未到达的粒子 → Hitlist（记录击中位置）
          ↓
[3] GEANT4 Secondary 模式
      Parlist → CSV → 生成宏文件 → ./test02 <宏>
      → 每个粒子继续在大气中输运 → SD1 再收集
          ↓
[4] 循环直到不再产生可逃逸粒子
```

**关键通信机制**：GEANT4 通过 `RunAction::EndOfRunAction()` 把 `ParticleData` 结构体以二进制形式写到 `stderr`，`manager.py` 用 `subprocess.run(stderr=PIPE)` 读取并用 numpy 对齐解析。`ParticleData` 结构（11 个 double）：

```
x, y, z, dx, dy, dz, energy, PID, t, charge, m
位置(km)   动量方向(未归一)  动能(MeV) 粒子 时间(s) 电荷  质量
```

### 3.2 数据合并（Opposite/combine/）

GEANT4 用 32 线程输出，每个线程一个 ROOT 文件（`*_t0.root` ~ `*_t31.root`）。`combine/` 用 ROOT `TChain` 把它们合并成单一 ROOT 文件，并按 `ParentId != 0` 过滤出次级粒子。

### 3.3 通量分析（Analysis/）

`Analysis/` 读取合并后的 ROOT 数据，用 SDEMMA 质谱（`2012-05-13_1AU_proton.dat`）作为入射 GCR 谱，计算 400 km 轨道上的质子/电子/γ 通量，并与 AMS-01 实测对比绘图。

### 3.4 集群批量运行（HTCondor）

```
table.txt（36 个经纬度坐标 + 起始能量 bin）
    → run-mac-gen.cc 为每个坐标生成 54 个能量 bin 的宏
    → submit.py（htcondor2）提交 run_task.sh → manager.py
    → submitcombine.py / submitcombinep.py 提交数据合并
```

---

## 4. 编译与运行

### 4.1 依赖

- Geant4 11.3.0（带 UI/Vis）
- ROOT（Ntuple 输出 + 数据合并）
- CMake ≥ 3.16、GCC（C++17）
- Python 3.10 + numpy / pandas / uproot / scipy / htcondor2

### 4.2 编译 GEANT4 主程序

```bash
cd Opposite/build
cmake ..
make -j$(nproc)
```

构建后自动把 `run.mac`、`manager.py`、`Spectrum.dat` 等复制到 `build/`。

### 4.3 编译数据合并库

```bash
cd Opposite/combine/build
cmake ..
make -j$(nproc)
# 生成 libmylib.a 和 dataAnalyseTest 可执行文件
```

### 4.4 本地运行

```bash
# 单次 GEANT4 模拟
cd Opposite/build
./test02 run.mac

# 完整两层模拟
python3 manager.py ../run.mac <能量bin序号> <保存子路径>
```

### 4.5 集群批量运行

```bash
cd Opposite
g++ run-mac-gen.cc -o run-mac-gen && ./run-mac-gen   # 生成宏文件
python3 submit.py                                    # 提交到 HTCondor
```

---

## 5. 关键物理参数

| 参数 | 值 | 说明 |
|------|-----|------|
| 地球半径 | 6371.2 km | 硬编码在多处 |
| 大气层数 | 208 层（0–100 km） | 不规则分层，密度/成分随纬度和高度变化 |
| 纬度划分 | 36 条带（每 5°，90°S–90°N） | |
| SD1 高度 | 130 km | 顶部截断探测器 |
| SD2 | 地面 | 底部截断探测器 |
| 初级粒子生成高度 | 400 km（`givenum`） | 卫星轨道高度 |
| 能量分 bin | 54 个 | 质子 22 MeV – 90.268 GeV；He 22 MeV – 178.684 GeV |
| 磁场模型 | IGRF 一阶偶极（C++）/ IGRF14 2025（Python） | |
| 物理列表 | FTFP_BERT_HP | 高精度强子物理 |
| 堆栈截断能 | 15 MeV（次级粒子） | 低于此能量且非中微子则杀死 |
| 记录截断能 | SD1/SD2 ≥30 MeV，SD2 ≥5 MeV | |
| 线程数 | 32 | GEANT4 多线程 + Python 32 进程 |

---

## 6. 文档导航

- **[Opposite/CODE_ANALYSIS.md](Opposite/CODE_ANALYSIS.md)** —— 代码详解，逐模块介绍几何、生成器、磁场、探测器、Python 管理层。
- **[Opposite/PERFORMANCE_ANALYSIS_2.md](Opposite/PERFORMANCE_ANALYSIS_2.md)** —— 性能瓶颈分析与优化方案（**修订版，推荐阅读**）。
- **[Opposite/PERFORMANCE_ANALYSIS.md](Opposite/PERFORMANCE_ANALYSIS.md)** —— 性能分析初版（部分描述已被 _2 修正，仅供参考）。

---

## 7. 已知问题与注意事项

1. **硬编码路径**：`run_task.sh`、`Gsubmitfile.cc`、`run-mac-gen.cc`、`combine/dataAnalyseTest.cc` 中硬编码了 `/home/ams/lchen/...` 路径，迁移到新机器需修改。
2. **Störmer 数据依赖**：Primary 模式依赖 `ctable/all_<lon>_<lat>.csv`，缺失对应坐标的文件会导致失败。
3. **相对路径依赖**：`BinaryReader.cc` 依赖 `../ctable/`，需在 `build/` 目录下运行。
4. **坐标系约定**：涉及 Zenith（天顶）与 GTOD（地心地固）两套坐标的多次转换，Python/C++ 两侧需保持一致（曾出现过坐标变换 bug，已修正）。
