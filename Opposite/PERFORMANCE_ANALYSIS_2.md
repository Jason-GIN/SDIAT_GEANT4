# SDIAT_GEANT4/Opposite — 性能瓶颈分析与优化方案（修订版）

> **日期**: 2026-06-08（修订）  
> **原版**: 2026-06-05  
> **修订说明**: 基于当前代码实际状态重新分析，修正原版中与实际代码不符的描述（尤其是 2.1 节），更新已实施优化的状态  
> **分级**: 🔴 严重瓶颈 | 🟡 中等瓶颈 | 🟢 轻微影响 | ✅ 已优化

---

## 目录

1. [总体性能画像](#1-总体性能画像)
2. [C++ GEANT4 侧模块分析](#2-c-geant4-侧模块分析)
   - [2.1 PrimaryGeneratorAction — 🔴 最严重瓶颈（重点修订）](#21-primarygeneratoraction--最严重瓶颈)
   - [2.2 MyMagneticField::GetFieldValue — ✅ 已优化 + 🟡 残余瓶颈](#22-mymagneticfieldgetfieldvalue--已优化--残余瓶颈)
   - [2.3 TrackerSD::ProcessHits — 🟡 中等瓶颈](#23-trackersdprocesshits--中等瓶颈)
   - [2.4 DetectorConstruction::Construct — 🟡 初始化瓶颈](#24-detectorconstructionconstruct--初始化瓶颈)
   - [2.5 SteppingAction — 🟢 轻微影响](#25-steppingaction--轻微影响)
   - [2.6 MyStackingAction — 🟢 轻微影响](#26-mystackingaction--轻微影响)
   - [2.7 BinaryReader — 🔴 数据加载瓶颈](#27-binaryreader--数据加载瓶颈)
   - [2.8 SphericalSystemSDinAir — 🟡 冗余记录](#28-sphericalsystemsdinair--冗余记录)
3. [Python 侧模块分析](#3-python-侧模块分析)
4. [系统级瓶颈](#4-系统级瓶颈)
5. [综合优化方案](#5-综合优化方案)
6. [预期性能提升汇总](#6-预期性能提升汇总)
7. [实施优先级路线图](#7-实施优先级路线图)

---

## 1. 总体性能画像

### 模拟计算时间分布估算

基于当前代码路径分析，一次典型的1000-event Primary模式运行的时间分布：

```
┌────────────────────────────────────────────────────────────────┐
│ 阶段                          估算占比    主要瓶颈             │
├────────────────────────────────────────────────────────────────┤
│ GEANT4 粒子输运+物理           55-65%    磁场计算、物理过程     │
│   ├─ 磁场场值查询              18-23%    GetFieldValue每步调用  │
│   ├─ 强子物理过程              20-25%    FTFP_BERT_HP          │
│   └─ 输运步进                  10-15%    步长控制、导航         │
│                                                                │
│ PrimaryGeneratorAction          8-20%    CSV文件I/O在每事件中   │
│   ├─ all_*.csv 读取+解析        7-15%    每事件fopen+扫描3MB   │
│   ├─ bitset拒绝采样             1-2%     do-while随机查找       │
│   └─ 坐标转换                   1-3%     trig + 矩阵乘法       │
│                                                                │
│ SensitiveDetector处理          5-10%    Hit分配+Ntuple填充     │
│                                                                │
│ Python 磁场轨迹反推            15-20%   solve_ivp + IGRF14    │
│                                                                │
│ 文件I/O与数据整理              3-8%     CSV读写、ROOT写入      │
└────────────────────────────────────────────────────────────────┘
```

### 关键数字

| 指标 | 当前值 | 问题 |
|------|--------|------|
| all_*.csv 文件大小 | ~3 MB (54行×13963列) | 每事件都打开+扫描 |
| all_*.csv fopen/事件 | 1次 | 但每次扫描~1.5MB到目标行 |
| bitset拒绝采样 | 0-100+次/事件 | 取决于稀疏度 |
| GetFieldValue 调用/事件 | 10⁴-10⁶次 | 每个charged particle每步调用 |
| ProcessHits heap分配/事件 | 10³-10⁴次 | SD内每步new TrackerHit |
| 大气层逻辑体总数 | 7,488个 | 208层×36纬度带 |

---

## 2. C++ GEANT4 侧模块分析

### 2.1 PrimaryGeneratorAction — 🔴 最严重瓶颈

> **⚠️ 重要修订**：原版 md 描述的 "瓶颈点1: SelectDirection()在do-while循环中每轮打开文件（Störmer锥数据361行）" 是基于旧代码路径。当前 `GeneratePrimaries()` 实际调用的是 `GeneratorDirectionCSV1()`，走的是 bitset CSV 路径，而非 cone 文件路径。`SelectDirection()`/`SelectDirection2()`/`GeneratorDirection()` 仍然存在于代码中，但在主流程中**未被调用**（属于遗留代码），且 ctable 目录中也没有对应的 cone 数据文件。

#### 当前实际执行流程

```
GeneratePrimaries()
  ├─ 构造文件名: "../ctable/all_<lon>_<lat>.csv"  (3个文件之一)
  ├─ 线性查找: E1[43]/E2[43] → targetrow          (43元素循环)
  └─ GeneratorDirectionCSV1(str0, pos, lat, lon, targetrow)
       ├─ BinaryReader::readCSVSpecificRow()        ← 🔴 每事件打开文件!
       │    ├─ std::ifstream file(filename)         打开 ~3MB CSV
       │    ├─ while(getline) 线性扫描到targetRow   平均扫描27行
       │    └─ 解析13963列 "0.0"/"1.0" → bitset     每行 ~28KB
       ├─ do { id=rand()*7500 } while(rowdata.test(id)==0)  ← 🟡 拒绝采样
       ├─ DirectionData::getDirection(id)           静态数组O(1)查找
       ├─ VectorZenithinGTOD1(xyz, colat, lon)      ← 🟡 每事件计算trig
       └─ VectorZenithinGTOD(dp, xyzm)              ← 🟡 每事件计算trig
```

#### 瓶颈点 1（🔴 最严重）：all_*.csv 文件每事件重复读取

**位置**：`src/PrimaryGeneratorAction.cc` 第677-708行 → `src/BinaryReader.cc` 第50-86行

**问题描述**：
```cpp
// GeneratePrimaries() 第855-864行 — 每个事件都调用
std::string str0 = "../ctable/all_" + std::to_string(int(std::round(lon/deg)))
                   + "_" + std::to_string(int(std::round(lat/deg))) + ".csv";
dpp = GeneratorDirectionCSV1(str0, pos[0], pos[1], pos[2], lat, lon, targetrow);

// GeneratorDirectionCSV1() 第686行 — 每次都打开文件
if (BinaryReader::readCSVSpecificRow(filename, targetrow, rowdata))

// BinaryReader::readCSVSpecificRow() 第52行 — 打开+线性扫描
std::ifstream file(filename);
while (std::getline(file, line)) {
    if (currentRow == targetRow) { /* 解析13963列 */ }
    currentRow++;
}
```

**影响量化**：
- 文件大小：all_*.csv 每个 ~3 MB（54行 × 13963列 × ~4字节/列）
- 每事件：打开文件 + 线性扫描（平均 ~27行，~1.5MB）+ 解析13963列字符串
- 每次 `std::stod` 或字符串比较 → 13963次/事件
- 1000事件 → 1000次文件打开 + ~1.5GB 数据读取 + ~14,000,000次字符串解析
- **总耗时估算**：每事件 ~1-5ms（取决于磁盘缓存），1000事件 → **1-5秒纯I/O**
- 对于 `lon`/`lat` 在一次 run 中固定的场景，**文件名始终相同，完全没必要重复读取**

**根本原因**：`all_<lon>_<lat>.csv` 中的数据在整个 run 期间不变（lon/lat 由 DetectorConstruction 设置后不变），应该在初始化时加载到内存。

#### 瓶颈点 2（🟡）：拒绝采样查找允许方向

**位置**：`src/PrimaryGeneratorAction.cc` 第693-698行

```cpp
do {
    id = static_cast<int>(G4UniformRand() * 7500);  // 随机0~7499
} while (rowdata.test(id) == 0);  // 不断重试直到命中
```

**问题**：
- 这是**拒绝采样**（rejection sampling）
- 效率取决于 bitset 中 "1" 的密度：若只有 500 个方向被允许（密度 ~6.7%），平均需重试 ~15次
- 如果某行稀疏（只有几十个允许方向），可能需数百次重试
- 每次重试：一次随机数生成 + 一次 bitset::test()

**影响量化**：
- 每次 `G4UniformRand()`：~50ns
- 每次 `bitset::test()`：~5ns
- 密度 6.7% → 平均 15 次重试 → ~1μs/事件
- 密度 0.5% → 平均 200 次重试 → ~10μs/事件
- 总体影响不大，但代码模式低效

#### 瓶颈点 3（🟡）：能量→行号的线性查找

**位置**：`src/PrimaryGeneratorAction.cc` 第825-853行

```cpp
for (int i = 0; i < 43; i++) {
    if (E1[i] == Ek) { targetrow = num[i]; break; }
    else { continue; };  // ← continue 是多余的
}
```

**影响**：43元素线性扫描 ~100ns，本身很便宜。但这是可以用 O(1) 替代的模式，且代码中 `else { continue; }` 是无效代码。

#### 瓶颈点 4（🟡）：每事件 G4cout 调试输出

**位置**：`src/PrimaryGeneratorAction.cc` 第798-800行、第856-862行

```cpp
// 第798-800行 — 每个事件都输出
G4cout << "-------------GIN0.3---------------" << G4endl;
G4cout << "lat: " << lat/deg << "\t" << "lon: " << lon/deg << G4endl;

// 第856-862行 — 每个事件都输出
G4cout << "Ek: " << Ek << G4endl;
G4cout << "bf targetrow: " << targetrow << G4endl;
G4cout << "str0: " << str0 << G4endl;
```

**影响**：每次 `G4cout` + `G4endl`(flush) 可能 1-10ms。批处理模式下完全无用。

#### 瓶颈点 5（🟡）：每事件的坐标转换 trig 计算

**位置**：`src/PrimaryGeneratorAction.cc` 第728-730行（GeneratorDirectionCSV1）

```cpp
xyzm = VectorZenithinGTOD1(xyz, (90.0 * deg - lat), lon);  // cos(colat), sin(colat), cos(lon), sin(lon)
ddp = VectorZenithinGTOD(dp, xyzm);  // acos(z/r) + atan2(y,x) + 6 trig
```

**问题**：
- `VectorZenithinGTOD1` 内部计算 `cos(colat)`, `sin(colat)`, `cos(lon)`, `sin(lon)` → 4 trig
- `VectorZenithinGTOD` 内部调用 `xyz2r()` → `acos` + `atan2` → 2 trig
- `VectorZenithinGTOD` 内部再计算 `cos(phi)`, `sin(phi)`, `cos(theta)`, `sin(theta)` → 4 trig
- 合计：~10 trig calls + 1 acos + 1 atan2 / 事件
- `lat` 和 `lon` 在整个 run 中是**常量**，每次事件都重新计算它们的 trig 值

#### 🔑 优化方案（PrimaryGeneratorAction 重点）

---

##### 方案 1（🔴 P0 — 最高优先级）：内存化 all_*.csv bitset 数据

这是**收益最大**的优化。核心思路：**一次加载，全 run 复用**。

```cpp
// 新增：PrimaryGeneratorAction.hh 中添加成员
class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
private:
    // ... 现有成员 ...
    
    // 缓存已加载的 all_*.csv 数据
    struct DirectionMaskCache {
        std::string cachedFilename;
        std::vector<std::bitset<13963>> rows;  // 54行全量bitset
        // 预计算每行中设为1的索引列表（用于O(1)随机选择）
        std::vector<std::vector<int>> allowedIndices;  // 每行的允许方向索引
        
        bool load(const std::string& filename) {
            if (filename == cachedFilename && !rows.empty()) return true;
            
            rows.clear();
            allowedIndices.clear();
            
            std::ifstream file(filename);
            if (!file.is_open()) return false;
            
            std::string line;
            while (std::getline(file, line)) {
                std::bitset<13963> row;
                std::vector<int> indices;
                
                std::stringstream ss(line);
                std::string cell;
                int col = 0;
                while (std::getline(ss, cell, ',') && col < 13963) {
                    if (cell[0] == '1') {  // 比 cell == "1.0" 快得多！
                        row.set(col);
                        indices.push_back(col);
                    }
                    col++;
                }
                rows.push_back(row);
                allowedIndices.push_back(std::move(indices));
            }
            cachedFilename = filename;
            return true;
        }
        
        // O(1) 随机选择允许方向（无拒绝采样！）
        int getRandomAllowed(int targetRow) const {
            const auto& indices = allowedIndices.at(targetRow);
            int idx = static_cast<int>(G4UniformRand() * indices.size());
            return indices[idx];
        }
    };
    
    DirectionMaskCache fDirectionCache;
    
    // 预计算常量三角函数（整个run不变）
    double fCachedCosColat = 0.0;
    double fCachedSinColat = 0.0;
    double fCachedCosLon = 0.0;
    double fCachedSinLon = 0.0;
    bool fTrigCached = false;
    
    // 能量→行号 O(1) 映射
    std::unordered_map<double, int> fProtonEnergyToRow;
    std::unordered_map<double, int> fOtherEnergyToRow;
};
```

**优化后的 `GeneratorDirectionCSV1`**：
```cpp
G4ThreeVector PrimaryGeneratorAction::GeneratorDirectionCSV1(
    std::string filename, G4double x, G4double y, G4double z, 
    G4double lat, G4double lon, int targetrow)
{
    // 加载文件（仅首次真正读盘，后续命中缓存）
    if (!fDirectionCache.load(filename)) {
        G4cout << "Error loading: " << filename << G4endl;
        return G4ThreeVector(0, 0, 0);
    }
    
    // O(1) 随机选择允许方向（无拒绝采样循环！）
    int id = fDirectionCache.getRandomAllowed(targetrow);
    
    G4ThreeVector dp = DirectionData::getDirection(static_cast<size_t>(id));
    
    // 使用预计算的 trig 值（如果尚未缓存则计算并缓存）
    if (!fTrigCached) {
        G4double colat = 90.0 * deg - lat;
        fCachedCosColat = std::cos(colat);
        fCachedSinColat = std::sin(colat);
        fCachedCosLon = std::cos(lon);
        fCachedSinLon = std::sin(lon);
        fTrigCached = true;
    }
    
    G4double xyz[3] = {x, y, z};
    G4double xyzm[3], ddp[3];
    
    // 内联展开的 VectorZenithinGTOD1（使用缓存trig）
    xyzm[0] = fCachedCosLon * fCachedCosColat * xyz[0] 
            - fCachedSinLon * xyz[1] 
            + fCachedCosLon * fCachedSinColat * xyz[2];
    xyzm[1] = fCachedSinLon * fCachedCosColat * xyz[0] 
            + fCachedCosLon * xyz[1] 
            + fCachedSinLon * fCachedSinColat * xyz[2];
    xyzm[2] = -fCachedSinColat * xyz[0] + fCachedCosColat * xyz[2];
    
    // 内联展开的 VectorZenithinGTOD（从 xyzm 的球坐标转换 dp）
    G4double r = std::sqrt(xyzm[0]*xyzm[0] + xyzm[1]*xyzm[1] + xyzm[2]*xyzm[2]);
    G4double inv_r = 1.0 / r;
    G4double cos_t = xyzm[2] * inv_r;
    G4double sin_t = std::sqrt(1.0 - cos_t * cos_t);
    G4double phi = std::atan2(xyzm[1], xyzm[0]);
    G4double cos_p = std::cos(phi), sin_p = std::sin(phi);
    
    ddp[0] = cos_p * cos_t * dp[0] - sin_p * dp[1] + cos_p * sin_t * dp[2];
    ddp[1] = sin_p * cos_t * dp[0] + cos_p * dp[1] + sin_p * sin_t * dp[2];
    ddp[2] = -sin_t * dp[0] + cos_t * dp[2];
    
    return G4ThreeVector(-ddp[0], -ddp[1], -ddp[2]);
}
```

**预期提升**：
- 消除每事件文件I/O：**~100-500x 加速**（首次加载后）
- 消除拒绝采样循环：**~10-200x 加速**（采样部分）
- 消除重复trig计算：**~2-3x 加速**（坐标转换部分）
- **GeneratorDirectionCSV1 整体加速：100-500x**

---

##### 方案 2（P1）：能量→行号 O(1) 查找

在构造函数中构建映射：

```cpp
PrimaryGeneratorAction::PrimaryGeneratorAction(DetectorConstruction *detCon)
    : G4VUserPrimaryGeneratorAction(), fDetectorConstruction(detCon)
{
    // ... 现有初始化 ...
    
    // 构建能量→行号映射（一次性）
    for (int i = 0; i < 43; i++) {
        fProtonEnergyToRow[E1[i]] = num[i];
        fOtherEnergyToRow[E2[i]] = num[i];
    }
}

// GeneratePrimaries 中使用:
if (Pname == "proton") {
    auto it = fProtonEnergyToRow.find(Ek);
    targetrow = (it != fProtonEnergyToRow.end()) ? it->second : -1;
} else {
    auto it = fOtherEnergyToRow.find(Ek);
    targetrow = (it != fOtherEnergyToRow.end()) ? it->second : -1;
}
```

**预期提升**：从 43 步线性扫描 → 1 次 hash 查找（~5ns vs ~100ns），微优化。

---

##### 方案 3（P1）：移除/条件化 G4cout 调试输出

```cpp
// 方法A：直接删除（推荐用于批处理）
// 删除第798-800行和第856-862行的G4cout

// 方法B：用 verbose 级别控制
if (verboseLevel > 0) {
    G4cout << "lat: " << lat/deg << "\t" << "lon: " << lon/deg << G4endl;
}

// 方法C：使用 G4cout 的开关
// G4cout 只在 G4State_Idle 时输出，或者通过 UI 命令设置 /run/verbose 0
```

**预期提升**：消除每事件 ~4-8ms 的终端 I/O 抖动。

---

##### 方案 4（P3 — 代码清理）：删除遗留的 cone 相关函数

`SelectDirection()`、`SelectDirection2()`、`GeneratorDirection()` 这三个函数共约 320 行代码，在当前主流程中未被调用。如果确认不再需要可以移除，减少代码维护负担。

`GeneratorDirectionRoot()` 同样未被调用（且依赖 ROOT 的 TFile/TTree），也可以考虑移除。

---

#### 修正：原版 MD 文件的错误

| 原版描述 | 问题 | 实际情况 |
|----------|------|----------|
| "SelectDirection()在do-while循环中每轮打开文件" | cone数据文件不存在于ctable | 主流程使用`GeneratorDirectionCSV1()`（bitset路径），非cone路径 |
| "361行方向锥数据" | 文件不存在 | ctable中只有`all_*.csv`(54行×13963列)和`layers_lat_*.csv` |
| "3步坐标转换：Zenith→GTOD→Zenith" | 当前CSV1路径只有2步 | 去掉了`VectorGTODinZenith1`那一步 |
| "每次打开同一个文件一次（targetRow通常固定）" | targetRow每次事件从能量映射确定 | 文件确实相同，但targetRow取决于能量 |

---

### 2.2 MyMagneticField::GetFieldValue — ✅ 已优化 + 🟡 残余瓶颈

#### ✅ 已优化：字符串比较 → enum switch

**位置**：`src/MyMagneticField.cc` 第113-118行

```cpp
switch (fFieldTypeEnum) {  // 编译为跳转表，O(1)
    case FieldType::kUniform: UniformField(Track, B); break;
    case FieldType::kDipole: DipoleField(Track, B); break;
    case FieldType::kGlobalDipole: GlogalDipoleField(Track, B); break;
    default: B[0]=B[1]=B[2]=0.0;
}
```

✅ 已从 `G4String::operator==`（strcmp，~200 cycles）优化为 enum switch（跳转表，~5 cycles）。

#### ✅ 已优化：GlogalDipoleField 直接计算

**位置**：`src/MyMagneticField.cc` 第222-272行

```cpp
void MyMagneticField::GlogalDipoleField(const G4double Track[7], G4double B[3]) const {
    double r = std::sqrt(x*x + y*y + z*z);
    double cos_theta = z / r;
    // ... 直接计算，无G4ThreeVector临时对象
    B[0] = Br*sin_theta*cos_phi + Bt*cos_theta*cos_phi - Bp*sin_phi;
    B[1] = Br*sin_theta*sin_phi + Bt*cos_theta*sin_phi + Bp*cos_phi;
    B[2] = Br*cos_theta - Bt*sin_theta;
}
```

✅ 已消除了所有 `G4ThreeVector` 临时对象和坐标转换函数调用，直接使用局部 `double` 变量。

#### 🟡 残余瓶颈：DipoleField 仍使用旧模式

**位置**：`src/MyMagneticField.cc` 第161-220行

`DipoleField()` 仍然使用：
```cpp
G4ThreeVector xyzInGTOD, rtpInGTOD, BxyzInGTOD, BxyzInZenith;  // 临时对象
xyzInGTOD = VectorZenithinGTOD1(G4ThreeVector(x, y, z), (90*deg-lat), lon);  // trig!
rtpInGTOD = xyz2rtp(xyzInGTOD);  // acos + atan2
// ... 场计算 ...
BxyzInGTOD = VectorZenithinGTOD1(G4ThreeVector(Bt, Bp, Br), rtpInGTOD[1], rtpInGTOD[2]);
BxyzInZenith = VectorGTODinZenith1(BxyzInGTOD, (90*deg-lat), lon);
```

可以参照 `GlogalDipoleField` 的方式重写为直接计算。

#### 🟡 残余瓶颈：DipoleField 每步计算 `cos(90*deg-lat)` 和 `sin/cos(lon)`

`lat` 和 `lon` 在 `fGeolocation` 中存储，整个 run 不变。预计算到成员变量中：

```cpp
// 在 SetDipoleFieldParameters 或首次 GetFieldValue 时：
void MyMagneticField::SetDipoleFieldParameters(G4ThreeVector geo) {
    fGeolocation = geo;
    G4double colat = 90.0 * deg - fGeolocation[1];
    fCachedCosColat = std::cos(colat);
    fCachedSinColat = std::sin(colat);
    fCachedCosLon = std::cos(fGeolocation[2]);
    fCachedSinLon = std::sin(fGeolocation[2]);
}
```

#### 优化方案

**方案 A（P1）：DipoleField 改写为直接计算** — 参照 GlogalDipoleField 的模式，消除 G4ThreeVector 临时对象和不必要的坐标转换。

**方案 B（P1）：预计算 lat/lon trig** — 在 `SetDipoleFieldParameters` 中计算并缓存。

**预期提升**：DipoleField 加速 40-60%（若使用 dipole 模式）。

---

### 2.3 TrackerSD::ProcessHits — 🟡 中等瓶颈

（本节省略详细代码块，因为原版分析仍然适用，需要关注的点是：）

1. 🔴 **每步 heap 分配 TrackerHit**（`new TrackerHit()`）— 第70行
2. 🟡 **字符串比较判断探测器类型**（`SDetectorName.substr(0,10) == "Test02/SD2"`）— 第139、176行，每次创建临时 string
3. 🟡 **调试 G4cout** — 第148-150行、第185-187行
4. 🟡 **读取刚写入的值**（应先存为局部变量）
5. 🟢 **SD1 中重复判断中微子 PDG 编码** — 第207行，无 `std::abs` 优化

**优化方案**与原版 md 的 2.3 节相同（SDType 枚举化、Hit 对象池化、移除 G4cout、局部变量复用）。

---

### 2.4 DetectorConstruction::Construct — 🟡 初始化瓶颈

与原版 md 的 2.4 节描述一致：
- 7,488 个 Material 对象
- 72 次重复 CSV 读取
- 11,088 个逻辑体

**优化方案**同原版。

---

### 2.5 SteppingAction — 🟢 轻微影响

**位置**：`src/SteppingAction.cc` 第36-38行

```cpp
G4String process = step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
// ...
if (process == "Transportation" && dE < 1.0e+11) {  // 字符串比较每步!
```

**优化**：使用 `GetProcessType()` 枚举比较代替字符串比较。

---

### 2.6 MyStackingAction — 🟢 轻微影响

与原版 md 的 2.6 节一致。

---

### 2.7 BinaryReader — 🔴 数据加载瓶颈

#### 瓶颈点 1（🔴）：readCSVSpecificRow 每事件打开+扫描文件

**位置**：`src/BinaryReader.cc` 第50-86行

**这是 PrimaryGeneratorAction 瓶颈的根本原因。** 优化方案见 2.1 节方案1：在调用侧（PrimaryGeneratorAction）缓存数据，而不是在 BinaryReader 侧优化。

但如果要保留 BinaryReader 的通用性，也可以在此处添加文件级缓存：

```cpp
// BinaryReader 静态缓存
static std::unordered_map<std::string, std::vector<std::bitset<13963>>> fileCache;

bool BinaryReader::readCSVSpecificRow(const std::string &filename, int targetRow,
                                       std::bitset<13963> &result) {
    auto it = fileCache.find(filename);
    if (it == fileCache.end()) {
        // 首次加载全文件
        std::vector<std::bitset<13963>> rows;
        // ... 读取整个文件 ...
        it = fileCache.emplace(filename, std::move(rows)).first;
    }
    if (targetRow >= 0 && targetRow < (int)it->second.size()) {
        result = it->second[targetRow];
        return true;
    }
    return false;
}
```

**预期提升**：首次加载后 O(1)，加速 100-500x。

#### 瓶颈点 2（🟡）：readCSVSpecificRow 解析效率

第72行：`if (cell == "1.0")` — 字符串比较。可以改为 `if (cell[0] == '1')` 节省 ~90% 解析时间。

#### 瓶颈点 3（🟡）：readAtmLayer 中 G4cout

第121行：`G4cout << "---------------GINs---------------" << G4endl;` — 每次读取大气层数据都输出。

---

### 2.8 SphericalSystemSDinAir — 🟡 冗余记录

与原版 md 的 2.8 节一致。额外发现：
- 第103-104行空指针检查每次 ProcessHits 都执行
- 第191行 `kE>=30.*MeV` 检查在已分配 Hit 之后 — 应该提到前面做 early return

---

## 3. Python 侧模块分析

Python 侧的分析与原版 md 第3节基本一致，主要瓶颈仍然是：
1. 🔴 `tracing.py` 中 `solve_ivp` 数值积分
2. 🟡 `manager.py` 中 `np.vstack` O(n²) 复制

此处不再重复展开，参见原版 md 3.1-3.2 节。

---

## 4. 系统级瓶颈

与原版 md 第4节一致，额外补充：

### 4.5 关于 ctable 数据目录

当前 `ctable/` 目录结构：
```
ctable/
├── all_-160_25.csv         (3.0 MB) — lon=-160, lat=25 的方向掩码表
├── all_-170_-25.csv        (3.0 MB) — lon=-170, lat=-25
├── all_160_-25.csv         (3.0 MB) — lon=160, lat=-25
└── layers_lat_*.csv × 36   (16 KB each) — 大气层数据
```

**观察**：目前只有 3 个 `all_*.csv` 文件，覆盖 3 个地理位置。如果需要支持更多位置，每个新位置需要约 3MB 数据。若预加载多份到内存，需考虑内存开销（每份 ~3MB 文件 → ~1.7MB 内存作为 bitset + ~500KB 作为预计算索引列表）。

---

## 5. 综合优化方案

### 按优先级排序的优化清单

| 优先级 | 优化项 | 模块 | 预期加速 | 实现难度 | 风险 |
|--------|--------|------|----------|----------|------|
| **P0** | 内存化 all_*.csv bitset 数据 + 预计算允许索引 | PrimaryGeneratorAction | 100-500x（该模块） | 中 | 低 |
| **P0** | 移除 GeneratePrimaries 热路径 G4cout | PrimaryGeneratorAction | 消除I/O抖动 | 低 | 极低 |
| **P0** | 移除 TrackerSD 热路径 G4cout | TrackerSD | 消除I/O抖动 | 低 | 极低 |
| **P1** | 能量→行号 O(1) 查找 | PrimaryGeneratorAction | 微优化 | 低 | 极低 |
| **P1** | 预计算 lat/lon trig + 内联坐标转换 | PrimaryGeneratorAction | 2-3x（坐标转换） | 低 | 低 |
| **P1** | DipoleField 改写为直接计算 | MyMagneticField | 40-60%（dipole模式） | 中 | 中 |
| **P1** | 预计算 DipoleField 的 lat/lon trig | MyMagneticField | 微优化（热路径） | 低 | 低 |
| **P1** | BinaryReader::readCSVSpecificRow 文件缓存 | BinaryReader | 100-500x（回退路径） | 低 | 低 |
| **P1** | TrackerSD SDType 枚举化 | TrackerSD | 20-30% | 低 | 极低 |
| **P1** | np.vstack → list 收集 | manager.py | 5-10x（Python层） | 低 | 极低 |
| **P1** | 优化事件检测函数（只计算r） | tracing.py | 30-50%（积分） | 低 | 低 |
| **P2** | Hit 对象池化 | TrackerSD | 10-20% | 低 | 低 |
| **P2** | SphericalSystemSDinAir early return | SphericalSystemSDinAir | 20-30% | 低 | 极低 |
| **P2** | 增大 minStep 到 1m | DetectorConstruction | 步数减少30-50% | 低 | 低 |
| **P2** | 自适应积分方法 | tracing.py | 30-50%（积分） | 中 | 中 |
| **P3** | 删除遗留函数（SelectDirection等） | PrimaryGeneratorAction | 代码清理 | 低 | 极低 |
| **P3** | 减少逻辑体数量 | DetectorConstruction | 导航20-30% | 高 | 高 |
| **P3** | 并行 GEANT4 实例 | manager.py | 3-4x（批量） | 高 | 高 |

---

## 6. 预期性能提升汇总

### 端到端加速估算

```
                    当前耗时    优化后    加速比
                    ────────   ────────   ──────
GEANT4 粒子输运      55-65%    30-40%    1.5-2.0x
  - 磁场查询         18-23%     8-10%    2.0-2.5x  ✅ 已部分优化
  - 物理过程         20-25%    15-20%    1.1-1.3x
  - 输运步进         10-15%     5-8%     1.5-2.0x

PrimaryGenerator      8-20%     0.5-1%   20-50x    🔴 最大优化空间
SensitiveDetector     5-10%     3-5%     1.5-2.0x
Python tracing       15-20%     5-8%     2.5-4.0x
文件I/O               3-8%      1-3%     2.0-3.0x
                    ────────   ────────
总计                100%       40-55%   1.8-2.5x
```

### 关键指标预期变化

| 指标 | 优化前 | 优化后 | 改善 |
|------|--------|--------|------|
| 1000 event模拟时间 | ~60-120s | ~25-55s | ~2x |
| all_*.csv fopen/run | 1000次 | 1次（首次加载） | 1000x |
| bitset拒绝采样/事件 | 平均15-200次 | 1次（O(1)随机选择） | 15-200x |
| 坐标转换 trig/事件 | ~12次 | 2次（atan2+cos/sin各1） | 6x |
| GetFieldValue 字符串比较 | 0次 ✅ | 0次 | 已优化 |
| G4cout每事件输出行 | ~6行 | 0行 | ∞ |
| np.vstack 复制总量 | O(N²) | O(N) | ~100x |

---

## 7. 实施优先级路线图

### 第一阶段：低风险、高收益（1天）

```
1. P0: 内存化 all_*.csv bitset 数据 + 预计算允许索引
   ├── PrimaryGeneratorAction 添加 DirectionMaskCache
   ├── 重写 GeneratorDirectionCSV1 使用缓存
   └── 预计算每行的 allowedIndices 向量

2. P0: 移除热路径 G4cout
   ├── PrimaryGeneratorAction::GeneratePrimaries 中的 6 处
   ├── TrackerSD::ProcessHits 中的 4 处
   └── BinaryReader::readAtmLayer 中的 1 处

3. P1: 能量→行号 unordered_map
   └── 构造函数中构建 fProtonEnergyToRow / fOtherEnergyToRow
```

### 第二阶段：中等风险、高收益（2-3天）

```
4. P1: 预计算 lat/lon trig + 内联坐标转换
   ├── 缓存 cos(colat)/sin(colat)/cos(lon)/sin(lon)
   └── 内联展开 VectorZenithinGTOD1 和坐标转换

5. P1: BinaryReader::readCSVSpecificRow 文件缓存（回退优化）
6. P1: TrackerSD SDType 枚举化
7. P1: Python np.vstack → list 收集
8. P1: Python 事件检测函数优化
```

### 第三阶段：深度优化（1周）

```
9.  P2: Hit 对象池化
10. P2: SphericalSystemSDinAir early return
11. P2: 增大 minStep 到 1m
12. P2: DipoleField 直接计算改写
13. P3: 删除遗留函数
```

---

> **总结**: 
> 1. **最重要的优化**是 2.1 节方案1：将 `all_*.csv` 的 bitset 数据在初始化时一次性加载到内存。当前代码每事件都打开 ~3MB CSV 文件、扫描、解析 13963 列，这是 PrimaryGeneratorAction 的主要耗时来源。优化后该模块可达 **100-500x 加速**。
> 2. MyMagneticField 的字符串比较和 GlogalDipoleField 的优化已经完成 ✅。
> 3. 原版 md 文件 2.1 节关于 Störmer 锥数据（cone data）的描述与实际代码不符 — 当前主流程使用的是 bitset CSV 路径，cone 文件路径是遗留代码且 ctable 中没有对应数据文件。
> 4. 实施 P0 优化后，预计整体模拟速度可提升 **1.8-2.5 倍**。
