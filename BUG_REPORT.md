# 代码 Bug 审查报告

> 审查范围：`Opposite/`（GEANT4 模拟 + Python 两层调度）、`Opposite/combine/`（ROOT 合并库）、`Analysis/`（通量分析）。
> 审查时间：2026-10-22
> 更新：2026-10-22（评审本次 3 处源码改动，标记修复状态，并新增「🔧 修改方案」章节）
> 说明：本报告按严重程度分为 🔴 严重 / 🟠 中等 / 🟡 轻微。每条问题标注状态：✅ 已修复 / ⚠️ 修复尝试有误 / ⬜ 待修复。

---

## 本次改动评审结论（2026-10-22 更新）

本次共改动 3 个源文件，对应报告里的 3 条 🔴 问题：

| 文件 | 改动 | 对应问题 | 结论 |
|------|------|---------|------|
| `Opposite/src/MyMagneticField.cc:41` | `Clone()` 增加 `clonedField->fFieldTypeEnum = fFieldTypeEnum;` | 🔴-1 | ✅ 修复正确 |
| `Opposite/src/FieldMessenger.cc:139` | `SetDipoleFieldParameters` → `SetUniformFieldParameters` | 🔴-4 | ✅ 修复正确 |
| `Opposite/src/PrimaryGeneratorAction.cc:103` | `col < 8` → `col =< 8` | 🔴-5 | ⚠️ **引入语法错误**：`=<` 不是合法 C++ 运算符，**无法编译** |

**结论**：
- 🔴-1、🔴-4 已正确修复。
- 🔴-5 修复方向正确（应读满 9 列以覆盖 `fields[8]`），但 `=<` 是笔误，导致**程序无法通过编译**，必须立即改正（详见 🔴-6）。
- 其余 24 条问题（🔴-2、🔴-3、🟠-6~17、🟡-18~27）**本次未改动**，仍待修复。

---

## 总览

| 级别 | 原数量 | 已修复 | 待处理 |
|------|--------|--------|--------|
| 🔴 严重 | 5 | 2（🔴-1、🔴-4） | 4：🔴-2、🔴-3、🔴-5（⚠️ 修复有误）、🔴-6（本次新增） |
| 🟠 中等 | 12 | 0 | 12：🟠-6 ~ 🟠-17 |
| 🟡 轻微 | 10 | 0 | 10：🟡-18 ~ 🟡-27 |

---

## 🔴 严重问题

### 🔴-1. `MyMagneticField::Clone()` 未复制 `fFieldTypeEnum` → 多线程下磁场完全失效　【✅ 已修复】

**文件**：
- `Opposite/src/MyMagneticField.cc:29-43`（`Clone()`）
- `Opposite/include/MyMagneticField.hh:51`（`fFieldTypeEnum` 默认 `kNone`）
- `Opposite/src/MyMagneticField.cc:113-118`（`GetFieldValue` 只认 `fFieldTypeEnum`）
- `Opposite/src/DetectorConstruction.cc:452`（worker 线程调用 `Clone()`）

**问题**：`GetFieldValue` 的开关用的是枚举 `fFieldTypeEnum`，而 `Clone()` 只复制了字符串 `fFieldType`、`fGeolocation`、`fUniStrength`、`fFieldEnabled`，**唯独漏掉了 `fFieldTypeEnum`**。克隆出的新对象 `fFieldTypeEnum` 保持默认值 `FieldType::kNone`。

**后果**：宏文件里 `/field/SetType globaldipole` 只设置了主线程的 master 场。运行 `manager.py`（`/run/numberOfThreads 32`）时，每个 worker 线程在 `ConstructSDandField` 里 `Clone()` 出的磁场对象，其 `GetFieldValue` 永远走 `default: B=0`，即**所有 worker 线程的磁场恒等于 0**。这意味着 32 线程模拟里，地磁场偏转这一核心物理实际上是被静默关闭的。

```cpp
// MyMagneticField.cc:29
MyMagneticField *MyMagneticField::Clone() const {
    MyMagneticField *clonedField = new MyMagneticField();
    clonedField->fFieldType      = fFieldType;
    clonedField->fGeolocation    = fGeolocation;
    clonedField->fUniStrength    = fUniStrength;
    clonedField->fFieldEnabled   = fFieldEnabled;
    // ❌ 缺少：clonedField->fFieldTypeEnum = fFieldTypeEnum;
    return clonedField;
}
```

---

### 🔴-2. `ConstructSDandField` 中 `static` 局部变量 `threadField` 多线程数据竞争

**文件**：`Opposite/src/DetectorConstruction.cc:443`

```cpp
static MyMagneticField *threadField = nullptr;   // ❌ static 在所有线程间共享
if (G4Threading::IsWorkerThread()) {
    threadField = fMagneticField->Clone();       // 每个 worker 都往同一个 static 写
} else {
    threadField = fMagneticField;
}
```

**问题**：函数级 `static` 变量是**所有线程共享同一份内存**的。32 个 worker 线程各自把 `Clone()` 结果写进同一个 `threadField`，发生数据竞争，且线程 A 可能读到线程 B 克隆出的场对象。即使没有竞争，`static` 也违背了“每个线程持有自己的场”的意图。

**建议**：改成普通局部变量（`ConstructSDandField` 每次调用都新建），或 `G4ThreadLocal`。

---

### 🔴-3. `GeneratePrimaries` 中 `targetrow` 未初始化 + 浮点 `==` 比较

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:834-864`

```cpp
int targetrow;                                  // ❌ 未初始化
if (Pname == "proton") {
    for (int i = 0; i < 54; i++) {
        if (E1[i] == Ek) { targetrow = i; break; }   // ❌ 浮点 == 比较
    }
} else {
    for (int i = 0; i < 54; i++) {
        if (E2[i] == Ek) { targetrow = i; break; }
    }
}
...
dpp = GeneratorDirectionCSV1(str0, pos[0], pos[1], pos[2], lat, lon, targetrow);
```

**问题**：
1. `Ek = fParticleGun->GetParticleEnergy()` 与 `E1/E2`（54 个 bin 能量，单位 MeV）做**精确浮点相等比较**。能量一旦不是精确的 bin 值（例如默认 `11.98*GeV = 11980 MeV`，或 `run/run_H/*.mac` 里的 `5.59 GeV = 5590 MeV`，而 `E1[30]=5586`），循环就找不到匹配项，`targetrow` 保持**未初始化**。
2. 未初始化的 `targetrow` 随后传给 `GeneratorDirectionCSV1` → `getRandomAllowed(targetrow)` → `allowedIndices.at(targetrow)`，越界访问抛 `std::out_of_range`（或产生随机错误行为）。

**证据**：`run.mac:26` 用 `7.244 GeV`（=7244，恰好命中 `E1[32]`）所以碰巧能跑；但 `run/run_H/run_H_1.mac:14` 用 `5.59 GeV`（=5590，不命中 5586）等一批宏文件使用了**四舍五入的 bin 中心值**，这些宏文件都会触发未初始化 `targetrow`。

---

### 🔴-4. `/field/UniStrength` 命令错误地写到了偶极场参数　【✅ 已修复】

**文件**：`Opposite/src/FieldMessenger.cc:132-140`

```cpp
}else if (command == fSetUniformFieldParametersCmd) {
    ...
    G4ThreeVector strength = fSetUniformFieldParametersCmd->GetNew3VectorValue(newValue);
    fField->SetDipoleFieldParameters(strength);   // ❌ 应该是 SetUniformFieldParameters
}
```

**问题**：`SetDipoleFieldParameters` 写的是 `fGeolocation`（偶极场的经纬位置），而 `SetUniformFieldParameters` 写的才是 `fUniStrength`（均匀场强度）。结果：`/field/UniStrength` 设置的均匀场强度永远不会生效，反而会污染偶极场的地理位置参数。

---

### 🔴-5. `SetNewValue(Readfile)` 只读 8 列却访问 `fields[8]`　【⚠️ 修复尝试引入语法错误，仍需修复】

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:103-116`

```cpp
while (col < 8 && std::getline(ss, cell, ',')) {   // ❌ 只读 8 列 → fields[0..7]
    fields.push_back(cell);
}
...
p.kineticEnergy = std::stod(fields[6]) * MeV;
p.pdgCode       = std::stoi(fields[7]);
p.time          = std::stoi(fields[8]) * s;        // ❌ fields[8] 越界访问
```

**问题**：注释里说“第 9 列 → time”，但循环条件 `col < 8` 只解析了前 8 列（索引 0–7）。第 116 行访问 `fields[8]` 是**越界访问**，属于未定义行为，大概率崩溃或读到垃圾值。`Secondary` 生成器（`manager.py` 会写 `/generator/Readfile`）一旦触发就会命中此 bug。

---

### 🔴-6. 【本次改动引入】`PrimaryGeneratorAction.cc:103` `=<` 语法错误，无法编译

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:103`

**问题**：本次改动把
```cpp
while (col < 8 && std::getline(ss, cell, ','))   // 原代码
```
改成了
```cpp
while (col =< 8 && std::getline(ss, cell, ','))   // ❌ `=<` 不是合法运算符
```

C++ 中「小于等于」写作 `<=`；`=<` 会被解析成两个相邻 token（`=` 与 `<`），属于语法错误，**整个 `test02` 将无法编译**。这是本次改动引入的新问题，优先级最高，须先修正。

**正确写法**（任选其一）：
```cpp
while (col <= 8 && std::getline(ss, cell, ','))   // ✅ 读满 9 列（索引 0..8）
// 或
while (col < 9  && std::getline(ss, cell, ','))   // ✅ 等价，更推荐
```

> 该行的意图是修复 🔴-5（读满 9 列以访问 `fields[8]`），方向正确；只需把笔误 `=<` 改为 `<=`（或 `< 9`）即可同时修复 🔴-5 与 🔴-6。

---

## 🟠 中等问题

### 🟠-6. `SelectDirection` 二分查找使用未初始化变量 `c`

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:209-231`

```cpp
int a = 0, b = 360, c;                 // ❌ c 未初始化
while (abs(a - b) != 1) {
    if (philist[b] > phi && philist[a] < phi) {
        c = b; b = (a + b + 1) / 2;
    } else if (philist[b] < phi) {
        a = b; b = c;                   // ❌ 若首次进入此分支，c 是垃圾值
    }
    ...
}
```

**问题**：`c` 只在第一个 `if` 分支被赋值；若循环第一次就进入 `else if` 分支，`b = c` 使用了未初始化的 `c`。该函数（`SelectDirection`/`SelectDirection2`）虽已被 `GeneratorDirectionCSV1` 取代（legacy），但仍是明确的逻辑缺陷。

---

### 🟠-7. 打开文件失败后仍 `fclose(NULL)` + 使用未初始化数组

**文件**：
- `Opposite/src/PrimaryGeneratorAction.cc:132-160`（`GeneratorEnergyFromSpectrum`）
- `Opposite/src/PrimaryGeneratorAction.cc:182-206`（`SelectDirection`）
- `Opposite/src/PrimaryGeneratorAction.cc:311-330`（`SelectDirection2`）

```cpp
FILE *fp = fopen("Spectrum.dat", "rt");
if (!fp) { printf("Error ..."); }   // ❌ 只打印，不 return
else { /* 读数据 */ }
fclose(fp);                          // ❌ fp 为 NULL 时 fclose(NULL) 是 UB
return energies[index];              // ❌ energies/weights 未初始化即使用
```

**问题**：文件打开失败时只打印错误信息，没有提前返回，导致后续 `fclose(NULL)`（未定义行为）以及 `energies[index]`/`weights` 未初始化即使用。三个函数都有同样的问题。

---

### 🟠-8. `SphericalSystemSDinAir/Orbit` 构造函数重复 `push_back` 使 `fRadii` 翻倍

**文件**：
- `Opposite/src/SphericalSystemSDinAir.cc:30-37`
- `Opposite/src/SphericalSystemSDinOrbit.cc:30-36`

```cpp
SphericalSystemSDinAir(const G4String& name, const std::vector<G4double>& radii)
    : G4VSensitiveDetector(name), fRadii(radii) {   // ❌ 已经复制了一遍
    ...
    for (size_t i = 0; i < radii.size() - 1; ++i) {
        fRadii.push_back(radii.at(i));              // ❌ 又追加了一遍
    }
}
```

**问题**：`fRadii(radii)` 已把 `radii` 复制进去，后面循环又把 `radii[0..size-2]` 追加一遍，导致 `fRadii` 变成约 2 倍长度。另外若 `radii` 为空，`radii.size()-1` 会下溢成 `SIZE_MAX`，循环对 `radii.at(0)` 越界抛异常。目前 `fRadii` 在后续代码中未被使用，所以暂未造成实际错误，但属于明确的逻辑 bug。

---

### 🟠-9. `DetectorConstruction::Construct` 检查了错误的 vector

**文件**：`Opposite/src/DetectorConstruction.cc:326`

```cpp
if (static_cast<int>(logicSphereSDlayerinOrbit.size()) != 0) {
    G4cout << "Error!!! ..." << G4endl;
    return physWorld;
}
// 下面实际 push 的是 logicSphereSDlayerinAir
```

**问题**：重入检查检查的是 `logicSphereSDlayerinOrbit`（永远为空的成员），但实际填充的是 `logicSphereSDlayerinAir`。若 `Construct` 被重复调用，不会报错，反而会重复创建 SD4 体积，产生几何重叠。

---

### 🟠-10. `FieldMessenger::SetNewValue` 两个相同的 `else if` 分支，第二个不可达

**文件**：`Opposite/src/FieldMessenger.cc:114` 与 `:123`

```cpp
}else if (command == fSetDipoleFieldParametersCmd) { ... }   // 分支 1
...
}else if (command == fSetDipoleFieldParametersCmd) { ... }   // ❌ 分支 2 永远不执行
```

**问题**：第二个 `else if` 的判断条件与第一个完全相同（本意可能是想区分 `dipole` 和 `globaldipole`），但第二个分支永远不可达，属于死代码。

---

### 🟠-11. `SphericalSystemSDinAir/Orbit` 的 Time 列读取未初始化内存

**文件**：
- `Opposite/src/SphericalSystemSDinAir.cc:149-151`（`SetTime` 被注释掉）、`:171`（`GetTime()`）
- `Opposite/src/SphericalSystemSDinOrbit.cc:136-138`、`:159`
- `Opposite/include/SphericalSystemHitinAir.hh:87`（`G4double fTime;` 无初始化）
- `Opposite/include/SphericalSystemHitinOrbit.hh:86`（同上）

```cpp
// newHit->SetTime(aStep->GetPostStepPoint()->GetGlobalTime());   // ❌ 被注释
// newHit->SetlTime(...);
// newHit->SetpTime(...);
...
G4double timeOfArr = newHit->GetTime();   // ❌ fTime 未初始化
pdata->FillNtupleDColumn(1, 10, timeOfArr/ns);  // ❌ 垃圾值写入 ntuple
```

**问题**：`SphericalHitinAir/Orbit` 的 `fTime` 成员没有默认值，而 `ProcessHits` 中 `SetTime` 被注释掉了，于是 `GetTime()` 返回未初始化的垃圾值，写进 `DetectionInfo` ntuple 的 Time 列。对比 `TrackerSD`（SD1/SD2）中 `SetTime` 是正常调用的（`TrackerSD.cc:108`），所以 SD1/SD2 的时间列是对的，而 Air/Orbit 的时间列是垃圾值。

---

### 🟠-12. 同一 ntuple 内 Time 单位不一致（`/ns` 与 `/s` 混用）

**文件**：
- `Opposite/src/SphericalSystemSDinAir.cc:208`（`timeOfArr/ns`）
- `Opposite/src/SphericalSystemSDinOrbit.cc:196`（`timeOfArr/s`）
- `Opposite/src/TrackerSD.cc:164`（SD2 用 `/ns`）、`:202`（SD1 用 `/s`）

**问题**：`DetectionInfo`（ntuple 1）里，SD2 和 Air 的 Time 列是纳秒，而 Orbit 的 Time 列是秒，同一棵树里的时间单位不统一，直接下游分析会算错。`BoundaryInfo`（ntuple 2）里 SD1 用的是秒。

---

### 🟠-13. `BinaryReader::readROOTSpecificBranch` 无空指针检查 + 悬垂临时字符串

**文件**：`Opposite/src/BinaryReader.cc:89-104`

```cpp
TFile *file = TFile::Open(filename.c_str());          // 可能返回 null
TTree *allowed = (TTree *)file->Get("allowed");       // 可能返回 null
allowed->SetBranchAddress(std::to_string(targetBranch).c_str(), &aflag);  // ❌ 悬垂指针
```

**问题**：
1. `TFile::Open` 与 `file->Get("allowed")` 都没有判空，文件/树不存在时直接解引用空指针崩溃。
2. `std::to_string(targetBranch).c_str()` 是对**临时 `std::string`** 取 `c_str()`，临时对象在语句结束后销毁，指针悬垂。
3. `&aflag` 是 `Double_t`，若 ROOT 分支实际是 `Int_t` 会读错数据。（该函数是 legacy，`GeneratorDirectionRoot` 已不被主流程使用。）

---

### 🟠-14. `readCSVinAll` 无列数上界 → `row.set(col)` 越界

**文件**：`Opposite/src/BinaryReader.cc:31-42`

```cpp
while (std::getline(ss, cell, ',')) {   // ❌ 没有 col < 13963 保护
    if (cell == "1.0") { row.set(col); } // ❌ col >= 13963 时 std::bitset::set 抛异常
    col++;
}
```

**问题**：`readCSVSpecificRow`（`:70`）有 `col < 13963` 的上界保护，但 `readCSVinAll` 没有。若某行逗号分隔的单元格超过 13963 个，`row.set(col)` 抛 `std::out_of_range`。

---

### 🟠-15. `ChainFilter` 构造函数 TFile 泄漏 + `delete currentTree` 双重所有权

**文件**：`Opposite/combine/src/ChainFilter.cc:11-22, 52-54`

```cpp
ChainFilter::ChainFilter(const char *filename, const char *treename) {
    TFile *f = TFile::Open(filename);        // ❌ f 是局部变量，泄漏
    currentTree = (TTree *)f->Get(treename); // ❌ 未判空
}
...
// filterStep / 析构函数中：
delete currentTree;                           // ❌ currentTree 由 TFile 拥有，delete 它属双重释放
```

**问题**：
1. `TFile *f` 是构造函数局部变量，从未保存、从未关闭 → 文件句柄泄漏。
2. `currentTree = f->Get(...)` 返回的指针归 `TFile` 所有，却在 `filterStep` 和析构里 `delete currentTree`，属于删除一个被 TFile 拥有的对象，有双重释放/崩溃风险。

---

### 🟠-16. `manager.py` 读取了 `count` 却未用于切片

**文件**：`Opposite/manager.py:71-88`

```python
count = int.from_bytes(raw_count, byteorder='little', signed=False)
print(f"接收到 {count} 个粒子")
...
raw_data = result.stderr[4:]        # ❌ 没用 count，直接取全部剩余字节
return np.frombuffer(raw_data, dtype=particle_dtype)
```

**问题**：C++ 端写的是 `count` 个 `ParticleData`（`count * 88` 字节），但 Python 端算出了 `count` 却没有用它来切 `count * itemsize` 字节，而是直接把 `stderr[4:]` 全部当粒子数据。一旦 stderr 里混入任何其它输出（GEANT4 的 `G4cerr`、warnings 等），二进制解析就会被污染。

---

### 🟠-17. `FluxCalculationOn_T_E` 调用 `AreaFilter` 缺少 `model` 参数

**文件**：`Analysis/mypylib/FluxCalculation.py:61`

```python
def AreaFilter(xyz, xyzlocal, d, model):        # 需要 4 个参数
    ...
idxInA = AreaFilter(xyz, xyzlocal, ARange[3])   # ❌ 只传了 3 个参数 → TypeError
```

**问题**：`FluxCalculationOn_T_E` 调用 `AreaFilter` 时漏传 `model`，一旦被调用会直接 `TypeError`。

---

## 🟡 轻微问题 / 潜在风险

### 🟡-18. `SteppingAction::n` 成员变量跨 track 计数串扰

**文件**：`Opposite/src/SteppingAction.cc:39-44`

`n` 是类成员，被所有 track 共享。非 `Transportation` step 会把 `n` 重置为 0，导致不同 track 之间的连续步数计数互相干扰，`n>100` 的“卡住判定”可能误判。

### 🟡-19. `TrackingAction::PreUserTrackingAction` 中 `if (parentID >= 0)` 恒真

**文件**：`Opposite/src/TrackingAction.cc:35`

primary 粒子的 `parentID` 为 0，`parentID >= 0` 恒成立，该判断没有实际作用（注释说“只存父粒子信息”，但实际是按自身 `trackID` 存储）。

### 🟡-20. `combine/Combine.cc` 内存泄漏

**文件**：`Opposite/combine/src/Combine.cc:27-49, 71-76, 100-103`

`CloneTree()` 返回的临时树（`DetectionInfo`、`EventInfo` 等）在写完后没有 `delete`，每次合并泄漏内存。`dataAnalyseTest.cc` 中 `new mylib::Combine()` 也从不 `delete`。

### 🟡-21. `DetectorConstruction` 中 `fieldMgr` 未注册删除 → 泄漏

**文件**：`Opposite/src/DetectorConstruction.cc:460`

`G4FieldManager *fieldMgr = new G4FieldManager(threadField);` 未 `G4AutoDelete::Register`，每个线程泄漏一个 field manager。

### 🟡-22. `tracing.py` 中 `rigidity = pGeV / abs(charge)` 潜在除零

**文件**：`Opposite/mypylib/tracing.py:168`

当前 `ParticleSelect` 只选电荷非零的粒子（e±/p/p̄/核），所以暂时安全；但若将来加入中子（charge=0），这里会除零。

### 🟡-23. `manager.py` 注释块中第二处 `particle_dtype` 字段顺序不同

**文件**：`Opposite/manager.py:76-81`（生效）vs `:305-309`（注释块）

生效的 dtype 顺序是 `..., PID, t, charge, m`；注释块里的顺序是 `..., PID, charge, t, m`（`charge` 与 `t` 颠倒）。若有人取消注释误用第二处，会与 C++ 端 `ParticleData` 的字段顺序（`t` 在前）错位，静默解出错误数据。建议删除或统一。

### 🟡-24. `trac2Orbit_vec` 粒子背向球面运动时取到负根

**文件**：`Analysis/mypylib/ToHigherOrbit.py:60`

`t = (-b + sqrt(disc))/(2*a)` 恒取较大根；当粒子已在高轨道外且方向背离地球时，该根为负，`X + V*t` 得到粒子背后的点（错误位置）。当前调用假设粒子从低到高，属于潜在边界问题。

### 🟡-25. `FluxCalculationOn_T_P` 与 `_T_P_E` 方向符号不一致

**文件**：`Analysis/mypylib/FluxCalculation.py:109` vs `:146`

`FluxCalculationOn_T_P` 用 `xyz2rthetaphi(-mx,-my,-mz)`（取负，因为“探测器探测的是进入方向”），而 `FluxCalculationOn_T_P_E` 用 `xyz2rthetaphi(mx,my,mz)`（不取负）。两个函数的 theta/phi 符号约定不一致，下游比较会引入系统性偏差。

### 🟡-26. `str0` 文件名依赖经纬度整数度取整

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:865`

`std::to_string(int(std::round(lon/deg)))` 依赖 `lon/lat` 取整到整数度。若 `lon` 不在整数度（或取整边界），构造出的 `all_<lon>_<lat>.csv` 文件名将不存在，`GeneratorDirectionCSV1` 返回 `(0,0,0)` 方向（静默错误）。

### 🟡-27. `tracing.main` 的 `n` / `thread` 参数未使用

**文件**：`Opposite/mypylib/tracing.py:159`

`def main(Xinit, monento, otherpar, i, n, thread)` 中的 `n` 和 `thread` 从未被使用（原用于生成文件名，现已注释掉）。不算 bug，但参数冗余，容易误导。

---

## 建议的修复优先级

1. **先修 🔴-1 / 🔴-2**：这是多线程下磁场静默失效 + 数据竞争，直接影响所有 32 线程仿真的物理正确性，且**没有任何报错**，危害最大。
2. **再修 🔴-3 / 🔴-5 / 🔴-4**：分别会导致 `Primary`/`Secondary` 生成器崩溃或参数写错。
3. **🟠-11 / 🟠-12**：Time 列垃圾值 + 单位混乱，影响下游通量/时间分布分析的正确性。
4. 其余按需修复。

> 未在报告中展开的点：`PrimaryGeneratorAction.cc` 中 `dtest`（`:881`）等若干计算后未使用的死变量；`SphericalSystemSDinAir.cc:87-97` 的 `AltJudge` 逻辑（当前被注释未启用）。

---

## 🔧 修改方案（Fix Plans）

> 覆盖所有「待修复」与「本次新增」的问题，按优先级排列。每条给出最小改动（✅）与可选的更稳健写法。改动后请重新编译 `test02` 并跑一个宏文件验证（尤其验证 32 线程下磁场是否生效、`Secondary` 生成器是否正常）。

### 修复优先级

1. 🔴-6（编译错误，**必须先改**）
2. 🔴-2（多线程磁场数据竞争）
3. 🔴-3（`targetrow` 未初始化 + 浮点 `==`）
4. 🔴-5（`fields[8]` 越界）
5. 🟠-11 / 🟠-12（Time 列垃圾值 + 单位混乱）
6. 🟠-16 / 🟠-17（Python 端解析与调用错误）
7. 其余按需。

---

#### 🔴-6. `=<` 语法错误（本次新增，阻塞编译）

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:103`

```cpp
// ❌ 现状
while (col =< 8 && std::getline(ss, cell, ','))
// ✅ 修正（任选其一）
while (col <= 8 && std::getline(ss, cell, ','))
while (col <  9 && std::getline(ss, cell, ','))   // 推荐
```

---

#### 🔴-2. `static` 局部变量 `threadField` 数据竞争

**文件**：`Opposite/src/DetectorConstruction.cc:443`

```cpp
// ❌ 现状：static 在所有线程间共享，32 个 worker 互相覆盖
static MyMagneticField *threadField = nullptr;

// ✅ 修正：去掉 static，改为普通局部变量（每个线程各自的栈变量）
MyMagneticField *threadField = nullptr;
```

`ConstructSDandField` 每个线程各调用一次，局部变量天然线程隔离，无需 `G4ThreadLocal`。

---

#### 🔴-3. `targetrow` 未初始化 + 浮点 `==` 比较

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:834-864`

```cpp
// ❌ 现状
int targetrow;                       // 未初始化
...
if (E1[i] == Ek) { targetrow = i; break; }   // 浮点精确相等，几乎匹配不上

// ✅ 修正：初始化 + 最近邻 bin 搜索
int targetrow = 0;
double mindiff = 1e300;
if (Pname == "proton") {
    for (int i = 0; i < 54; i++) {
        double diff = std::fabs(E1[i] - Ek);
        if (diff < mindiff) { mindiff = diff; targetrow = i; }
    }
} else {
    for (int i = 0; i < 54; i++) {
        double diff = std::fabs(E2[i] - Ek);
        if (diff < mindiff) { mindiff = diff; targetrow = i; }
    }
}
```

> 这样 `5.59 GeV(=5590 MeV)` 会落到最近的 bin `E1[30]=5586`，而不再是未初始化。若能量有明确的 bin 边界，用 `std::upper_bound` 做严格分 bin 更精确。

---

#### 🔴-5. `fields[8]` 越界访问

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:103-116`

见 🔴-6 的修正（把 `col < 8` 改为 `col < 9` 即可读满 9 列）。更稳健写法（防御某行逗号数不足）：

```cpp
while (std::getline(ss, cell, ',')) { fields.push_back(cell); }
// 访问前判断列数
if (fields.size() > 8) {
    p.time = std::stoi(fields[8]) * s;
}
```

---

#### 🟠-6. `SelectDirection` 未初始化变量 `c`

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:209-231`（legacy）

```cpp
// ❌ int a = 0, b = 360, c;      // c 未初始化
// ✅ int a = 0, b = 360, c = 0;
```

> 该函数已被 `GeneratorDirectionCSV1` 取代，建议直接删除，而非修复。

---

#### 🟠-7. 打开文件失败后 `fclose(NULL)` + 使用未初始化数组

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:132-160 / 182-206 / 311-330`

```cpp
// ❌ 现状
if (!fp) { printf("Error ..."); }
...
fclose(fp);                 // fp==NULL 时 UB
return energies[index];     // 未初始化

// ✅ 修正：失败即提前返回
if (!fp) {
    printf("Error on opening Spectrum.dat.\n");
    return 0.0;             // GeneratorEnergyFromSpectrum
    // return 0;            // SelectDirection / SelectDirection2（返回 G4int）
}
```

---

#### 🟠-8. `SphericalSystemSDinAir/Orbit` 构造函数 `fRadii` 翻倍

**文件**：`Opposite/src/SphericalSystemSDinAir.cc:29-49`、`SphericalSystemSDinOrbit.cc:29-48`

```cpp
SphericalSystemSDinAir(const G4String& name, const std::vector<G4double>& radii)
    : G4VSensitiveDetector(name), fRadii(radii)   // 已复制一遍
{
    collectionName.insert("SystemHitsinAirCollection");
    // ❌ 删除下面整段循环（会让 fRadii 翻倍 + 空 vector 下溢）
    // for (size_t i = 0; i < radii.size() - 1; ++i) { fRadii.push_back(radii.at(i)); }
}
```
Orbit 同理。

---

#### 🟠-9. `Construct` 检查了错误的 vector

**文件**：`Opposite/src/DetectorConstruction.cc:326`

```cpp
// ❌ if (...logicSphereSDlayerinOrbit.size() != 0)   // 检查了永远为空的成员
// ✅ if (...logicSphereSDlayerinAir.size() != 0)     // 检查实际填充的成员
```

---

#### 🟠-10. `FieldMessenger` 重复的 `else if` 分支（死代码）

**文件**：`Opposite/src/FieldMessenger.cc:123-131`

`/field/Geolocation` 只有一个命令（`fSetDipoleFieldParametersCmd`），第二段 `else if (command == fSetDipoleFieldParametersCmd)` 与第一段条件完全相同，永远不可达。✅ 直接删除第二段（第 123–131 行）。`GlogalDipoleField` 不使用 `fGeolocation`，无需额外命令。

---

#### 🟠-11. Time 列读取未初始化内存

**文件**：`Opposite/src/SphericalSystemSDinAir.cc:149-151`、`SphericalSystemSDinOrbit.cc:136-138`；头文件 `SphericalSystemHitinAir.hh:87`、`SphericalSystemHitinOrbit.hh:86`

```cpp
// ✅ 方案 A：取消注释，真正写入时间
newHit->SetTime(aStep->GetPostStepPoint()->GetGlobalTime());
newHit->SetlTime(aStep->GetPostStepPoint()->GetLocalTime());
newHit->SetpTime(aStep->GetPostStepPoint()->GetProperTime());

// ✅ 方案 B（兜底）：头文件里给 fTime 默认值
G4double fTime = 0.0;   // 原为 G4double fTime; 无初始化
```
两个方案同时做最稳妥。

---

#### 🟠-12. Time 单位不一致（ns/s 混用）

**文件**：`SphericalSystemSDinOrbit.cc:196`（`/s`）、`TrackerSD.cc:202`（SD1 `/s`）

统一为纳秒（与 Air `:208`、SD2 `:164` 一致）：

```cpp
// ❌ SphericalSystemSDinOrbit.cc:196   pdata->FillNtupleDColumn(1, 10, timeOfArr/s);
// ✅                                  pdata->FillNtupleDColumn(1, 10, timeOfArr/ns);
// ❌ TrackerSD.cc:202（SD1）            .../s);
// ✅                                  .../ns);
```

---

#### 🟠-13. `BinaryReader::readROOTSpecificBranch` 空指针 + 悬垂指针

**文件**：`Opposite/src/BinaryReader.cc:89-104`（legacy）

```cpp
TFile *file = TFile::Open(filename.c_str());
if (!file || file->IsZombie()) return result;
TTree *allowed = (TTree *)file->Get("allowed");
if (!allowed) { file->Close(); return result; }
std::string branchName = std::to_string(targetBranch);
allowed->SetBranchAddress(branchName.c_str(), &aflag);   // 用具名 string，避免悬垂
```

---

#### 🟠-14. `readCSVinAll` 无列数上界

**文件**：`Opposite/src/BinaryReader.cc:31`

```cpp
// ❌ while (std::getline(ss, cell, ','))
// ✅ while (std::getline(ss, cell, ',') && col < 13963)
```

---

#### 🟠-15. `ChainFilter` TFile 泄漏 + `delete currentTree` 双重所有权

**文件**：`Opposite/combine/src/ChainFilter.cc:11-22, 52-54`（legacy）

```cpp
// 头文件增加成员：TFile *fFile = nullptr;
ChainFilter::ChainFilter(const char *filename, const char *treename) {
    originalFileName = filename;
    fFile = TFile::Open(filename);
    if (!fFile || fFile->IsZombie()) { currentTree = nullptr; return; }
    currentTree = (TTree *)fFile->Get(treename);   // 归 fFile 所有
}
~ChainFilter() {
    if (fFile) { fFile->Close(); delete fFile; fFile = nullptr; }
    // ❌ 不要 delete currentTree：原树归 fFile 所有，delete 属双重释放
}
```

`filterStep` 里 `delete currentTree; currentTree = newTree;` 同样会 double-free 原树；`newTree`（`CloneTree(0)`）应另用 `bool ownTree` 标志跟踪所有权，在析构时只 delete 自己 Clone 出来的树。

---

#### 🟠-16. `manager.py` 未用 `count` 切分数据

**文件**：`Opposite/manager.py:84`

```python
# ❌ raw_data = result.stderr[4:]
# ✅ 用 count 精确切分 count 个粒子（每个 itemsize 字节）
raw_data = result.stderr[4:4 + count * particle_dtype.itemsize]
```

---

#### 🟠-17. `AreaFilter` 调用缺 `model` 参数

**文件**：`Analysis/mypylib/FluxCalculation.py:61`

```python
# ❌ idxInA = AreaFilter(xyz, xyzlocal, ARange[3])
# ✅ FluxCalculationOn_T_E 是点模式，补上 model=0
idxInA = AreaFilter(xyz, xyzlocal, ARange[3], 0)
```

---

#### 🟡-18. `SteppingAction::n` 跨 track 串扰

**文件**：`Opposite/src/SteppingAction.cc:39-44`

把计数从成员变量改成「按 trackID 存」：`std::map<G4int, G4int> nMap;`，或在 `UserTrackingAction` 起点重置。最小改动：在 `PreUserTrackingAction` 里 `n = 0;`。

---

#### 🟡-19. `TrackingAction` 中 `parentID >= 0` 恒真

**文件**：`Opposite/src/TrackingAction.cc:35`

```cpp
// ❌ if (parentID >= 0)   // primary 的 parentID 为 0，恒成立
// ✅ if (parentID > 0)    // 只有次级粒子才有父粒子
```

---

#### 🟡-20. `combine/Combine.cc` 内存泄漏

**文件**：`Opposite/combine/src/Combine.cc:27-49, 71-76, 100-103`

对 `CloneTree()` 返回的临时树，在写完后 `delete`；`dataAnalyseTest.cc` 中 `new mylib::Combine()` 用智能指针或手动 `delete`。

---

#### 🟡-21. `fieldMgr` 未注册删除

**文件**：`Opposite/src/DetectorConstruction.cc:460`

```cpp
G4FieldManager *fieldMgr = new G4FieldManager(threadField);
G4AutoDelete::Register(fieldMgr);   // ✅ 增加这一行
```

---

#### 🟡-22. `tracing.py` 潜在除零

**文件**：`Opposite/mypylib/tracing.py:168`

```python
# ❌ rigidity = pGeV / abs(charge)
# ✅ rigidity = pGeV / abs(charge) if charge != 0 else float('inf')
```

---

#### 🟡-23. `manager.py` 注释块 dtype 顺序不一致

**文件**：`Opposite/manager.py:305-309`

删除注释块里字段顺序颠倒的第二处 `particle_dtype`（`charge` 与 `t` 颠倒），或改为与生效版一致，避免误用。

---

#### 🟡-24. `trac2Orbit_vec` 背向运动取负根

**文件**：`Analysis/mypylib/ToHigherOrbit.py:60`

```python
import math
disc = b*b - 4*a*c
if disc < 0:
    return None
sq = math.sqrt(disc)
t1 = (-b + sq) / (2*a)
t2 = (-b - sq) / (2*a)
ts = [t for t in (t1, t2) if t >= 0]
t = min(ts) if ts else None   # 取非负根；若都为负说明不交
```

---

#### 🟡-25. `FluxCalculationOn_T_P` 与 `_T_P_E` 方向符号不一致

**文件**：`Analysis/mypylib/FluxCalculation.py:109` vs `:146`

统一「进入方向」约定：两处都取负号，且 theta 定义统一。

```python
# _T_P 已是：  r,t,p = xyz2rthetaphi(-mx,-my,-mz); tdeg = np.rad2deg(t)
# _T_P_E 改为：r,t,p = xyz2rthetaphi(-mx,-my,-mz); tdeg = np.rad2deg(np.pi-t)   # 与 _T_E 一致
```

---

#### 🟡-26. `str0` 文件名依赖经纬度整数度取整

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:865`

构造文件名前校验文件存在，或在 `GeneratorDirectionCSV1` 打开失败时打印错误并返回明确值，避免静默返回 `(0,0,0)`。

---

#### 🟡-27. `tracing.main` 冗余参数

**文件**：`Opposite/mypylib/tracing.py:159`

删除未使用的 `n`、`thread` 参数（及调用处 `LoopBasic` 的对应传参），或保留并加注释说明。
