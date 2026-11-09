# SDIAT_GEANT4/Opposite — 性能瓶颈分析与优化方案（第三版）

> **日期**: 2026-10-25
> **性质**: 第三版，基于**当前代码实际状态**全文重审。前两版（`PERFORMANCE_ANALYSIS.md` 初版、`PERFORMANCE_ANALYSIS_2.md` 修订版）中有多处与实际代码不符或已过时的结论，本版以现有代码为准逐条订正。
> **分级**: 🔴 严重瓶颈 | 🟠 中等瓶颈 | 🟡 轻微影响 | ✅ 已优化 | 🆕 本版新增发现

> ⚠️ **本版与前两版最重要的差异**：前两版均**漏掉了编译优化级别**——当前整个 GEANT4 模拟是以 `-O0`（Debug，无任何优化）编译的。这是零风险、收益最大的加速项，排在所有优化之前。

---

## 目录

1. [总体性能画像](#1-总体性能画像)
2. [🔴 头号瓶颈：Debug(-O0) 编译](#2-头号瓶颈debug-o0-编译)
3. [C++ GEANT4 侧模块分析](#3-c-geant4-侧模块分析)
   - [3.1 MyMagneticField（真正的热路径）](#31-mymagneticfield真正的热路径)
   - [3.2 DetectorConstruction::Construct（被循环放大的初始化成本）](#32-detectorconstructionconstruct被循环放大的初始化成本)
   - [3.3 PrimaryGeneratorAction](#33-primarygeneratoraction)
   - [3.4 TrackerSD / SphericalSystemSD 每步开销](#34-trackersd--sphericalsystemsdinair-每步开销)
   - [3.5 SteppingAction / MyStackingAction](#35-steppingaction--mystackingaction-微优化)
4. [Python 侧模块分析](#4-python-侧模块分析)
5. [系统级瓶颈](#5-系统级瓶颈)
6. [综合优化方案（按优先级）](#6-综合优化方案按优先级)
7. [预期性能提升汇总](#7-预期性能提升汇总)
8. [实施优先级路线图](#8-实施优先级路线图)
9. [前两版结论订正表](#9-前两版结论订正表)

---

## 1. 总体性能画像

### 端到端流程回顾（以现有 `manager.py` 为准）

```
manager.py main(runmac, Egnum, mypth)
  └─ GEANT4RuningFun(runmac)                     # 第 1 次 GEANT4 进程：Primary 模式
       └─ ./test02 run_H_x.mac / run.mac         # /run/beamOn 10000~80000 事件
       └─ 输出 → stderr 二进制(count + ParticleData×N)
  └─ ParticleSelect(particles)                   # 筛 质子/电子/核子
  └─ while (pnum > 0)  (最多 ~200 次)            # 🔴 每次循环 = 一次完整 GEANT4 进程重启
       └─ LoopBasic(...)
            ├─ tracing.main × 32 进程            # solve_ivp 反推每个粒子的地磁轨迹
            ├─ 幸存粒子 → 写 CSV → Gmacfile
            └─ GEANT4RuningFun(strmac)           # 第 2..N 次 GEANT4 进程：Secondary 模式
                 └─ /run/initialize → Construct()  # 🔴 每次重建全部几何/Material
```

### 一次 Primary 运行（10000 事件）的耗时分布估算

| 阶段 | 估算占比 | 主要瓶颈 |
|------|----------|----------|
| GEANT4 粒子输运 + 物理（**-O0 下运行**） | 60-75% | 强子簇射物理、磁场查询、输运步进 |
| GEANT4 初始化 `Construct()` | 5-15% | 11088 逻辑体 + 7488 Material + 36 次 CSV |
| Python `tracing.main`（solve_ivp 反推） | 15-25% | IGRF14 + 洛伦兹力积分 |
| 文件 I/O / ROOT 写入 / stderr 管道 | 3-8% | CSV 写、ROOT ntuple、二进制传输 |

### 关键数字（当前代码实测/推算）

| 指标 | 当前值 | 说明 |
|------|--------|------|
| C++ 编译优化级别 | **-O0**（Debug） | 🔴 头号问题，见第 2 节 |
| 磁场热路径 | `GlogalDipoleField` | 所有宏都用 `globaldipole` |
| 逻辑体总数 | ~11088（7488 SD3 + 3600 SD4） | 每次 GEANT4 进程重启都重建 |
| G4Material 数量 | 7488 | 208 层 × 36 纬度带 |
| `readAtmLayer` CSV 读取 | 36 次/进程 | 每次还 `G4cout` 一条 "GINs" |
| GEANT4 进程重启次数 | 1 + while 循环次数（≤200） | 🔴 初始化成本被循环放大 |
| 重叠检查 `checkOverlaps` | `true` | 在 11088 个球壳上做重叠检测 |
| 磁场积分最小步长 `minStep` | `1 cm` | 对 km 尺度过于保守 |

---

## 2. 🔴 头号瓶颈：Debug(-O0) 编译

> 🆕 **这是本版最重要的发现，前两版完全漏掉。**

### 现状（已核实）

`Opposite/CMakeLists.txt` 第 48-50 行：

```cmake
SET(CMAKE_BUILD_TYPE "Debug")                                          # 第 48 行
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")        # 第 49 行
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")                # 第 50 行
```

实际编译产物 `build/CMakeFiles/Test02.dir/flags.make`：

```
CXX_FLAGS =  -O0 -Wall -g2 -ggdb -fPIC
```

`build/test02` 是 `with debug_info, not stripped` 的 ELF。

#### ⚠️ 为什么同时写了 DEBUG 和 RELEASE 两套 flags，结果还是 -O0？

`CMAKE_CXX_FLAGS_DEBUG` 与 `CMAKE_CXX_FLAGS_RELEASE` 同时定义**本身没有错**（CMake 本就支持多套配置）。但**真正决定用哪一套的是 `CMAKE_BUILD_TYPE`**：

| `CMAKE_BUILD_TYPE` | 实际使用的 flags |
|---|---|
| `Debug` | `CMAKE_CXX_FLAGS_DEBUG` → `-O0` |
| `Release` | `CMAKE_CXX_FLAGS_RELEASE` → `-O3` |

第 48 行 `SET(CMAKE_BUILD_TYPE "Debug")` **把构建类型钉死在 Debug**，于是每次都走 `-O0` 那套，第 50 行的 `-O3` RELEASE flags **永远不被使用**。

所以正确修法**不是**"把 `-O0` 改成 `-O3`"（那样会触发上一节说的 `DirectionData.cc` 编译卡死，而且 Debug 构建里跑优化代码语义也混乱），而是**把 `CMAKE_BUILD_TYPE` 从 Debug 切到 Release**。

### 影响量化

- **`-O0` 表示完全不优化**：没有函数内联、没有寄存器分配优化、没有循环优化、没有常量折叠/提升、没有 SIMD 自动向量化。
- 对于 GEANT4 这种**数值密集 + 大量小函数调用**的工作负载（物理过程、磁场查询、输运积分器、导航），`-O0` 相对 `-O3` 通常慢 **3-5 倍**，个别数值/循环密集处可到 10 倍。
- 尤其关键：`GlogalDipoleField` 每步都要算 `sqrt`/`atan2`/`sin`/`cos`，在 `-O0` 下这些函数调用无法内联、局部常量无法提升，且每次调用都伴随完整的栈帧建立/销毁。
- **所有其他优化（磁场重写、缓存、对象池……）加起来，都可能不如单纯把 `-O0` 换成 `-O3` 带来的提升大。**

### 修复方案（完整三步）

#### 第 1 步：把构建类型切到 Release（核心，改的是 `CMAKE_BUILD_TYPE`，不是 flags）

两种等价做法，选其一：

**做法 A（命令行，最快）**

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

**做法 B（改 `CMakeLists.txt`，推荐，避免以后又退回 Debug）**

把第 48-50 行替换为：

```cmake
# 不硬编码 Debug；若命令行未显式指定，则默认 Release
if(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# 各配置的优化级别（覆盖 CMake 默认值）
SET(CMAKE_CXX_FLAGS_DEBUG   "-O0 -Wall -g")       # 调试时用
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall") # 生产/提速用
```

说明：
- `CMAKE_BUILD_TYPE` 是 cache 变量，普通 `SET(CMAKE_BUILD_TYPE "Debug")` 只在当前生成里临时覆盖；要可靠生效，用上面带 `CACHE ... FORCE` 的写法，或命令行 `-DCMAKE_BUILD_TYPE=Release`。
- `-DNDEBUG` 关掉 `assert`，进一步提速，对已调通的物理流程无影响。
- 要保留 Debug 能力时，另开 `build-debug/` 目录，别在生产目录里用 `-O0`。

#### 第 2 步（强烈推荐）：给 `DirectionData.cc` 单独降优化，否则 -O3 编译会卡死

`src/DirectionData.cc`（484KB、13963 元素的静态方向表）在 `-O3` 下编译极慢、单文件吃数 GB 内存（8 核并行时内存打满触发 swap → 表现为"卡住"）。它只是静态查找表，`-O3` 对它几乎零运行时收益。在 `CMakeLists.txt` 加一行：

```cmake
# 巨型静态方向表：-O3 编译极慢且无收益，单独降为 -O1
set_source_files_properties(src/DirectionData.cc PROPERTIES COMPILE_OPTIONS "-O1")
```

这样 `-O3` 的收益保留给真正的热路径（磁场/输运/SD），大表文件用 `-O1` 秒编译。

#### 第 3 步：并行度别拉满

```bash
make -j4        # 8 核机器用 -j8 会让多个大文件同时 -O3，内存打满触发 swap
```

编译期间可另开终端 `htop` 观察 `cc1plus` 内存，确认没有吃满内存/swap。

### 预期提升

- C++ 端（输运 + 物理 + 磁场 + 导航）整体 **2.5-5 倍**加速。
- 因为 C++ 端占端到端 60-75%，折合**端到端约 1.8-3 倍**加速。
- **成本为零，风险几乎为零**，应作为第一步立即执行（配合第 2 步的 `-O1` 例外，编译才不会卡住）。

---

## 3. C++ GEANT4 侧模块分析

### 3.1 MyMagneticField（真正的热路径）

#### 订正前两版的错误

前两版（尤其 v2 的 2.2 节）把 `DipoleField` 当作残余瓶颈重点分析。但**当前所有宏文件**（`run.mac`、`run/run_H/*.mac`、`run/run_He/*.mac`）都用：

```
/field/SetType globaldipole
```

所以热路径是 **`GlogalDipoleField`**（`MyMagneticField.cc:222-272`），而不是 `DipoleField`。`DipoleField` 当前根本不会被调用，优化它是无效功。

#### ✅ 已优化部分（v2 的结论仍然成立）

- `GetFieldValue` 已用 `enum class FieldType` 的 `switch` 分发（`MyMagneticField.cc:113-118`），不再是字符串比较。
- `GlogalDipoleField` 已直接使用局部 `double`，消除了 `G4ThreeVector` 临时对象和 `VectorZenithinGTOD1` 函数调用（`MyMagneticField.cc:249-263`）。

#### 🟠 残余瓶颈：每步的 `atan2` + `sin/cos`

当前 `GlogalDipoleField`（`MyMagneticField.cc:249-258`）每个带电粒子每一步都要执行：

```cpp
double r   = std::sqrt(x*x + y*y + z*z);
double cos_theta = z / r;
double sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);
double phi  = std::atan2(y, x);          // ← atan2
double cos_phi = std::cos(phi), sin_phi = std::sin(phi);  // ← 2 次 trig
```

**问题**：`atan2` + `cos` + `sin` 是这里最贵的部分。但 `phi` 的 `cos/sin` 完全可以从直角坐标直接得到，无需经过 `atan2` 往返：

```cpp
double r    = std::sqrt(x*x + y*y + z*z);
double rho  = std::sqrt(x*x + y*y);      // 1 次 sqrt，替换 atan2
double cos_theta = z / r;
double sin_theta = rho / r;              // 替换 sqrt(1-cos²)
double cos_phi = x / rho;                // 替换 cos(atan2)
double sin_phi = y / rho;                // 替换 sin(atan2)
```

- 用「1 次 `sqrt` + 2 次除法」替换「1 次 `atan2` + 2 次 `sin/cos` + 1 次 `sqrt`」。
- `atan2`（~150 cycles）比 `sqrt`（~30 cycles）贵得多；`sin/cos` 各 ~50-100 cycles。
- 单次调用省下约 200-300 cycles，在每步 × 每个带电粒子 × 32 线程的热路径上累积显著（尤其 `-O0` 下 `atan2/sin/cos` 是库函数调用）。

**进一步**：`a³`、`g0/g1/h1` 目前作为局部 `const` double 每调用声明一次（`MyMagneticField.cc:234-237`）。在 `-O3` 下编译器会自动提升；在 `-O0` 下不会。切 Release 后此点自然解决，但也可以顺手把它们提为 `static constexpr` 成员，语义更清晰。

#### 预期提升

- GlogalDipoleField 函数体单次调用加速 ~30-50%。
- 折合端到端（磁场查询占输运的一部分）约 5-10%。

---

### 3.2 DetectorConstruction::Construct（被循环放大的初始化成本）

> 🆕 前两版把 `Construct()` 当成"一次性的初始化瓶颈"（🟡），认为影响不大。**但实际上它在 `manager.py` 的 while 循环里被重复执行最多 200 次**，成本被放大了两个数量级。

#### 问题描述

`DetectorConstruction::Construct()`（`DetectorConstruction.cc:84-383`）每次 `/run/initialize` 都会执行以下工作：

1. **7488 个 G4Material**（`DetectorConstruction.cc:279`）：208 层 × 36 纬度带，每个 `new G4Material` + 3 次 `AddElement`。
2. **~11088 个逻辑体**：SD3 大气层 7488 个（`DetectorConstruction.cc:302-316`）+ SD4 探测器层 3600 个（`DetectorConstruction.cc:348-371`）。
3. **36 次 `readAtmLayer` CSV 读取**（`DetectorConstruction.cc:266`），每次读一个 16KB 文件，且 `readAtmLayer` 内部还 `G4cout << "---------------GINs---------------"`（`BinaryReader.cc:129`）一条输出。
4. **`checkOverlaps = true`**（`DetectorConstruction.cc:95`）：在 11088 个球壳上做几何重叠检查，这是构建阶段很贵的一步。

而 `manager.py` 的流程是（`manager.py:269-277`）：

```python
while (pnum > 0):
    ...
    tparticlelist, pnum = LoopBasic(tparticlelist, Egnum, count, mypth)
    count = count + 1   # 最多 ~200 次
```

每次 `LoopBasic` 内部 `GEANT4RuningFun(strmac)`（`manager.py:243`）都会**启动一个全新的 GEANT4 进程**，重新执行 `/run/initialize` → `Construct()`。

> 结果：**每跑一个能量 bin 的完整反推，可能重启 GEANT4 几十到 200 次**，每次都重建 11088 逻辑体 + 7488 Material + 读 36 个 CSV + 做重叠检查。这部分的累计耗时非常可观（每次进程初始化可能数秒到十几秒，200 次就是几分钟到几十分钟）。

#### 优化方案（按收益/风险排序）

**方案 A（P0，低风险）：关闭重叠检查**

```cpp
G4bool checkOverlaps = false;   // 生产运行，几何已调通，无需每进程做重叠检测
```

重叠检查在 11088 个球壳（大量同心球壳 + 纬度分段）上成本很高。几何一旦验证无误，生产运行应关闭。**预计单次 `Construct()` 构建时间减少 20-40%。**

**方案 B（P0，低风险）：`readAtmLayer` 的 G4cout 移除或条件化**

`BinaryReader.cc:129` 的 `G4cout << "---------------GINs---------------"` 在每次读大气层文件时都打印。虽然只是 36 次/进程，但在反复重启下累积。直接删除或加 `if (verboseLevel > 0)`。

**方案 C（P1，中风险）：合并多次 Secondary run 为一次进程**

这是架构级优化，收益最大但改动较大。核心思路：不要每轮 `LoopBasic` 都重启 GEANT4，而是：

- 在 `Gmacfile` 里一次 `beamOn` 跑完当前所有幸存粒子（现在已经这么做，但每轮幸存粒子仍需新进程）。
- 更好的做法：在 GEANT4 侧支持"动态追加粒子源"，或用 `/generator/Readfile` 一次读入多个 batch 的粒子，减少进程重启次数。
- 或者：把 `Secondary` 阶段的粒子按 batch 合并，减少 while 循环的进程数。

**方案 D（P2，高收益高风险）：减少逻辑体/Material 数量**

- 7488 个 SD3 大气层体积中，很多相邻层的**物质组成相同、仅密度不同**（N₂/O₂/Ar 比例接近）。可考虑用更少的 Material 模板 + 密度参数化。
- 若物理允许，把 36 纬度带降到实际需要的几个（例如只模拟 1 个地理位置的 ±5° 范围），逻辑体数量可减为 1/36。
- 这是最激进也最有效的几何瘦身，但需要确认物理正确性。

#### 预期提升

- 方案 A+B：单次 `Construct()` 构建时间减 20-40%，且在 200 次循环下直接体现。
- 方案 C：消除大部分重复初始化，端到端 Secondary 阶段加速数倍。
- 方案 D：构建时间 + 导航性能双提升。

---

### 3.3 PrimaryGeneratorAction

#### 状态确认

- ✅ `DirectionMaskCache`（`PrimaryGeneratorAction.hh:95-146`）已实现：`load()` 首次读盘、后续命中缓存；`getRandomAllowed()` 用预计算的 `allowedIndices` 做 O(1) 随机选择，**已消除 v1/v2 担心的每事件文件 I/O 和拒绝采样**。
- ✅ 能量→行号用 `std::unordered_map<double,int>`（`PrimaryGeneratorAction.cc:888-897`），O(1)。

#### 🟠 残余问题 1：`fCachedCosColat` 等成员是死代码（从未使用）

`PrimaryGeneratorAction.hh:150-154` 声明了：

```cpp
double fCachedCosColat = 0.0;
double fCachedSinColat = 0.0;
double fCachedCosLon = 0.0;
double fCachedSinLon = 0.0;
bool   fTrigCached = false;
```

但 `GeneratorDirectionCSV1`（`PrimaryGeneratorAction.cc:693-729`）**并没有使用它们**，仍然每事件调用：

```cpp
ddp = VectorZenithinGTOD(dp, G4ThreeVector(x, y, z));   // 第 723 行
```

`VectorZenithinGTOD`（`PrimaryGeneratorAction.cc:474-485`）内部调用 `xyz2r`（`sqrt + acos + atan2`）再算 4 次 trig。虽然 `lat/lon` 在整个 run 不变，但这里对**粒子方向 `dp` 的旋转**而言，`xyz2r(xyz)` 里的 `acos/atan2` 对每个事件的位置 `(x,y,z)` 是变化的（位置随机），所以这部分 trig 无法全部预计算——但 `VectorZenithinGTOD` 里的 `cos_p/sin_p/cos_t/sin_t` 是基于位置的，每事件都要算。

- 结论：v2 里"预计算 lat/lon trig 消除每事件 trig"的方案**不适用于当前 `GeneratorDirectionCSV1` 路径**（当前路径只做一次 `VectorZenithinGTOD`，不涉及 `(90°-lat)`/`lon` 的旋转矩阵）。那 4 个缓存成员应删除，避免误导。
- 真正的每事件 trig 是 `VectorZenithinGTOD` 内的 `xyz2r`（位置相关）+ 4 trig，这部分单次成本低（~几百 ns），且每事件只调 1 次，**不是主要瓶颈**，优先级低。

#### 🟠 残余问题 2：能量查找的精确匹配（正确性 bug，非性能）

`PrimaryGeneratorAction.cc:888-897`：

```cpp
if (Pname == "proton") {
    auto it = fProtonEnergyToRow.find(Ek);
    targetrow = (it != fProtonEnergyToRow.end()) ? it->second : -1;
} else {
    auto it = fHeliumEnergyToRow.find(Ek);
    targetrow = (it != fHeliumEnergyToRow.end()) ? it->second : -1;
}
```

`std::unordered_map<double,int>::find` 对 `double` 仍做**精确位匹配**；`run_H`/`run_He` 用 `/gun/particle ion`（`Pname != "proton"`，错查 E2）+ 四舍五入能量 → miss → `targetrow = -1` → `getRandomAllowed(-1)` → `allowedIndices.at(-1)` 抛 `out_of_range`。

- 这是**崩溃 bug**（已在 `BUG_REPORT.md` 记为 🔴-3），修复方案是最近邻 bin 搜索 + 按粒子种类选 E1/E2。
- 从性能角度它本身不耗时间，但会导致 `run_H`/`run_He` 全部崩溃，**必须先修才能谈性能**。详见 `BUG_REPORT.md`，此处不重复展开。

#### 🟡 残余问题 3：`DirectionMaskCache::load()` 用 stringstream 解析

`PrimaryGeneratorAction.hh:120-131` 对 54 行 × 13963 列用 `std::stringstream` + `std::getline(ss, cell, ',')` 逐 cell 解析，约 75 万次字符串操作。这是一次性初始化成本（每次进程 1 次），在 `-O0` 下较慢。可用裸 `const char*` 指针扫描逗号来显著加速（切 `-O3` 后 stringstream 也会快很多）。优先级低。

---

### 3.4 TrackerSD / SphericalSystemSDinAir 每步开销

#### 状态确认

- ✅ `TrackerSD`/`SphericalSystemSDinAir` 的热路径 `G4cout` 调试输出已基本注释掉。
- ✅ Hit 用 `G4Allocator` 线程本地池（`TrackerHit.hh:103-115`、`SphericalSystemHitinAir.hh:110-122`），`new` 不再走 malloc。

#### 🟠 问题 1：`SphericalSystemSDinAir::ProcessHits` 没有 early-return

`SphericalSystemSDinAir.cc:101-220`：进入函数后**无条件** `new SphericalHitinAir()`、`Set*` 20 个字段、做父粒子 `localMap->find`、插入 HitsCollection，直到第 192 行才判断：

```cpp
if (kE >= 30.*MeV) { /* 填 Ntuple */ }
```

意味着大量**能量低于 30 MeV** 的粒子步也完整走了一遍 Hit 创建 + 字段填充。而 SD4 有 3600 个敏感体积（100 层 × 36 纬度带），低能次级粒子在海平面附近大量产生，这些无效 Hit 的创建/填充/回收开销可观。

**优化**：在函数开头立即判断动能，低于阈值直接 `return true`：

```cpp
G4bool SphericalSystemSDinAir::ProcessHits(G4Step* aStep, G4TouchableHistory*) {
    if (!fSystemHitsinAirCollection) { ... }
    G4double kE = aStep->GetTrack()->GetKineticEnergy();
    if (kE < 30.0 * MeV) return true;   // 🔴 提前返回，避免建 Hit
    ...
}
```

**预期**：SD4 相关步处理加速 30-50%（取决于低能粒子占比）。

#### 🟡 问题 2：每步 2 次 `unit()` + 多次字符串比较

- `TrackerSD.cc:139` `G4double dir = position.unit().dot(momentum.unit());` —— 每步 2 次 `unit()`（每次 1 次 `1/sqrt`），且 `dir` 只在 SD1 分支用到。可延迟到需要时再算，或用 `position.dot(momentum)` 直接判断符号（点积 > 0 等价于 `dir > 0`，且不用归一化）。
- `TrackerSD.cc:142/180` `SDetectorName.substr(0, 10) == "Test02/SD2"` —— `substr` 每步创建临时 `std::string`。改为在构造函数里判断一次并缓存枚举（`fSDType`），热路径直接比较枚举。

#### 🟡 问题 3：SD1/SD2 的 Ntuple 填充

SD1（`TrackerSD.cc:180-232`）在 `kE >= 5 MeV` 时填 15 列 Ntuple（`BoundaryInfo`，ntuple id=2）；SD2 同样。这是 ROOT I/O，属于必要输出，但可以确认是否有粒子在 SD1 附近被反复记录（每步一次）。当前已有 `fStopAndKill`（`TrackerSD.cc:239`）在记录后杀停，所以每个粒子最多记录一次，OK。

---

### 3.5 SteppingAction / MyStackingAction 微优化

#### SteppingAction（`SteppingAction.cc:26-46`）

每步执行 `GetProcessName()` 并与 `"Transportation"` 字符串比较：

```cpp
G4String process = step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
if (process == "Transportation" && dE < 1.0e+11) { ... }
```

`GetProcessName()` 返回 `G4String`，`==` 是逐字符比较。改用枚举：

```cpp
if (step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessType()
        == G4ProcessType::fTransportation && dE < 1.0e+11) { ... }
```

每步省一次 strcmp。`-O3` 下差异变小，但属于零成本改进。

#### MyStackingAction（`MyStackingAction.cc:19-44`）

```cpp
if (track->GetDefinition()->GetPDGEncoding() == 12 ||
    track->GetDefinition()->GetPDGEncoding() == 14 || ... )  // 调 6 次
```

改为：

```cpp
G4int pdg = track->GetDefinition()->GetPDGEncoding();
G4int apdg = std::abs(pdg);
if (apdg == 12 || apdg == 14 || apdg == 16) return fUrgent;
```

`ClassifyNewTrack` 在簇射中每个新次级粒子都调用，减少函数调用次数有意义（尤其 `-O0` 下）。

> 注意：`ActionInitialization.cc:33` 把 stacking 阈值 `SetEnergyThreshold(15.0*MeV)`，覆盖了 `MyStackingAction` 构造里的 `5.0*MeV`（`MyStackingAction.cc:13`）。这是三处能量阈值不一致之一（见 `BUG_REPORT.md` 🟡-30），也会影响被杀掉的低能次级粒子数量 → 间接影响输运耗时。统一阈值是正确性 + 性能双重收益。

---

## 4. Python 侧模块分析

### 4.1 tracing.py（`mypylib/tracing.py`）

#### ✅ 已优化部分

- `main()`（`tracing.py:159-196`）已按磁刚度 `rigidity` 自适应选择积分器（`DOP853`/`RK45`/`LSODA`）并缩短高能粒子积分时间（`tracing.py:170-178`）。

#### 🟠 残余问题 1：事件函数重复计算完整球坐标

`solve_ivp` 的 `events=[rLow, rHigh, rDetector]`（`tracing.py:186`），三个事件函数都调用 `myxyz2polar`（`tracing.py:44-49`），计算 `r`、`theta`、`phi`，但**只需要 `r`**：

```python
def rLow(t, XV, qcharge, Ekg):
    x, y, z, vx, vy, vz = XV
    r, theta, phi = myxyz2polar(np.asarray([x, y, z]))  # 算了 theta/phi，浪费
    return r - 90 - 6371
```

`solve_ivp` 每个积分子步都会调用事件函数，`theta/phi` 的 `acos/atan2` 是纯浪费。改为：

```python
def _r_event(t, XV, qcharge, Ekg, r0):
    x, y, z = XV[0], XV[1], XV[2]
    return math.sqrt(x*x + y*y + z*z) - r0
```

事件函数是 Python 层 `solve_ivp` 的每步热点之一，这个改动对低能粒子（积分步多）尤其有效。**预计事件检测开销降低 50-70%。**

#### 🟡 残余问题 2：`complex_rLow`（如启用）成本极高

`tracing.py:97-131` 的 `complex_rLow` 在事件点调用 `myigrf14.igrf_wrapper` + `PitchAngle` + `BMirror`（完整 IGRF 场 + 镜像点判断）。当前主流程用的是 `rLow`（第 186 行，`complex_rLow` 被注释）。如果未来启用它，这是 Python 侧最贵的单点，需权衡物理必要性。

#### 🟡 残余问题 3：`myigrf14.rhs_particle` 是编译 .so

IGRF14 + 洛伦兹力计算在 `mypylib/myigrf14.cpython-310-*.so`（C 扩展）里，已经比纯 Python 快。这是 RHS 每步的计算主体，无法在 Python 层进一步优化（除非改 C 源码做向量化/预计算球谐系数）。

### 4.2 manager.py

#### ✅ 已优化部分

- `LoopBasic` 已用 `par_list`/`hit_list` 收集 + 最后一次性 `np.vstack`（`manager.py:154-176`），消除了 v1/v2 指出的 O(n²) `np.vstack` 循环。

#### 🟠 问题 1：每次 `LoopBasic` 重启 GEANT4 进程（与 3.2 呼应）

`LoopBasic`（`manager.py:130-251`）每次循环末尾 `GEANT4RuningFun(strmac)`（`manager.py:243`）都重启 GEANT4。这是**端到端最大的架构级开销**，详见 3.2 方案 C。

#### 🟠 问题 2（正确性→间接性能）：`ParticleSelect` 的 PID 阈值写错

`manager.py:93`：

```python
indx = np.where((PID==11)|(PID==-11)|(PID==2212)|(PID==-2212)|(PID>1e10)|(PID<-1e10))
```

原子核的 PDG 编码 ≈ `1e9`（如 α = `1000020040`），`PID > 1e10` 的阈值把**氦核等所有核子漏选**（正确值应为 `> 1e9`）。这属于 `BUG_REPORT.md` 🟠-19 的正确性 bug，但它也导致氦核通量被错误地排除 → 反推阶段粒子数偏少 → 结果错误。修复后粒子数会增加，性能上会**变慢**（但这是正确性所必需）。

#### 🟡 问题 3：`SaveRoot` 每次 `uproot.recreate`

`SaveRoot`（`manager.py:118-127`）每次循环用 `uproot.recreate(filename)` 重写文件。若每轮都写，I/O 开销累积。可改为追加模式或合并写。

---

## 5. 系统级瓶颈

### 5.1 磁场积分最小步长 `minStep = 1 cm`（`DetectorConstruction.cc:464`）

```cpp
G4double minStep = 1 * cm;
G4ChordFinder *chordFinder = new G4ChordFinder(threadField, minStep, stepper);
```

对 km 尺度的地磁偏转（曲率半径 ~10³-10⁴ km），`1 cm` 的最小步长过于保守，会导致多余的分段积分。可放宽到 `1 m` 甚至 `10 m`：

```cpp
G4double minStep = 1 * m;   // 对 km 尺度完全足够
```

**预期**：步数减少、场值查询次数同比减少，输运阶段再省 10-30%（需验证精度）。

### 5.2 物理列表 `FTFP_BERT_HP`（`test02.cc:64`）

`FTFP_BERT_HP` 含高精度中子（HP）模型。若中子不是主要关注对象，`FTFP_BERT` 可提速 15-30%。若需中子则保留。

### 5.3 生产截断（Production Cuts）

当前未显式设置 cut，GEANT4 默认值可能产生大量低能次级粒子。可在宏文件里显式增大：

```
/run/setCut 1 mm
/run/setCutForAGivenParticle e- 100 um
/run/setCutForAGivenParticle gamma 100 um
```

配合 stacking 阈值（`ActionInitialization.cc:33` 的 15 MeV），减少低能粒子输运量。

### 5.4 多线程

- GEANT4 32 线程（`/run/numberOfThreads 32`），Python `mps.Pool(32)`。若二者同时跑，需注意 CPU 过度订阅（总并发 > 物理核数）。但二者是串行（先 GEANT4 后 tracing），不会同时，问题不大。
- GEANT4 线程扩展性在 16-32 线程后递减，32 是合理上限。

---

## 6. 综合优化方案（按优先级）

| 优先级 | 优化项 | 模块 | 预期加速 | 难度 | 风险 |
|--------|--------|------|----------|------|------|
| **P0** | 🆕 编译优化级别 -O0 → -O3 -DNDEBUG | 全局 C++ | 端到端 **1.8-3x** | 极低 | 极低 |
| **P0** | 修复 🔴-3 崩溃（能量查找最近邻 + 按粒子种类选 E1/E2） | PrimaryGeneratorAction | 让 run_H/run_He 能跑 | 低 | 低 |
| **P0** | 关闭 `checkOverlaps` | DetectorConstruction | 单次构建 -20~40% | 极低 | 极低 |
| **P0** | 移除 `readAtmLayer` 的 G4cout | BinaryReader | 消除反复 I/O 抖动 | 极低 | 极低 |
| **P1** | GlogalDipoleField 用 rho 替换 atan2+sin/cos | MyMagneticField | 场计算 -30~50% | 低 | 低 |
| **P1** | SphericalSystemSDinAir early-return（kE<30MeV） | SD | SD4 步处理 -30~50% | 低 | 极低 |
| **P1** | `minStep` 1cm → 1m | DetectorConstruction | 输运 -10~30% | 低 | 中（需验证精度） |
| **P1** | 事件函数只算 r（不算 theta/phi） | tracing.py | 积分 -20~40% | 低 | 低 |
| **P1** | SteppingAction 用 ProcessType 枚举 | SteppingAction | 微 | 极低 | 极低 |
| **P1** | MyStackingAction 缓存 pdg + abs | MyStackingAction | 微 | 极低 | 极低 |
| **P1** | TrackerSD SDType 枚举 + 延迟算 dir | TrackerSD | SD 处理 -20% | 低 | 极低 |
| **P2** | 合并多次 Secondary run 为一次进程 | manager.py + 架构 | Secondary 阶段数倍 | 高 | 高 |
| **P2** | 统一能量阈值（stacking/SD/Air 三处） | 全局 | 正确性 + 输运量 | 低 | 低 |
| **P2** | 减少逻辑体/Material 数（纬度带缩减/密度参数化） | DetectorConstruction | 构建 + 导航 | 高 | 高 |
| **P2** | `FTFP_BERT_HP` → `FTFP_BERT`（若可） | test02.cc | 输运 -15~30% | 低 | 中 |
| **P3** | 删除死代码（fCachedCos* 成员、遗留 cone 函数） | PrimaryGeneratorAction | 代码清理 | 极低 | 极低 |
| **P3** | 显式 production cut | 宏文件 | 减少低能粒子 | 低 | 低 |
| **P3** | DirectionMaskCache::load 用 char* 扫描 | PrimaryGeneratorAction | 初始化加速 | 低 | 低 |

---

## 7. 预期性能提升汇总

### 端到端加速估算（叠加后）

```
                        当前耗时      优化后       加速比
                        ────────     ────────     ──────
GEANT4 输运+物理(-O0)    60-75%       15-25%      3-4x   ← P0 编译优化主导
GEANT4 初始化 Construct   5-15%        2-5%       2-3x   ← P0 关闭 overlap + P2 合并进程
Python tracing           15-25%       8-15%       1.5-2x  ← P1 事件函数
文件 I/O / ROOT          3-8%         2-5%       1.5-2x
                        ────────     ────────
总计                     100%         30-50%      2-3x（整体）
```

### 关键结论

1. **只做 P0 的第一项（-O0 → -O3）**，端到端就能提速约 **2 倍**，且零风险。这是所有优化里收益/成本比最高的一项，前两版都漏掉了。
2. 第二项最重要的架构级优化是 **Secondary 阶段的进程重启次数**（3.2 方案 C），但改动大、需单独评估。
3. 前两版反复强调的"PrimaryGeneratorAction 文件 I/O"（每事件读 CSV）**在当前代码里已经通过 `DirectionMaskCache` 解决**，不再是瓶颈；当前该模块真正的问题是 🔴-3 崩溃（正确性）而不是性能。
4. 磁场热路径是 `GlogalDipoleField`（非 v2 分析的 `DipoleField`），已部分优化，剩余主要是每步 `atan2/sin/cos` 可进一步省掉。

---

## 8. 实施优先级路线图

### 第一阶段：立即执行（0.5 天，零/极低风险）

```
1. 编译优化：cmake -DCMAKE_BUILD_TYPE=Release（改 CMakeLists 默认值）
   └─ 预计端到端 2x，所有模块受益
2. 修复 🔴-3 崩溃（能量最近邻 + 粒子种类选 E1/E2 + getRandomAllowed 越界保护）
   └─ 否则 run_H/run_He 无法运行，谈不上性能
3. 关闭 checkOverlaps
4. 移除 BinaryReader::readAtmLayer 的 G4cout
```

### 第二阶段：低风险热点优化（1-2 天）

```
5. GlogalDipoleField 用 rho 替换 atan2+sin/cos
6. SphericalSystemSDinAir::ProcessHits early-return
7. tracing.py 事件函数只算 r
8. minStep 1cm → 1m（验证精度）
9. SteppingAction / MyStackingAction / TrackerSD 的枚举化微优化
```

### 第三阶段：架构级优化（1 周，需评估）

```
10. 合并 Secondary 阶段进程重启（manager.py + GEANT4 侧支持）
11. 统一能量阈值
12. 减少逻辑体/Material 数（纬度带缩减 / 密度参数化）
13. 物理列表 FTFP_BERT_HP → FTFP_BERT（若物理允许）
```

---

## 9. 前两版结论订正表

| 前两版结论 | 问题 | 本版订正 |
|-----------|------|----------|
| 未提及编译优化级别 | 重大遗漏 | 🆕 当前 `-O0` Debug，端到端最大加速项 |
| v2 2.2 重点分析 `DipoleField` | 当前所有宏用 `globaldipole`，DipoleField 不被调用 | 热路径是 `GlogalDipoleField` |
| v2 "预计算 lat/lon trig 消除每事件 trig" | 当前 `GeneratorDirectionCSV1` 路径不涉及 lat/lon 旋转矩阵 | 该方案不适用；`fCachedCos*` 成员是死代码 |
| v1/v2 "PrimaryGeneratorAction 每事件读 CSV 是最严重瓶颈" | 已由 `DirectionMaskCache` 解决 | 不再成立；当前是 🔴-3 崩溃（正确性） |
| v2 "DetectorConstruction 是一次性初始化（🟡）" | 实际被 manager.py while 循环重启最多 200 次 | 初始化成本被放大两个数量级（🔴/🟠） |
| v1 "SelectDirection cone 文件每事件打开" | cone 路径是遗留代码，ctable 无对应文件 | 维持 v2 的订正（主流程走 bitset CSV） |
| v2 "np.vstack O(n²)" | 已改为 list 收集 | ✅ 已修复 |
| v2 "MyMagneticField 字符串比较" | 已改 enum switch | ✅ 已修复 |
| v2 "tracing.py 固定 RK45" | 已按 rigidity 自适应 | ✅ 已修复 |

---

> **总结（第三版）**：
> 1. **第一优先、几乎免费**：把 `-O0`（Debug）换成 `-O3 -DNDEBUG`（Release），预计端到端 **~2 倍**加速，前两版都没发现。做完这一条再谈其他优化才有意义。
> 2. **第二优先**：先修 `BUG_REPORT.md` 🔴-3 崩溃（否则 `run_H`/`run_He` 根本跑不起来）。
> 3. **第三优先**：`Construct()` 的重复初始化（关闭 overlap 检查 + 减少 Secondary 进程重启次数），这是被 while 循环放大 200 倍的隐藏成本。
> 4. 前两版部分结论已过时（磁场热路径、PrimaryGeneratorAction 文件 I/O、一次性初始化），本版均已按现有代码订正。
