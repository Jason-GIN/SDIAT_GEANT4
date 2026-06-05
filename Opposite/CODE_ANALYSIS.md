# SDIAT_GEANT4/Opposite — 宇宙线地磁场偏转与大气簇射模拟代码详解

> **作者**：GIN  
> **版本**：基于 Geant4 11.3.0 + ROOT  
> **日期**：2026-06-05 分析整理

---

## 目录

1. [项目概览](#1-项目概览)
2. [物理模型与模拟流程](#2-物理模型与模拟流程)
3. [目录结构与文件清单](#3-目录结构与文件清单)
4. [编译与构建系统](#4-编译与构建系统)
5. [核心模块详解](#5-核心模块详解)
   - [5.1 main() — 程序入口 (test02.cc)](#51-main--程序入口-test02cc)
   - [5.2 DetectorConstruction — 几何与探测器构建](#52-detectorconstruction--几何与探测器构建)
   - [5.3 PrimaryGeneratorAction — 初级粒子生成器](#53-primarygeneratoraction--初级粒子生成器)
   - [5.4 MyMagneticField — 地磁场模块](#54-mymagneticfield--地磁场模块)
   - [5.5 FieldMessenger — 磁场宏命令接口](#55-fieldmessenger--磁场宏命令接口)
   - [5.6 ActionInitialization — 用户动作注册](#56-actioninitialization--用户动作注册)
   - [5.7 RunAction / EventAction — 运行与事例管理](#57-runaction--eventaction--运行与事例管理)
   - [5.8 SteppingAction — 步长动作](#58-steppingaction--步长动作)
   - [5.9 TrackingAction — 径迹动作（父子关系追踪）](#59-trackingaction--径迹动作父子关系追踪)
   - [5.10 MyStackingAction — 粒子堆栈管理](#510-mystackingaction--粒子堆栈管理)
   - [5.11 TrackerSD — 灵敏探测器](#511-trackersd--灵敏探测器)
   - [5.12 SphericalSystemSDinAir — 大气球壳灵敏探测器](#512-sphericalsystemsdinair--大气球壳灵敏探测器)
   - [5.13 TrackerHit / SphericalHitinAir — Hit数据类](#513-trackerhit--sphericalhitinair--hit数据类)
   - [5.14 MyRun — 自定义Run类（粒子数据收集）](#514-myrun--自定义run类粒子数据收集)
   - [5.15 BinaryReader — 数据文件读取器](#515-binaryreader--数据文件读取器)
   - [5.16 DirectionData — 预计算方向数据库](#516-directiondata--预计算方向数据库)
6. [Python管理层详解](#6-python管理层详解)
   - [6.1 manager.py — 主控脚本](#61-managerpy--主控脚本)
   - [6.2 tracing.py — 地磁场径迹追踪](#62-tracingpy--地磁场径迹追踪)
   - [6.3 RotationModel.py — 坐标系转换库](#63-rotationmodelpy--坐标系转换库)
   - [6.4 SparticleCalculate.py — 次级粒子处理](#64-sparticlecalculatepy--次级粒子处理)
7. [宏文件系统](#7-宏文件系统)
8. [数据文件格式](#8-数据文件格式)
9. [运行方式](#9-运行方式)
   - [9.1 本地运行](#91-本地运行)
   - [9.2 集群批量运行 (HTCondor)](#92-集群批量运行-htcondor)
10. [工作流详解](#10-工作流详解)
11. [如何修改代码](#11-如何修改代码)
12. [已知问题与注意事项](#12-已知问题与注意事项)

---

## 1. 项目概览

这套代码使用 **GEANT4** 蒙特卡洛工具包模拟**宇宙线粒子从太空进入地球磁场、在大气中产生次级粒子簇射的完整过程**。核心物理链条为：

```
宇宙线初级粒子（质子/He核等）
    → 在地磁场中偏转（磁刚性问题）
    → 到达大气层顶部
    → 与大气分子相互作用（强子簇射）
    → 产生大量次级粒子（质子、中子、电子、缪子、中微子等）
    → 次级粒子继续在大气中输运并可能到达地面/探测器
```

代码的**独特设计**在于：
- 采用**两层递进式模拟**：第一层用Python追踪地磁场中的粒子偏转，第二层用GEANT4模拟大气簇射
- 几何模型为**地球球壳分层大气**，每层独立定义物质密度
- 支持Störmer锥限制条件，只允许能从特定方向到达地球的粒子

---

## 2. 物理模型与模拟流程

### 2.1 总体流程

```
[Step 0] Python manager.py 启动
         ↓
[Step 1] GEANT4 运行 → 初级粒子在大气层顶部生成
         ↓ (使用 run.mac 配置)
[Step 2] 粒子在大气中输运，与大气分子碰撞
         ↓
[Step 3] 到达SD1（大气顶部探测器）的粒子被记录
         ↓ (通过stderr管道传回Python)
[Step 4] Python接收粒子数据 → 过滤次级粒子（质子/电子/核子）
         ↓
[Step 5] 对每个次级粒子，用IGRF磁场模型反推其在地磁场中的轨迹
         ↓ (tracing.py，多进程并行)
[Step 6] 筛选能到达卫星轨道高度(400km)的粒子
         ↓
[Step 7] 将筛选后的粒子写为GEANT4宏文件 → 再次运行GEANT4
         ↓
[Step 8] 循环直到不再产生可到达卫星的粒子
```

### 2.2 关键物理参数

| 参数 | 值 | 说明 |
|------|-----|------|
| 地球半径 | 6371.2 km | 硬编码在多处 |
| 大气层数 | 208 层 | 从地面到约100km，采用不规则分层 |
| 轨道探测器高度 | 400 km | SD1位置 |
| 大气纬度划分 | 36 条带（每5°） | 从90°S到90°N |
| 磁场模型 | IGRF14 / 偶极近似 | 三种模式可选 |
| 物理列表 | FTFP_BERT_HP | 高精度强子物理 |
| 粒子截断能 | 15 MeV（堆栈）/ 30 MeV（记录） | 低于此能量的次级粒子被杀死 |
| Störmer锥数据 | ~13963个方向 | 预计算允许方向 |

---

## 3. 目录结构与文件清单

```
Opposite/
├── CMakeLists.txt              # CMake构建配置
├── test02.cc                   # 主程序入口 (main函数)
├── testdd.cc                   # 独立测试程序（方向生成算法验证）
├── manager.py                  # ★ Python主控脚本（双层模拟调度）
│
├── include/                    # C++ 头文件
│   ├── DetectorConstruction.hh # 几何构建器（大气分层、探测器）
│   ├── PrimaryGeneratorAction.hh # 初级粒子生成器
│   ├── MyMagneticField.hh      # 地磁场类
│   ├── FieldMessenger.hh       # 磁场UI命令接口
│   ├── ActionInitialization.hh # 动作初始化
│   ├── RunAction.hh            # Run动作（ROOT输出、管道通信）
│   ├── EventAction.hh          # Event动作
│   ├── SteppingAction.hh       # 步长动作（超时杀死）
│   ├── TrackingAction.hh       # 径迹动作（父子关系记录）
│   ├── MyStackingAction.hh     # 堆栈动作（能量截断）
│   ├── TrackerSD.hh            # 灵敏探测器（SD1/SD2）
│   ├── TrackerHit.hh           # Hit数据类
│   ├── SphericalSystemSDinAir.hh # 大气球壳灵敏探测器
│   ├── SphericalSystemSDinOrbit.hh # 轨道球壳灵敏探测器（预留）
│   ├── SphericalSystemHitinAir.hh # 大气Hit数据
│   ├── SphericalSystemHitinOrbit.hh # 轨道Hit数据
│   ├── MyRun.hh                # 自定义Run类（粒子数据收集）
│   ├── MySession.hh            # 自定义输出会话（stderr重定向）
│   ├── BinaryReader.hh         # CSV/ROOT文件读取器
│   └── DirectionData.hh        # 预计算方向数据库（13963方向）
│
├── src/                        # C++ 源文件
│   ├── DetectorConstruction.cc # 几何构建实现（~475行）
│   ├── PrimaryGeneratorAction.cc # 粒子生成器实现（~907行，核心模块）
│   ├── MyMagneticField.cc      # 磁场场值计算（~264行）
│   ├── FieldMessenger.cc       # 宏命令处理
│   ├── ActionInitialization.cc # 动作注册
│   ├── RunAction.cc            # Run管理
│   ├── EventAction.cc          # Event管理
│   ├── SteppingAction.cc       # 步长控制
│   ├── TrackingAction.cc       # 父子关系记录
│   ├── MyStackingAction.cc     # 能量截断
│   ├── TrackerSD.cc            # SD1/SD2探测逻辑
│   ├── TrackerHit.cc           # Hit实现
│   ├── SphericalSystemSDinAir.cc # 大气层灵敏探测
│   ├── SphericalSystemSDinOrbit.cc # 轨道层灵敏探测
│   ├── SphericalSystemHitinAir.cc # 大气Hit
│   ├── SphericalSystemHitinOrbit.cc # 轨道Hit
│   ├── BinaryReader.cc         # 文件读取实现
│   └── DirectionData.cc        # ★ 方向数据库（~492KB，13963个G4ThreeVector）
│
├── mypylib/                    # Python辅助库
│   ├── __init__.py             # 包初始化
│   ├── tracing.py              # ★ 地磁场中粒子径迹数值求解（核心Python模块）
│   ├── RotationModel.py        # 坐标系转换函数库
│   └── igrf14.cpython-310...so # IGRF14地磁场编译模块
│
├── run.mac                     # 标准GEANT4宏文件（初级粒子模式）
├── test2.mac                   # GPS多源模式宏文件（次级粒子模式）
├── init_vis.mac                # 交互式可视化初始化
├── vis.mac                     # 可视化详细设置
├── Spectrum.dat                # 初级粒子能谱权重数据
├── data.dat                    # 其他数据（大气密度等）
│
├── run_command/                # 集群任务生成工具
│   ├── shell-generate.cc       # 生成批量.sh脚本的C++程序
│   ├── shell-generate          # 编译后的可执行文件
│   ├── test.sh                 # 单任务测试脚本
│   ├── H/                      # 氢(H)批量脚本目录
│   └── He/                     # 氦(He)批量脚本目录
│
├── run/                        # 批量运行宏文件生成
│   ├── run-mac-gen.cc          # 生成GEANT4宏文件的C++程序
│   ├── run_H/                  # 氢运行宏文件
│   └── run_He/                 # 氦运行宏文件
│
├── result/                     # 输出结果（ROOT文件）
│   ├── smac/                   # 次级宏文件
│   ├── TraceData/              # 轨迹数据CSV
│   ├── seconddata*_*.root      # 次级粒子数据
│   └── HitChargePdata*_*.root  # Hit电荷数据
│
├── build/                      # CMake构建输出目录
├── JobWork_H.jdf               # HTCondor任务描述文件（氢）
├── JobWork_He.jdf              # HTCondor任务描述文件（氦）
├── JobWork_test.jdf            # HTCondor测试任务
├── ReadAndTest.py              # 数据处理/测试脚本
└── SparticleCalculate.py       # 次级粒子统计与计算脚本
```

---

## 4. 编译与构建系统

### CMakeLists.txt 解析

```cmake
cmake_minimum_required(VERSION 3.16...3.21)
project(test02)

# 查找依赖：Geant4 (11.3.0, 带UI/Vis) + ROOT
find_package(Geant4 11.3.0 REQUIRED ui_all vis_all)
find_package(ROOT REQUIRED)

# 可执行文件
add_executable(test02 test02.cc ${sources} ${headers})
target_link_libraries(test02 ${Geant4_LIBRARIES} ${ROOT_LIBRARIES})

# Debug模式编译
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
```

**关键依赖**：
- Geant4 11.3.0（带UI和可视化）
- ROOT（用于读取预计算方向数据、输出Ntuple）
- 编译器：GCC/G++（支持C++17）

**构建命令**：
```bash
cd build/
cmake ..
make -j$(nproc)
```

构建后自动将宏文件、数据文件复制到 build 目录。

---

## 5. 核心模块详解

### 5.1 main() — 程序入口 (test02.cc)

**文件**：`test02.cc`  
**功能**：GEANT4模拟的主入口，初始化所有核心组件

```cpp
int main(int argc, char** argv) {
    // 1. 随机数引擎初始化（RanecuEngine + 时间种子）
    CLHEP::HepRandom::setTheEngine(new CLHEP::RanecuEngine());
    G4long seed = time(NULL);
    CLHEP::HepRandom::setTheSeed(seed);

    // 2. 创建UI会话（无参数=交互模式，有参数=批处理模式）
    G4UIExecutive* ui = nullptr;
    if (argc == 1) { ui = new G4UIExecutive(argc, argv); }

    // 3. 创建Run Manager
    auto* runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

    // 4. 初始化磁场 → 探测器几何 → 物理列表 → 用户动作
    auto gMasterField = new MyMagneticField();
    auto gFieldMessenger = std::make_unique<FieldMessenger>(gMasterField);
    runManager->SetUserInitialization(new DetectorConstruction(gMasterField));
    runManager->SetUserInitialization(new FTFP_BERT_HP);  // 物理列表
    runManager->SetUserInitialization(new ActionInitialization());

    // 5. 可视化初始化
    G4VisManager* visManager = new G4VisExecutive;
    visManager->Initialize();

    // 6. 执行宏文件或启动交互会话
    if (!ui) {
        // 批处理模式：执行命令行参数的宏文件
        UImanager->ApplyCommand("/control/execute " + fileName);
    } else {
        // 交互模式
        UImanager->ApplyCommand("/control/execute init_vis.mac");
        ui->SessionStart();
    }
}
```

**两种运行模式**：
| 模式 | 触发条件 | 用途 |
|------|----------|------|
| 批处理 | `./test02 run.mac` | 被Python manager.py调用，传入宏文件参数 |
| 交互式 | `./test02` | 可视化调试，手动输入命令 |

### 5.2 DetectorConstruction — 几何与探测器构建

**文件**：`include/DetectorConstruction.hh`, `src/DetectorConstruction.cc`（~475行）  
**命名空间**：`Test02`  
**功能**：构建地球+大气层的球壳分层几何模型

#### 几何层次结构

```
World (Galactic真空, 1.2×Envelope)
├── Envelope (Galactic真空)
│   ├── SD1 (轨道高度探测器，球壳 r≈6771km，厚10cm)
│   │   └── 记录到达该高度的粒子，终止其追踪
│   ├── SD2 (地面探测器，球壳 r≈6371km，厚10cm)
│   │   └── 记录到达地面的粒子，终止其追踪
│   ├── [大气分层 0..207] × [纬度带 0..35] = 7488个逻辑体
│   │   ├── LayerSD3_<cpnu>_nlat_<nlat>  (大气层)
│   │   └── LayerSD4_<cpnu>_nlat_<nlat>  (每1km一层的SD探测器)
│   └── [轨道层 0..99] × [纬度带 0..35] (预留，当前不激活)
```

#### 大气分层模型

- **垂直方向**：208层，从地面到约100km高度
  - 分层高度存储在 `AtmLayer[209]` 数组中（单位：米）
  - 低层密度大、分层密；高层密度小、分层疏
  - 例如：第0层0m→40.9m，第207层52.4km→100km

- **水平方向**：36条纬度带，每5°一条
  - 从 90°S 到 90°N（nlat=0 → -90°, nlat=35 → +85°）

- **每层物质**：
  - 从 `../ctable/layers_lat_<lat>.csv` 读取
  - 包含：N₂ 比例、O₂ 比例、Ar 比例、密度(mg/cm³)
  - 每个纬度带不同（反映大气环流差异）

#### 关键方法

```cpp
// 构造函数：初始化命令定义、保存磁场指针
DetectorConstruction(MyMagneticField *masterField);

// 构建几何体
G4VPhysicalVolume* Construct() override;

// 设置灵敏探测器和磁场
void ConstructSDandField() override;
```

**注**：`ConstructSDandField()` 中包含多线程安全处理——工作线程克隆主磁场对象以避免数据竞争。

---

### 5.3 PrimaryGeneratorAction — 初级粒子生成器

**文件**：`include/PrimaryGeneratorAction.hh`, `src/PrimaryGeneratorAction.cc`（~907行，最核心模块之一）  
**功能**：生成入射宇宙线粒子的位置、方向和能量

#### 两种生成模式

| 模式 | 激活命令 | 用途 |
|------|----------|------|
| **Primary** | `/generator/select Primary` | 从能谱和Störmer锥采样生成初级宇宙线粒子 |
| **Secondary** | `/generator/select Secondary` | 从CSV文件读取预计算的次级粒子列表，逐个发射 |

#### Primary模式下的粒子生成流程

```
1. 根据粒子种类和能量，确定 targetrow（对应预计算方向表中的行号）
   - proton: 使用E1[43]数组，能量范围 250 MeV → 90.27 GeV
   - He等:   使用E2[43]数组，能量范围 270 MeV → 178.68 GeV

2. 在地球表面均匀采样生成位置
   - 在Envelope体积的球冠区域（10°弧长）内随机采样
   - 位置转换到GTOD坐标

3. 调用方向生成算法：
   ├── Step A: 随机生成天顶坐标下的方向(θ, φ)
   ├── Step B: 用 SelectDirection() 判断是否在Störmer允许锥内
   │   ├── 读取 ../ctable/all_<lon>_<lat>.csv 预计算数据
   │   └── 二分查找该φ对应的允许θ范围
   ├── Step C: 若不在锥内，重新生成（do-while循环）
   └── Step D: 将天顶坐标方向转换回GTOD坐标
```

#### 坐标转换链（四个关键函数）

代码中包含大量坐标系转换，核心概念：

- **Zenith（天顶坐标）**：以地面某点(lat, lon)为原点，z轴向天顶
- **GTOD（地心地固坐标）**：以地心为原点的地球固定坐标系

转换矩阵是基于余纬度(colatitude)=90°-lat的标准旋转：

```cpp
// 天顶坐标 → GTOD
G4ThreeVector VectorZenithinGTOD1(xyz, colat, lon);

// GTOD → 天顶坐标
G4ThreeVector VectorGTODinZenith1(xyz, colat, lon);

// 完整的三步转换：
// 1. 位置从Zenith → GTOD
// 2. 方向从Zenith → GTOD（用GTOD中的位置做参考）
// 3. 结果方向从GTOD → 目标坐标系的Zenith
```

#### Störmer锥判断算法

```cpp
// SelectDirection(): 二分查找phi对应的theta阈值
// 返回: 0=方向在允许锥内(可到达), 1=不在(需重新生成)
// 返回: -1=数据格式异常,需启用备选算法SelectDirection2()

// SelectDirection2(): 复杂几何判断
// 将phi±1°内的theta值按连续性分组，用奇偶性判断
```

#### 能量采样

```cpp
// GeneratorEnergyFromSpectrum(): 从Spectrum.dat读取能谱权重
// 使用 G4RandGeneral 按权重采样能量bin
```

#### Secondary模式

直接从CSV文件逐行读取粒子参数（位置、方向、能量、PDG码），每个event发射一个粒子。用于Python层筛选后的次级粒子再注入。

---

### 5.4 MyMagneticField — 地磁场模块

**文件**：`include/MyMagneticField.hh`, `src/MyMagneticField.cc`（~264行）  
**功能**：实现三种地磁场模型

#### 三种磁场类型

| 类型 | 命令 | 描述 |
|------|------|------|
| `uniform` | `/field/SetType uniform` | 均匀磁场（测试用） |
| `dipole` | `/field/SetType dipole` | 倾斜偶极子场，在地理位置(lat,lon)的天顶坐标下给出 |
| `globaldipole` | `/field/SetType globaldipole` | 倾斜偶极子场，在GTOD坐标下给出（默认模式） |

#### 偶极场实现（IGRF一阶近似）

使用IGRF的一阶高斯系数（单位：nT）：
```
g₀¹ = -29404.8 nT
g₁¹ = -1450.9 nT
h₁¹ =  4652.5 nT
```

球坐标下的偶极场公式（在GTOD坐标中）：
```
Br = 2(a³/r³)[g₀¹·cos(θ) + (g₁¹·cos(φ) + h₁¹·sin(φ))·sin(θ)]
Bθ = -(a³/r³)[-g₀¹·sin(θ) + (g₁¹·cos(φ) + h₁¹·sin(φ))·cos(θ)]
Bφ = -(a³/r³)[-g₁¹·sin(φ) + h₁¹·cos(φ)]
```

- `DipoleField()`：在天顶坐标下计算（粒子位置先转到GTOD，计算B后再转回）
- `GlogalDipoleField()`：直接在GTOD下计算（粒子位置就在GTOD中）

#### 关键方法

```cpp
// 每个步长被GEANT4调用，计算Track位置的磁场值
void GetFieldValue(const G4double Track[7], G4double B[3]) const;

// 多线程支持：每个工作线程克隆独立的磁场对象
MyMagneticField* Clone() const override;

// 启用/禁用、设置类型和参数
void EnableField(bool enable);
void SetFieldType(G4String type);
void SetDipoleFieldParameters(G4ThreeVector geo);  // {r, lat, lon}
void SetUniformFieldParameters(G4ThreeVector stre); // {Bx, By, Bz}
```

---

### 5.5 FieldMessenger — 磁场宏命令接口

**文件**：`include/FieldMessenger.hh`, `src/FieldMessenger.cc`  
**功能**：将宏文件命令映射到MyMagneticField的方法

**支持的命令**：
| 宏命令 | 功能 |
|--------|------|
| `/field/enable true/false` | 启用/禁用磁场 |
| `/field/SetType dipole\|globaldipole\|uniform` | 选择磁场类型 |
| `/field/Geolocation r lat lon degree` | 设置地理位置（偶极场模式） |
| `/field/UniStrength Bx By Bz gauss` | 设置均匀场强度 |

---

### 5.6 ActionInitialization — 用户动作注册

**文件**：`include/ActionInitialization.hh`, `src/ActionInitialization.cc`  
**功能**：注册所有GEANT4用户动作类

```cpp
void Build() const {
    // 主线程和工作线程都调用
    SetUserAction(new PrimaryGeneratorAction(...));
    
    // 堆栈动作：杀死 <15MeV 的次级粒子（保留原初粒子和中微子）
    MyStackingAction* stacking = new MyStackingAction();
    stacking->SetEnergyThreshold(15.0 * MeV);
    SetUserAction(stacking);
    
    // Run/Event/Stepping/Tracking 动作
    SetUserAction(new RunAction);
    SetUserAction(new EventAction(runAction));
    SetUserAction(new SteppingAction);
    SetUserAction(new TrackingAction);
}

void BuildForMaster() const {
    // 仅主线程调用
    SetUserAction(new RunAction);
}
```

---

### 5.7 RunAction / EventAction — 运行与事例管理

**文件**：`include/RunAction.hh`, `src/RunAction.cc`; `include/EventAction.hh`, `src/EventAction.cc`

#### RunAction — 运行级管理

**核心功能**：

1. **ROOT Ntuple输出**：创建三个Ntuple：
   - `EventInfo` (Ntuple 0)：每个事件的ID、原初粒子类型、原初能量
   - `DetectionInfo` (Ntuple 1)：被SD记录的粒子详细信息（16列）
   - `BoundaryInfo` (Ntuple 2)：边界穿越信息（16列）

2. **管道通信（Python集成）**：
   ```cpp
   void EndOfRunAction(const G4Run* Run) {
       // 非工作线程：将收集的粒子数据通过stderr管道发送
       // 格式: [4字节count][count×ParticleData结构体]
       const auto& particles = run->GetAllParticles();
       uint32_t count = particles.size();
       std::cerr.write((char*)&count, sizeof(count));
       std::cerr.write((char*)particles.data(), count * sizeof(ParticleData));
   }
   ```
   **这是GEANT4与Python通信的关键机制**——利用stderr管道传输二进制粒子数据。

3. **自定义Run类**：使用 `MyRun` 替代默认 `G4Run`，支持收集 `ParticleData` 向量

#### EventAction — 事件级管理

- 每个事件开始时：记录Primary粒子的PID和能量
- 每个事件结束时：填充EventInfo Ntuple、清理父粒子信息映射

---

### 5.8 SteppingAction — 步长动作

**文件**：`include/SteppingAction.hh`, `src/SteppingAction.cc`  
**功能**：监控长时间飞行的粒子并终止

```cpp
void SteppingAction::UserSteppingAction(const G4Step* step) {
    // 检测长时间Transportation步骤
    G4double flytime = step->GetPostStepPoint()->GetProperTime();
    if (process == "Transportation" && dE < 1.0e11) {
        n++;
        // 飞行时间 > 1e13 ns 且连续100步无能量变化 → 杀死
        if (flytime/ns > 1e+13 && n > 100) {
            step->GetTrack()->SetTrackStatus(fStopAndKill);
        }
    } else {
        n = 0;  // 有能量变化时重置计数器
    }
}
```

这个机制防止粒子在地磁场中无限回旋而永远不停止模拟。

---

### 5.9 TrackingAction — 径迹动作（父子关系追踪）

**文件**：`include/TrackingAction.hh`, `src/TrackingAction.cc`  
**功能**：记录每个track的父粒子信息，建立粒子族谱

```cpp
void PreUserTrackingAction(const G4Track* track) {
    // 将当前track的信息（名称+PDG编码）存入map
    // key = trackID, value = ParentInfo{name, pdg}
    // 这样TrackerSD在记录hit时可以查询父粒子信息
    (*localMap)[trackID] = {def->GetParticleName(), def->GetPDGEncoding()};
}
```

使用 `G4ThreadLocal` 保证多线程安全——每个线程有独立的map。

---

### 5.10 MyStackingAction — 粒子堆栈管理

**文件**：`include/MyStackingAction.hh`, `src/MyStackingAction.cc`  
**功能**：控制哪些次级粒子值得追踪

```cpp
G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* track) {
    G4double energy = track->GetKineticEnergy();
    
    if (energy < fEnergyThreshold) {  // 默认15 MeV
        if (track->GetParentID() == 0) {
            return fUrgent;  // 保留原初粒子
        } else {
            // 保留中微子（它们几乎不与物质作用）
            if (/* PDG == 12,14,16,-12,-14,-16 */)
                return fUrgent;
            else
                return fKill;  // 杀死低能次级粒子
        }
    }
    return fUrgent;
}
```

这个机制大幅减少需要追踪的次级粒子数量，加速模拟。

---

### 5.11 TrackerSD — 灵敏探测器

**文件**：`include/TrackerSD.hh`, `src/TrackerSD.cc`（~254行）  
**功能**：SD1和SD2的灵敏探测逻辑

#### SD1（轨道高度探测器，~400km）

当带电粒子（能量 > 30MeV）穿过SD1时：
1. 记录粒子信息到 `BoundaryInfo` Ntuple
2. 排除中微子（PDG 12,14,16及其反粒子）
3. 将粒子数据存入 `ParticleData` 结构体（通过 `SetParticleData()`）
4. **终止粒子追踪**（`fStopAndKill`）

> **重要**：SD1收集的 `ParticleData` 最终通过 `MyRun` → `RunAction::EndOfRunAction()` → stderr管道传回Python。

#### SD2（地面探测器）

当带电粒子到达SD2时：
1. 记录到 `DetectionInfo` Ntuple
2. **终止粒子追踪**

#### ParticleData结构体
```cpp
struct ParticleData {
    double x, y, z;       // 位置 (km)
    double dx, dy, dz;    // 动量方向（未归一化）
    double energy;        // 动能 (MeV)
    double PID;           // PDG粒子编码
    double charge;        // 电荷 (e)
    double t;             // 到达时间 (ns)
    double m;             // 质量
};
```

---

### 5.12 SphericalSystemSDinAir — 大气球壳灵敏探测器

**文件**：`include/SphericalSystemSDinAir.hh`, `src/SphericalSystemSDinAir.cc`  
**功能**：大气层内每1km的球壳层灵敏探测器

与TrackerSD功能相似但用于大气内部的球壳层探测器，记录每个穿过大气层的粒子的信息到 `DetectionInfo` Ntuple。

**注意**：当前代码中大气层SD使用的是 `SphericalSystemSDinAir`，而轨道层SD（`SphericalSystemSDinOrbit`）的装配代码被注释掉了（见 `DetectorConstruction::ConstructSDandField()` 末尾）。

---

### 5.13 TrackerHit / SphericalHitinAir — Hit数据类

**文件**：`include/TrackerHit.hh`, `include/SphericalSystemHitinAir.hh`  
**功能**：定义灵敏探测器中记录的单次Hit数据结构

**TrackerHit关键字段**：
| 字段 | 类型 | 说明 |
|------|------|------|
| fTrackID | int | 径迹ID |
| fParentID | int | 父粒子ID |
| fPID | int | PDG粒子编码 |
| fpPID | int | 父粒子PDG编码 |
| fEdep | double | 能量沉积 |
| fkE | double | 动能 |
| fCharge | double | 电荷 |
| fPos | G4ThreeVector | 位置 |
| fMom | G4ThreeVector | 动量 |
| fTime | double | 全局时间 |
| fParticalName | G4String | 粒子名称 |
| fParentParticalName | G4String | 父粒子名称 |
| fProcessName | G4String | 产生过程名称 |

`SphericalHitinAir` 在此基础上多了一个 `fLayerID` 字段用于标识所在大气层。

---

### 5.14 MyRun — 自定义Run类（粒子数据收集）

**文件**：`include/MyRun.hh`  
**功能**：扩展G4Run，增加粒子数据收集能力

```cpp
class MyRun : public G4Run {
public:
    void AddParticles(const std::vector<ParticleData>& particles) {
        fParticles.insert(fParticles.end(), particles.begin(), particles.end());
    }
    
    // 多线程合并：将工作线程的粒子数据合并到主线程
    void Merge(const G4Run* aRun) override {
        const MyRun* localRun = static_cast<const MyRun*>(aRun);
        fParticles.insert(...);
        G4Run::Merge(aRun);
    }
    
    const std::vector<ParticleData>& GetAllParticles() const { return fParticles; }
private:
    std::vector<ParticleData> fParticles;
};
```

---

### 5.15 BinaryReader — 数据文件读取器

**文件**：`include/BinaryReader.hh`, `src/BinaryReader.cc`  
**功能**：读取三种类型的数据文件

1. **CSV全文件读取** (`readCSVinAll`)：逐行解析CSV，将"1.0"标记的列存入bitset
2. **CSV指定行读取** (`readCSVSpecificRow`)：只读取CSV的特定行到bitset
3. **ROOT文件读取** (`readROOTSpecificBranch`)：读取ROOT TTree的特定分支
4. **大气层数据读取** (`readAtmLayer`)：读取 `layers_lat_<lat>.csv` 文件（每行4列：N₂比例, O₂比例, Ar比例, 密度）

**Störmer锥数据的bitset格式**：
- 每行13963个bit，每个bit对应一个预计算方向
- bit=1表示该方向在允许锥内（粒子可从此方向到达）
- CSV文件的一行对应一种能量/位置组合

---

### 5.16 DirectionData — 预计算方向数据库

**文件**：`include/DirectionData.hh`, `src/DirectionData.cc`（~492KB）  
**功能**：存储13963个预计算方向向量的静态数组

```cpp
class DirectionData {
public:
    static constexpr size_t NUM_DIRECTIONS = 13963;
    static G4ThreeVector getDirection(size_t index);
    static G4ThreeVector getRandomDirection();
private:
    static const G4ThreeVector directions[NUM_DIRECTIONS];
};
```

这13963个方向是预计算的均匀分布在天空的方向向量，与Störmer锥数据（13963个bit）一一对应。

---

## 6. Python管理层详解

### 6.1 manager.py — 主控脚本

**文件**：`manager.py`（~303行）  
**功能**：两层递进模拟的总调度器

#### 核心工作流

```
main(runmac, Enum)
│
├── Step 1: GEANT4RuningFun(runmac)
│   └── subprocess.run("./test02", runmac, stderr=PIPE)
│       接收stderr中的二进制粒子数据
│       解析为numpy structured array
│
├── Step 2: ParticleSelect(particles)
│   └── 筛选 PID==11(e⁻), -11(e⁺), 2212(p), -2212(anti-p), >1e10(核子) 的粒子
│
├── Step 3: 迭代循环 (while pnum > 0)
│   └── LoopBasic(tparticlelist, Enum, count)
│       │
│       ├── 多进程并行调用 tracing.main()
│       │   └── 对每个粒子，用地磁场模型反推轨迹
│       │   └── 返回: (task_id, 轨迹结果, Hit数据)
│       │
│       ├── 收集结果：
│       │   ├── 未到达400km的粒子 → Hitlist（记录其击中位置）
│       │   └── 到达400km的粒子 → Parlist（记录其到达时的状态）
│       │
│       ├── 将Parlist写为CSV → 生成GEANT4宏文件
│       │
│       └── 再次运行GEANT4 → 继续追踪这些粒子的后续行为
│
└── 循环直到不产生新的可到达卫星的粒子
```

#### 关键函数

**`Gmacfile(filename, RootPath, datafilpath, Pnum)`**：
生成GEANT4宏文件（Secondary模式），设置：
- 32线程
- 全球偶极磁场
- Secondary生成器模式
- 从CSV读取粒子列表

**`GEANT4RuningFun(macro_file)`**：
- 启动 `./test02` 子进程
- 从stderr读取：前4字节=粒子数量，后续=粒子数据二进制流
- 按 `ParticleData` 结构体对齐解析为numpy数组

**`ParticleSelect(particles)`**：
过滤感兴趣的粒子类型（电子、质子、重核）

**`LoopBasic(particlelist, Enum, n)`**：
- 对粒子列表中的每个粒子，创建 `(pos, mom, [mass, charge], ...)` 参数
- 使用 `multiprocessing.Pool`（最大32进程）并行调用 `tracing.main()`
- 收集追踪结果并分类处理

---

### 6.2 tracing.py — 地磁场径迹追踪

**文件**：`mypylib/tracing.py`（~124行）  
**功能**：用数值积分求解带电粒子在地磁场中的运动方程

#### 核心求解器

```python
def XVDinkm(t, XV, qcharge, Ekg):
    # 调用IGRF14磁场模型的右侧函数
    # 计算洛伦兹力的加速度:
    #   a = q*v×B / Ekg (相对论修正)
    return igrf14.rhs_particle(t, XV, qcharge, Ekg, 2025)
```

#### 三个事件检测器（控制积分何时停止）

| 事件 | 条件 | 行为 |
|------|------|------|
| `rLow` | r < 100+6371 km | **终止积分**（撞入稠密大气） |
| `rHigh` | r > 100000 km | **终止积分**（逃离地磁场） |
| `rDetector` | r < 400+6371 km | **记录但不终止**（到达卫星轨道高度） |

#### 主函数

```python
def main(Xinit, monento, otherpar, i, n, thread):
    # Xinit: 初始位置 [x, y, z] (km)
    # monento: 动量 [px, py, pz]
    # otherpar: [静质量(MeV), 电荷(e)]
    
    # 1. 相对论能量计算
    # 2. 速度 = c * p/E
    # 3. scipy.integrate.solve_ivp 数值积分 (RK45方法)
    # 4. 保存轨迹到CSV
    # 5. 返回事件结果
```

**物理意义**：从GEANT4在大气中记录的次级粒子位置反向追踪，看在磁场中回溯是否能到达卫星轨道高度（400km）。这实际上是在模拟"哪些次级粒子可以向上逃逸出大气层到达卫星轨道"。

---

### 6.3 RotationModel.py — 坐标系转换库

**文件**：`mypylib/RotationModel.py`（~95行）  
**功能**：提供与C++侧对应的坐标转换函数（Python版本）

核心函数：
- `xyz2rthetaphi()` / `rthetaphi2xyz()`：直角坐标 ↔ 球坐标
- `VectorGTODinZenith()` / `VectorZenithinGTOD()`：天顶坐标 ↔ GTOD
- `drirenomal()`：完整方向归一化转换

---

### 6.4 SparticleCalculate.py — 次级粒子处理

**文件**：`SparticleCalculate.py`（~146行）  
**功能**：处理和统计GEANT4输出的次级粒子

主要操作：
1. 读取 `BoundaryInfo` Ntuple（32个线程文件）
2. 提取到达SD1的粒子数据（位置、动量、能量、PID）
3. 筛选中子（PID==2112）
4. 计算中子平均能量、速度、15分钟内的飞行距离
5. 生成GPS多源模式宏文件 `test2.mac`

---

## 7. 宏文件系统

### run.mac（标准批处理宏）

```
/run/numberOfThreads 32          # 32线程并行
/field/enable true               # 启用磁场
/field/SetType globaldipole      # 全球偶极场
/field/Geolocation 1 25 -160 degree  # 位置: lat=25°, lon=-160°
/run/initialize                  # 初始化几何和物理
/myrun/setOutputFileName protondata_0.root
/Geo/lat 25 deg                  # 设置地理位置纬度
/Geo/lon -160 deg                # 设置地理位置经度
/generator/select Primary        # 使用Primary生成器
/gun/particle proton             # 粒子种类：质子
/gun/energy 9.34 GeV             # 固定能量
/run/beamOn 1000                 # 模拟1000个事例
```

### test2.mac（GPS多源模式宏）

由 `SparticleCalculate.py` 自动生成，使用GEANT4的GPS（General Particle Source）多源模式，每个粒子有自己的能量、位置和方向。最后设置 `/gps/source/multiplevertex true` 和 `/run/beamOn 1`。

### init_vis.mac / vis.mac（可视化宏）

用于交互式可视化调试，设置OpenGL视图、轨迹显示等。

---

## 8. 数据文件格式

### Spectrum.dat
```
<能量(GeV)> <权重>
```
用于 `GeneratorEnergyFromSpectrum()` 的能谱采样。

### ../ctable/all_<lon>_<lat>.csv
预计算的Störmer锥允许方向表：
- 54行（对应不同能量/粒子种类）
- 每行13963个bit（0或1），对应DirectionData的13963个预定义方向
- bit=1表示该方向的粒子能到达地球

### ../ctable/layers_lat_<lat>.csv
大气分层数据：
```
<N₂质量比例>, <O₂质量比例>, <Ar质量比例>, <密度(mg/cm³)>
```
- 每个纬度带独立文件
- 208行（对应208个大气层）

### 输出ROOT文件

| Ntuple名 | 内容 |
|----------|------|
| `EventInfo` | EventID, PrimaryType, PrimaryEnergy |
| `DetectionInfo` | SDnum, TrackID, Energy, Pos(xyz), Dir(xyz), Charge, Time, PID, ParentId, pPID, Detector, Process |
| `BoundaryInfo` | 同上格式，用于边界穿越 |
| `HitData` | 额外的Hit电荷数据 |

---

## 9. 运行方式

### 9.1 本地运行

#### 单次模拟
```bash
cd build/
./test02 run.mac
```

#### 完整双层模拟
```bash
cd build/
python3 manager.py ../run.mac <Enum>
```
- `Enum`：粒子种类标识（如"H", "He"等）
- 这个命令会自动循环调用GEANT4和Python追踪

#### 交互式可视化
```bash
cd build/
./test02
# 然后在GEANT4交互界面中手动输入命令
```

### 9.2 集群批量运行 (HTCondor)

项目包含HTCondor集群提交系统：

1. **生成批量脚本**：
   ```bash
   cd run_command/
   g++ shell-generate.cc -o shell-generate
   ./shell-generate
   # 生成 H/H_1.sh ~ H_24.sh, He/He_1.sh ~ He_24.sh
   ```

2. **提交任务**：
   ```bash
   condor_submit JobWork_H.jdf   # 提交氢的批量任务
   condor_submit JobWork_He.jdf  # 提交氦的批量任务
   ```

每个脚本设置GEANT4环境变量 → 切换到build目录 → 执行 `python3 manager.py`

---

## 10. 工作流详解

### 完整的一次两层模拟流程

```
┌──────────────────────────────────────────────────────────────────┐
│                         manager.py main()                         │
└──────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────────┐
│ 阶段1: GEANT4 Primary模式                                         │
│ ./test02 run.mac                                                 │
│                                                                   │
│ [宇宙线粒子在大气顶部生成]                                        │
│       ↓                                                          │
│ [地磁场偏转 → 大气簇射 → 产生次级粒子]                            │
│       ↓                                                          │
│ [SD1(400km)收集逃离大气的粒子] → stderr管道输出                   │
│       ↓                                                          │
│ [SD2(地面)收集到达地面的粒子] → ROOT文件输出                       │
└──────────────────────────────────────────────────────────────────┘
                              │
                              ▼ stderr二进制数据被Python解析
                              │ numpy structured array
                              │
┌──────────────────────────────────────────────────────────────────┐
│ 阶段2: Python筛选与反推                                           │
│ ParticleSelect() → 选出质子/电子/核子                             │
│                                                                   │
│ For each selected particle:                                       │
│   tracing.main(pos, mom, [mass, charge])                          │
│     → scipy.integrate.solve_ivp: 从大气位置反推轨迹               │
│     → 使用IGRF14磁场 + 洛伦兹力方程                               │
│     → rDetector事件: 检测是否到达400km轨道                        │
│       ↓                                                          │
│ [到达400km的粒子] → 记录其到达时的状态                            │
│ [未到达的粒子]   → 记录其最后击中位置(Hitlist)                    │
└──────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────────┐
│ 阶段3: 生成Secondary宏 → 再次运行GEANT4                            │
│                                                                   │
│ Parlist (到达400km的粒子) → CSV文件                                │
│   → Gmacfile() 生成GEANT4宏文件                                    │
│   → ./test02 <mac文件>                                             │
│     → 从CSV加载粒子列表作为GPS多源                                 │
│     → 每个粒子继续在大气中输运                                     │
│     → SD1再收集 → stderr输出                                       │
│       ↓                                                          │
│ [循环回到阶段2，直到pnum==0]                                       │
└──────────────────────────────────────────────────────────────────┘
```

---

## 11. 如何修改代码

### 11.1 修改地理经纬度

在宏文件中修改（或在代码中修改默认值）：
```
/Geo/lat <纬度> deg
/Geo/lon <经度> deg
/field/Geolocation 1 <纬度> <经度> degree
```
**注意**：需要确保 `../ctable/all_<lon>_<lat>.csv` 文件已预计算好。

### 11.2 修改模拟粒子种类和能量

**Primary模式（能谱采样）**：修改 `run.mac`
```
/gun/particle proton    # 改为 "alpha", "gamma" 等
/gun/energy 9.34 GeV    # 改为目标能量
```

**PrimaryGeneratorAction.cc 中修改**：
- `E1[43]`：质子的能量分bin（第825-838行）
- `E2[43]`：其他粒子的能量分bin（第841-853行）
- `num[43]`：对应的方向表行号

若要添加新粒子种类，需要：
1. 在 `PrimaryGeneratorAction::GeneratePrimaries()` 的参数查找逻辑中添加新分支
2. 确保对应的Störmer锥数据行存在

### 11.3 修改大气模型

**更换大气分层**：修改 `DetectorConstruction.hh` 中的 `AtmLayer[209]` 数组（第76-94行）

**更换大气成分**：修改 `../ctable/layers_lat_<lat>.csv` 文件，格式为：
```
<N₂质量比例>,<O₂质量比例>,<Ar质量比例>,<密度>
```
注意CSV不应有header行。

### 11.4 修改磁场模型

**使用IGRF14完整模型（更高精度）**：
Python层（`tracing.py`）已使用IGRF14。C++层使用简化的一阶偶极近似。

若要在C++侧升级磁场模型：
1. 修改 `MyMagneticField::GlogalDipoleField()` 中的高斯系数
2. 或将IGRF的高阶系数也加入计算

**切换磁场类型**：
```
/field/SetType uniform        # 均匀场（测试用）
/field/SetType dipole         # 偶极场（天顶坐标）
/field/SetType globaldipole   # 全球偶极场（GTOD坐标，推荐）
```

### 11.5 修改能量截断

```cpp
// ActionInitialization.cc 第33行
stacking->SetEnergyThreshold(15.0 * MeV);  // 堆栈截断

// TrackerSD.cc 第139行
if (kE >= 30. * MeV)  // 记录截断（SD1/SD2）

// SphericalSystemSDinAir.cc 第191行  
if (kE >= 30. * MeV)  // 记录截断（大气层SD）
```

### 11.6 修改探测器位置

SD1（轨道探测器）高度：修改 `DetectorConstruction.hh` 中 `SD1H` 默认值（第68行，默认565km），或在宏文件中通过 `SetSD1Height` 设置。

SD2始终在地面（Rmin - 10cm）。

### 11.7 修改输出格式

**RunAction.cc** 中的Ntuple定义（第47-95行）控制ROOT输出格式。
stderr二进制输出格式（第112-119行）控制Python通信格式——修改时需保持Python侧的 `particle_dtype` 一致。

### 11.8 添加新的灵敏探测器

1. 创建新的SD类（参考 `TrackerSD` 或 `SphericalSystemSDinAir`）
2. 在 `DetectorConstruction::Construct()` 中创建对应的几何体
3. 在 `DetectorConstruction::ConstructSDandField()` 中注册SD
4. 在 `RunAction::BeginOfRunAction()` 中添加新的Ntuple（如需新输出格式）

---

## 12. 已知问题与注意事项

### 12.1 硬编码路径

多处硬编码了 `/home/ams/lchen/...` 路径：
- `run_command/shell-generate.cc`（第24-41行）中的环境变量路径
- `run_command/test.sh` 中的cd路径
- `BinaryReader.cc` 中 `../ctable/` 的相对路径依赖

**修改时需注意**：这些路径在不同机器上需要调整。

### 12.2 Störmer锥数据依赖

`PrimaryGeneratorAction` 中的Primary模式强依赖 `../ctable/all_<lon>_<lat>.csv` 文件的存在。如果没有对应经纬度的预计算数据，程序会失败。

### 12.3 多线程中的磁场克隆

`DetectorConstruction::ConstructSDandField()` 中实现了线程安全的磁场克隆，但在 `FieldMessenger` 中可能存在竞态条件——主线程修改磁场参数时，已克隆的工作线程磁场不会更新。

### 12.4 内存使用

- `DirectionData.cc` 约492KB静态数据
- `BinaryReader::readCSVinAll()` 可能加载大量bitset数据
- Python多进程（最多32进程）每个进程使用scipy积分器，内存占用可能较大

### 12.5 testdd.cc 的独立性

`testdd.cc` 是一个独立的方向生成测试程序（有自己的 `main()`），不参与主模拟流程。它用于单独验证 `GeneratorDirection()` 函数的正确性。

### 12.6 数据输出位置

- GEANT4 ROOT输出：`build/` 目录下（如 `protondata_0.root`）
- Python轨迹输出：`../result/TraceData/`
- 次级粒子数据：`../result/seconddata*_*.root`
- Hit数据：`../result/HitChargePdata*_*.root`

### 12.7 遗留/注释代码

代码中有大量被注释掉的代码段，包括：
- 旧版多进程方式（使用 `mps.Process` + `Queue`）
- 废弃的文件读取方式（直接从dat文件读取大气数据）
- 预留的SphericalSystemSDinOrbit功能（轨道层球壳SD）

这些是开发迭代过程中的痕迹。

---

## 附录：关键数据流图

```
                    ┌──────────────┐
                    │  run.mac     │ GEANT4宏文件
                    └──────┬───────┘
                           │
                    ┌──────▼───────┐
                    │   test02     │ GEANT4可执行文件
                    │  (C++/G4)    │
                    └─┬─────────┬──┘
                      │         │
          ROOT文件    │         │ stderr管道
          (Ntuple)    │         │ (二进制粒子数据)
                      │         │
          ┌───────────▼─┐  ┌────▼──────────┐
          │ root输出     │  │ manager.py     │
          │ DetectionInfo│  │ (Python主控)   │
          │ BoundaryInfo │  └────┬───────────┘
          │ EventInfo    │       │
          └──────────────┘  ┌────▼───────────┐
                            │ tracing.py     │
                            │ (磁场轨迹反推)  │
                            └────┬───────────┘
                                 │
                            ┌────▼───────────┐
                            │ 筛选后的粒子   │ → CSV → 新宏文件
                            │ Parlist (+)    │ → test2.mac
                            │ Hitlist (记录) │ → HitData ROOT
                            └────┬───────────┘
                                 │
                            ┌────▼───────────┐
                            │  test02 (再次) │ → 循环
                            └────────────────┘
```

---

> **文档版本**: v1.0  
> **最后更新**: 2026-06-05  
> **分析基于**: SDIAT_GEANT4/Opposite 全部源文件
