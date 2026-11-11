# 代码 Bug 审查报告

> 审查范围：`Opposite/`（GEANT4 模拟 + Python 两层调度）、`Opposite/combine/`（ROOT 合并库）、`Analysis/`（通量分析）。
> 审查时间：2026-10-22（初版）→ 2026-10-25（全文重审）→ 2026-11-11（衰变追踪整合评审）
> 更新：2026-11-11 —— 用户对 `manager.py` 与 `mypylib/tracing*.py` 做了较大改动（新增 `tracingDecay.py`，把粒子衰变整合进主循环），本次针对这部分逐行核对，新增 🔴-8/9、🟠-20/21/22、🟡-40/41 等 7 条问题。
> 说明：按严重程度分为 🔴 严重 / 🟠 中等 / 🟡 轻微。每条标注状态：✅ 已修复 / ⚠️ 修复有误或不完整 / ⬜ 待修复 / 🆕 本次新增。

---

## 本次改动评审结论（2026-10-25 更新）

用户对之前的 Bug 做了较大改动，主要涉及：`PrimaryGeneratorAction`（+153 行，能量查找改 `unordered_map`、方向缓存 `DirectionMaskCache`、截止能量）、`MyMagneticField`（`Clone` 补字段、`GetFieldValue` 改 `switch`、全局偶极场重写）、`DetectorConstruction`（`static` 移除、SD4 几何重构）、`TrackerSD` / `SphericalSystemSD`（Time 列改 `lTime` 且单位统一为 `/s`）、`BinaryReader`（判空、上界保护）、`FieldMessenger`（命令修正）。

**总体结论**：
- 上次 27 条问题里，**约一半已正确修复**（🔴-1/2/4/6、🟠-7/8/10/11/12/13/14、🟡-19/22），这是实实在在的进展。
- 但 **🔴-3（`targetrow` 精确匹配）修复方向有误**：把 `E1[i]==Ek` 换成 `unordered_map<double,int>::find`，本质上仍是浮点精确匹配，且 miss 时明确 `targetrow = -1`，随后 `allowedIndices.at(-1)` 必然越界崩溃。**这是当前最严重的阻塞问题**，`run_H` / `run_He` 两个系列宏文件会全部崩溃（详见 🔴-3 / 🔴-7）。
- 部分修复留下了新的小问题（`col` 死变量、`radii.end()-1` 空 vector UB、`parentID > 0` 引发父粒子信息记录失效）。
- `combine/` 与 `Analysis/` 端的旧问题（🟠-15/16/17、🟡-20/23/24/25/27）本次**基本未动**。

---

## 本次改动评审结论（2026-11-11 更新）

用户对 `manager.py` 与 `mypylib/tracing*.py` 做了较大改动，把**粒子衰变**整合进两层递进模拟主循环，主要涉及：

- `mypylib/tracingDecay.py`（**新增**，~222 行）：可衰变粒子（μ/π/K/n 等）的径迹追踪 + 衰变产物生成。状态向量扩展为 7 维（新增弧长 `s`），用 `sVmfp` 事件在粒子飞过抽样衰变弧长时停止积分，再用 `particle`/`particletools`（PDG 质量/衰变道表）+ `pyFANG`（多体相空间衰变）生成衰变产物。
- `mypylib/tracing.py`：`maxstep` 已由旧版 `rigidity/(2*B)` 修正为回旋周期公式 `69.9·R/(β·B)/20`，并新增 `t_eval` 固定输出网格（解决追踪内存爆炸，见 `MEMORY_ISSUE_ANALYSIS.md`）。
- `mypylib/__init__.py`：`__all__` 增加 `tracingDecay`。
- `manager.py`：新增 `ParticleSelect1`（剔除 γ/中微子 + 分离稳定/可衰变粒子）、`Dtracing`（衰变粒子追踪调度）、`DataSetting`（统一结果后处理）；`LoopBasic` 签名扩展为 `(particlelist, dparticlelist, ...)`，把衰变产物并入下一轮追踪。
- `Analysis/mypylib/wrapper.f90`：新增 `rhs_dparticle`（7 维右侧函数）。
- `run.mac`：`/run/beamOn` 100 → 1000。

**总体结论**：
- 🟠-19（`ParticleSelect` 核子条件 `>1e10`）已随 `ParticleSelect1` 的引入修正为 `>1e9`（✅）。
- 新增的 `tracingDecay.py` 衰变分支存在**两个严重问题**（🔴-8/🔴-9）：`DecayProducts` 把字符串名字当 PID 列、且衰变产物动量/质量单位用 GeV 而主流程用 MeV。二者导致衰变产物一旦混入主粒子列表，dtype 变字符串 / 量纲错 1000 倍。**当前只在"中子（几乎不衰变）"下测试过，一旦有 μ/π 等短寿命粒子触发衰变分支就会出错。**
- 另有 `rLow`/`rHigh` 用 `max_events=1` 无法终止积分（🟠-20，用户已在 `sVmfp` 发现同类问题并改用 `terminal=True`，但漏改这两处）、`TFANG(seed=42)` 固定随机种子（🟠-21）等。

---

## 总览

| 级别 | 旧问题 | 已修复 | 修复有误/不完整 | 仍待修复 | 10-25 新增 | 11-11 新增 |
|------|--------|--------|----------------|---------|-----------|-----------|
| 🔴 严重 | 6（🔴-1~6） | 4（1/2/4/6） | 1（🔴-3） | 1（🔴-5 遗留） | 1（🔴-7） | 2（🔴-8/9） |
| 🟠 中等 | 12（🟠-6~17） | 8（7/8/10/11/12/13/14 等） | 0 | 4（6/9/15/16/17） | 2（🟠-18/19） | 3（🟠-20/21/22） |
| 🟡 轻微 | 10（🟡-18~27） | 2（19/22） | 0 | 8（18/20/21/23/24/25/26/27） | 12（🟡-28~39） | 2（🟡-40/41） |

---

## 🔴 严重问题

### 🔴-1. `MyMagneticField::Clone()` 未复制 `fFieldTypeEnum`　【✅ 已修复】

**文件**：`Opposite/src/MyMagneticField.cc:29-43`

第 41 行已补上 `clonedField->fFieldTypeEnum = fFieldTypeEnum;`。多线程下 worker 线程克隆出的磁场对象现在能正确携带场类型枚举，`GetFieldValue` 的 `switch` 不再落到 `default: B=0`。

---

### 🔴-2. `ConstructSDandField` 中 `static` 局部变量 `threadField` 数据竞争　【✅ 已修复】

**文件**：`Opposite/src/DetectorConstruction.cc:443`

```cpp
MyMagneticField *threadField = nullptr;   // ✅ 已去掉 static
```

现在每个 worker 线程各自持有栈上的 `threadField`，线程隔离正确。

---

### 🔴-3. `targetrow` 精确匹配问题——修复方向有误，仍会崩溃　【⚠️ 修复不完整，反而引入必然崩溃】

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:853-897`、头文件 `PrimaryGeneratorAction.hh:140-145`

用户把原来的 `E1[i] == Ek` 精确比较改成了 `unordered_map` 查找：

```cpp
// 构造函数（.hh:67-72）
for (int i = 0; i < 54; i++) {
    fProtonEnergyToRow.emplace(E1[i], i);
    fHeliumEnergyToRow.emplace(E2[i], i);
}

// GeneratePrimaries（.cc:888-897）
if (Pname == "proton") {
    auto it = fProtonEnergyToRow.find(Ek);
    targetrow = (it != fProtonEnergyToRow.end()) ? it->second : -1;   // ❌ miss 时 = -1
} else {
    auto it = fHeliumEnergyToRow.find(Ek);
    targetrow = (it != fHeliumEnergyToRow.end()) ? it->second : -1;   // ❌ miss 时 = -1
}
```

**问题**：
1. `std::unordered_map<double,int>::find(Ek)` 对 `double` 的 key 做的是**精确相等**（哈希 + `==`），本质上和原来的 `E1[i] == Ek` 一模一样。能量只要不是精确等于 bin 值，就 miss。
2. miss 后 `targetrow = -1`，随后传给 `GeneratorDirectionCSV1` → `fDirectionCache.getRandomAllowed(targetrow)` → `allowedIndices.at(-1)`（头文件 `.hh:142`）。`std::vector::at` 传 `-1` 会转成 `SIZE_MAX`，**必然抛 `std::out_of_range`**。
3. 也就是说：原来是"未初始化（不确定行为）"，现在是"**明确 -1 → 必然越界崩溃**"，问题不但没解决，反而更确定地崩。

**触发范围（关键）**：
- `run.mac` 用 `/gun/particle proton` + `/gun/energy 7.244 GeV`（=7244 MeV，恰好命中 `E1[32]=7244`），所以 **`run.mac` 能跑通**。
- 但 `run/run_H/*.mac` 用 `/gun/particle ion` + `/gun/ion 1 1` + 能量如 `5.59 GeV`、`19.5 GeV`、`21.99 GeV`（这些是 `E1` bin 中心值的四舍五入）。此时 `Pname != "proton"`（是 `GenericIon`），走 `else` 查 `fHeliumEnergyToRow`（E2），而 `E2` 里根本没有质子的能量值 → **全部 miss → `targetrow=-1` → 崩溃**。
- `run/run_He/*.mac` 用 `/gun/particle ion` + `/gun/ion 2 4` + 能量如 `12.93 GeV`（=12930，`E2[32]=12928` 附近）、`42.21 GeV`、`54.05 GeV`，同样是四舍五入的 bin 中心 → 全部 miss → 崩溃。

**结论**：`run_H` / `run_He` 两个系列（第一级初级粒子模拟）**必然崩溃**，只有 `run.mac`（精确命中 + 用 `proton` 名）能跑。这就是"测试完成版可以跑通"的假象来源。

**正确修法**：改用最近邻 bin 搜索（见 🔴-7 修改方案），且不要用 `-1` 作为哨兵值。

---

### 🔴-4. `/field/UniStrength` 命令错误地写到偶极场参数　【✅ 已修复】

**文件**：`Opposite/src/FieldMessenger.cc:129-130`

已改为 `fField->SetUniformFieldParameters(strength);`，均匀场强度能正确生效。

---

### 🔴-5. `SetNewValue(Readfile)` 只读 8 列却访问 `fields[8]`　【⚠️ 部分修复，遗留死代码】

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:109-123`

```cpp
int col = 0;
while (col <= 8 && std::getline(ss, cell, ','))   // ⚠️ col 从不自增，条件恒真
{
    fields.push_back(cell);                        // 缺 col++
}
```

**现状**：`col` 初始化为 0，循环体里**没有 `col++`**，所以 `col <= 8` 恒为真，循环靠 `getline` 读完所有列（9 列）后失败退出。因此 `fields[8]` 现在**安全**（不再越界），这是事实上的修复。

**但遗留问题**：`col` 成了死变量，`col <= 8` 这个条件完全无意义、且有误导性（读代码的人会以为只读 9 列）。更稳健的写法是去掉 `col`，读满后访问前判 `fields.size()`（见 🟡-28）。

---

### 🔴-6. `=<` 语法错误，无法编译　【✅ 已修复】

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:110`

已改为 `col <= 8`，可编译。

---

### 🔴-7. 【🆕 本次新增】`targetrow = -1` 导致 `allowedIndices.at(-1)` 越界崩溃

**文件**：
- `Opposite/src/PrimaryGeneratorAction.cc:888-897`（miss 时 `targetrow = -1`）
- `Opposite/include/PrimaryGeneratorAction.hh:142`（`allowedIndices.at(targetRow)`）
- `Opposite/src/PrimaryGeneratorAction.cc:703`（`getRandomAllowed(targetrow)` 调用点）

```cpp
// .hh:140-145
int getRandomAllowed(int targetRow) const {
    const auto &indices = allowedIndices.at(targetRow);   // ❌ targetRow == -1 时抛 out_of_range
    int idx = static_cast<int>(G4UniformRand() * indices.size());
    return indices[idx];
}
```

这是 🔴-3 的直接后果，单独列一条以强调：任何"能量不在 E1/E2 精确 bin 上"的 Primary 事件都会走到这里崩溃。**修复 🔴-3 时需一并处理本问题**（详见下方修改方案）。

### 🔴-8. 【🆕 2026-11-11】`DecayProducts` 把字符串名字当 PID 列，且引用错误的衰变道

**文件**：`Opposite/mypylib/tracingDecay.py:143-172`

```python
daughter_names = [ch[1] for ch in channels]                 # 所有衰变道的末态名列表（list of list）
chosen_daughters = random.choices(daughter_names, weights=weights, k=1)[0]   # 选中的那一条
...
charges = np.array([particle_data.charge(name) for name in chosen_daughters])
daughters_array = np.column_stack([px, py, pz, Ek, daughter_names[0], charges, mass])
#                                                      ^^^^^^^^^^^^^^^^^ ❌
```

`daughter_names[0]` 是**第一条衰变道**的末态名列表（`list[str]`），既不是选中的 `chosen_daughters`，也不是单个 PDG 编码。这里作为第 5 列（PID 列）写入，会产生两个后果：

1. **类型污染**：`np.column_stack` 里混入 `list[str]`，只要第一条衰变道的末态粒子数与选中道不同，就直接抛"维度不匹配"异常；若恰好相同，整列 `daughters_array` 会被提升为字符串 dtype。之后 `manager.py:260` 的 `np.vstack((particlelist, splist))` 会把整个粒子列表变成字符串，下一轮 `RotationModel.xyz2rthetaphi` / 数值运算必然崩溃。
2. **PID 错误**：即使改成 `chosen_daughters`，存的是字符串名字（如 `"pi+"`）而非数字 PDG（`211`），下游 `ParticleSelect1` 的 `PID > 1e9` 等数值比较也无法工作。

**修法**：`DecayProducts` 应返回每个末态粒子的**数字 PDG id**，即

```python
daughter_pdg = np.array([particle_data.pdgid(name) for name in chosen_daughters])
daughters_array = np.column_stack([px, py, pz, Ek, daughter_pdg, charges, mass])
```

### 🔴-9. 【🆕 2026-11-11】衰变产物单位不一致（GeV vs MeV），混入主列表后量纲错 1000 倍

**文件**：`Opposite/mypylib/tracingDecay.py:164-170, 216-220`

`DecayProducts` 里 `px, py, pz`（动量）、`Ek`（动能）、`mass`（质量）全部来自 `pyFANG` 的四动量，单位是 **GeV**（自然单位）。但主流程 `manager.py` 的粒子数组约定是：

```python
# 0,1,2 是 x,y,z(km)；3,4,5 是 vx,vy,vz 单位 MeV；6 是 E(MeV)；7 PID；8 t(s)；9 charge；10 mass(MeV)
```

衰变产物 `plist`（`tracingDecay.py:218-220`）把 GeV 的 `px/py/pz/Ek/mass` 直接写进第 3~6、10 列，`manager.py:260` 又把它 `vstack` 进 MeV 单位的 `particlelist`。下一轮 `tracing.main` 里 `pGeV = norm(monento)/1e3` 会把这些 GeV 值**再当 MeV 除 1e3**，动量/能量/质量整体偏小 1000 倍。

**修法**：在 `DecayProducts` 或 `main()` 里统一换算成 MeV（动量、动能、质量均 `×1000`），与主流程保持一致。

---

## 🟠 中等问题

### 🟠-6. `SelectDirection` 二分查找使用未初始化变量 `c`　【⬜ 待修复（legacy，无影响）】

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:218-220`

`int a = 0, b = 360, c;` 中 `c` 仍未初始化。该函数与 `SelectDirection2` 已不被主流程调用（`GeneratePrimaries` 只走 `GeneratorDirectionCSV1`），属死代码，建议删除而非修复。

---

### 🟠-7. 打开文件失败后 `fclose(NULL)` + 使用未初始化数组　【✅ 已修复】

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:141-145 / 192-196 / 322-326`

三个函数（`GeneratorEnergyFromSpectrum`、`SelectDirection`、`SelectDirection2`）现在都在 `if (!fp)` 分支里提前 `return`，不再 `fclose(NULL)`。

---

### 🟠-8. `SphericalSystemSDinAir/Orbit` 构造函数重复 `push_back` 使 `fRadii` 翻倍　【✅ 已修复，遗留空 vector 隐患】

**文件**：`Opposite/src/SphericalSystemSDinAir.cc:29-31`、`SphericalSystemSDinOrbit.cc:28-30`

```cpp
: G4VSensitiveDetector(name), fRadii(radii.begin(), radii.end() - 1)   // ✅ 不再 push_back 两次
```

重复 `push_back` 已消除。但新写法 `radii.end() - 1` 在 `radii` 为空时是未定义行为（见 🟡-39）。当前调用 `fLayRmininAir` 有 11088 个元素，不触发。

---

### 🟠-9. `DetectorConstruction::Construct` 检查了错误的 vector　【⬜ 待修复】

**文件**：`Opposite/src/DetectorConstruction.cc:326`

```cpp
if (static_cast<int>(logicSphereSDlayerinOrbit.size()) != 0)   // ❌ 仍检查永远为空的成员
```

实际填充的是 `logicSphereSDlayerinAir`（第 307/358 行）。重入检查失效，`Construct` 被重复调用会重复创建 SD4 体积导致几何重叠。

---

### 🟠-10. `FieldMessenger::SetNewValue` 两个相同的 `else if` 分支　【✅ 已修复】

**文件**：`Opposite/src/FieldMessenger.cc:94-134`

现在 `SetNewValue` 有 4 个条件互不相同的分支（`fEnableFieldCmd` / `fSetFieldTypeCmd` / `fSetDipoleFieldParametersCmd` / `fSetUniformFieldParametersCmd`），重复分支已消除。

---

### 🟠-11. Time 列读取未初始化内存　【✅ 已修复（改用 lTime）】

**文件**：`Opposite/src/SphericalSystemSDinAir.cc:150-151 / 173`、`SphericalSystemSDinOrbit.cc:136-137 / 160`、`TrackerSD.cc:108-109 / 131`

原 `GetTime()` 读未初始化的 `fTime`，现已改为 `SetlTime(GetLocalTime())` + `GetlTime()`，而 `flT` 有默认值 `0.`（头文件 `SphericalSystemHitinAir.hh:88`、`TrackerHit.hh:88`）。不再读垃圾值。

---

### 🟠-12. 同一 ntuple 内 Time 单位不一致　【✅ 已修复（统一为秒）】

**文件**：`TrackerSD.cc:165 / 203`、`SphericalSystemSDinAir.cc:209`、`SphericalSystemSDinOrbit.cc:196`

现在四处全部用 `timeOfL / s`（秒），单位已统一。但注意：语义从"全局到达时间"变成了"local time"，是否影响下游分析需确认（见 🟡-37）。

---

### 🟠-13. `BinaryReader::readROOTSpecificBranch` 空指针 + 悬垂临时字符串　【✅ 已修复】

**文件**：`Opposite/src/BinaryReader.cc:89-112`

已加 `if (!file || file->IsZombie())`、`if (!allowed)` 判空，`branchName` 用具名 `std::string` 避免悬垂。该函数为 legacy，主流程已用 `DirectionMaskCache` 取代。

---

### 🟠-14. `readCSVinAll` 无列数上界　【✅ 已修复】

**文件**：`Opposite/src/BinaryReader.cc:31`

已加 `&& col < 13963` 上界保护。

---

### 🟠-15. `ChainFilter` 构造函数 TFile 泄漏 + `delete currentTree` 双重所有权　【⬜ 待修复】

**文件**：`Opposite/combine/src/ChainFilter.cc:11-24, 46-50`

```cpp
TFile *f = TFile::Open(filename);            // ❌ 局部变量，泄漏，且未判空
currentTree = (TTree *)f->Get(treename);     // ❌ 未判空
...
delete currentTree;                           // ❌ 双重释放（TFile 也拥有该树）
```

`combine/` 端本次未改动，问题原样保留。

---

### 🟠-16. `manager.py` 读取了 `count` 却未用于切片　【⬜ 待修复】

**文件**：`Opposite/manager.py:84`

```python
raw_data = result.stderr[4:]   # ❌ 仍没用 count，直接取全部剩余字节
```

`count`（第 72-73 行）算了却没用。一旦 stderr 混入其它输出，二进制解析被污染。

---

### 🟠-17. `FluxCalculationOn_T_E` 调用 `AreaFilter` 缺少 `model` 参数　【⬜ 待修复】

**文件**：`Analysis/mypylib/FluxCalculation.py:61`

```python
idxInA = AreaFilter(xyz, xyzlocal, ARange[3])   # ❌ 仍只传 3 个参数 → TypeError
```

（同一文件第 94/129/165 行都传了 4 参数，唯独第 61 行漏了。）

---

### 🟠-18. 【🆕 本次新增】`TrackingAction` 父粒子信息记录逻辑错误，pPID/ParentParticalName 几乎恒为默认值

**文件**：`Opposite/src/TrackingAction.cc:29-47`、`SphericalSystemSDinAir.cc:127-139`（消费方）

本次把 `if (parentID >= 0)` 改成 `if (parentID > 0)`（修 🟡-19），但暴露/引入了逻辑缺陷：

```cpp
// TrackingAction.cc:35-45
if (parentID > 0) {                       // 只对"父粒子也是次级"的粒子才记录
    if (localMap->find(trackID) == localMap->end()) {
        (*localMap)[trackID] = {def->GetParticleName(), def->GetPDGEncoding()};  // 存的是"自己"
    }
}
```

- map 的语义是：`key = trackID`，`value = 该粒子自己的名字/PDG`；消费方用 `find(parentID)` 查父粒子。
- 但 `if (parentID > 0)` 导致 **primary 的直接子代（parentID==0）不被记录**。于是：
  - primary（ID=0）→ 直接次级 A（ID=1, parentID=0）→ 孙次级 B（ID=2, parentID=1）。
  - A 因 `parentID==0` 不满足 `>0`，`map[1]` 不存在；B 打中探测器时查 `find(1)` 找不到。
- **结果**：除了极少数"父粒子本身也是次级且其父也是次级"的情况，`pPID` / `ParentParticalName` 两列几乎恒为默认值 `-1` / `"nan"`，这个诊断功能实际失效。

**正确修法**：存所有粒子（去掉 `>0` 判断，或对 primary 也记录），消费方查 `parentID` 时若 `parentID==0` 单独标记为"父是初级粒子"。

---

### 🟠-19. 【🆕 本次新增】`ParticleSelect` 核子选择条件 `PID>1e10` 写错，漏选所有原子核　【✅ 已修复（随 `ParticleSelect1` 引入改为 `>1e9`）】

**文件**：`Opposite/manager.py:93`

```python
indx=np.where((PID==11)|(PID==-11)|(PID==2212)|(PID==-2212)|(PID>1e10)|(PID<-1e10))
```

原子核的 PDG 编码规则为 `±(1e9 + Z*1e4 + A*10 + I)`，例如 α（氦-4）是 `1000020040`，约 `1.0e9`。所以 `PID > 1e10`（即 `> 10^10`）**恒为假**，`PID < -1e10` 同理。结果是**没有任何原子核会被选入**（质子/电子靠 `==2212/==11` 能选到，但氦等重核全漏）。

项目要算氦（He）通量，这会直接漏掉氦核。应改为 `abs(PID) > 1e9`（或 `(PID > 1e9) | (PID < -1e9)`）。此问题是既有代码（非本次引入），但原报告遗漏，本次补上。当前 `ParticleSelect1` 已改为 `>1e9`，已修复。

### 🟠-20. 【🆕 2026-11-11】`tracingDecay.py` 的 `rLow`/`rHigh` 用 `max_events=1`，无法终止积分

**文件**：`Opposite/mypylib/tracingDecay.py:72, 87`

```python
rLow.max_events = 1             # ❌ 若撞上稠密大气则积分"可以"终止（实际不会）
rHigh.max_events = 1            # ❌ 若已逃离地磁场则积分"可以"终止（实际不会）
```

scipy 的 `max_events` 语义是「找到的事件数**超过**该值才终止积分」，`max_events=1` 意味着要触发**第 2 次**才停；对"只会发生一次"的事件（撞一次大气、逃逸一次）**不会停止**，积分会一直跑到 `tspan` 结束（100 s）。

用户其实已经发现了这个问题——`sVmfp`（第 78-79 行）注释明确写了「`max_events=1` 有个 bug，触发次数必须大于给定次数才停，对只发生一次的事件无法停止」，并把它改成了 `terminal=True`。但 `rLow`/`rHigh` 两处**漏改**了，仍是 `max_events=1`。

后果：撞大气（rLow）或逃逸（rHigh）的粒子不会停止积分，会继续跑到 100 s，导致（1）性能骤降；（2）物理错误（撞入稠密大气后不应继续追踪）。对比 `tracing.py` 中这两个事件都是 `terminal=True`。

**修法**：与 `tracing.py` 对齐，改回

```python
rLow.terminal = True
rHigh.terminal = True
```

### 🟠-21. 【🆕 2026-11-11】`TFANG(seed=42)` 固定随机种子，衰变产物存在 Monte Carlo 相关性

**文件**：`Opposite/mypylib/tracingDecay.py:159`

```python
gen = TFANG(seed=42)
```

每次调用 `DecayProducts` 都新建一个 `seed=42` 的 `TFANG`，随机数序列起点固定。同一父粒子（相同四动量）会生成**完全相同**的衰变产物；不同事件之间也存在随机数相关性，造成系统性偏差。应使用不重复的随机种子（如基于进程/粒子索引派生，或全局 `np.random.SeedSequence` 每次 `spawn`）。

### 🟠-22. 【🆕 2026-11-11】`datasplit` 的 `result[0][0:6]` 是对行切片而非列切片

**文件**：`Opposite/mypylib/tracingDecay.py:101-104`

```python
if len(result[0]) == 0:
    return (i, np.array([]), 0, result[2][:,0:6], result_t[2])
else:
    return (i, result[0][0:6], result_t[0], result[2][:,0:6], result_t[2])
#                ^^^^^^ ❌ 意图是取第一个事件的前 6 分量（剥离弧长 s），但这是对"行"切片
```

`result[0]` 是 `sol.y_events[0]`（rLow 事件数组，形状 `(n_events, 7)`）。`result[0][0:6]` 对**第 0 维（行）**切片，本意应是 `result[0][0, :6]`（取第一个事件的前 6 列，去掉弧长 `s`）。

当前因为 `max_events=1` 且单次事件 `n_events=1`，`result[0][0:6]` 恰好返回完整的 `(1,7)`（含 `s`），下游 `DataSetting` 只取 `res[0,0:3]`/`res[0,3:6]`，`s` 被忽略，所以**碰巧能用**。但语义错误：一旦 `n_events > 1`（比如改成 `terminal=True` 后仍可能多次穿越边界），`result[0][0:6]` 会取前 6 行而非第一个事件，丢失数据。

**修法**：`result[0][0, :6]`（或 `result[0][0][0:6]`）。

---

## 🟡 轻微问题 / 潜在风险

### 🟡-18. `SteppingAction::n` 成员变量跨 track 计数串扰　【⬜ 待修复】

**文件**：`Opposite/include/SteppingAction.hh:28`、`Opposite/src/SteppingAction.cc:39-44`

`n` 仍是类成员，被所有 track 共享，不同 track 之间连续步数计数互相干扰，`n>100` 的"卡住判定"可能误判。

### 🟡-19. `TrackingAction::PreUserTrackingAction` 中 `if (parentID >= 0)` 恒真　【✅ 已修复，但引发 🟠-18】

**文件**：`Opposite/src/TrackingAction.cc:35`

已改为 `parentID > 0`。但如 🟠-18 所述，这导致父粒子信息记录失效，需一并重设计。

### 🟡-20. `combine/Combine.cc` 内存泄漏　【⬜ 待修复】

**文件**：`Opposite/combine/src/Combine.cc:28/29/73/102/103`、`Opposite/combine/dataAnalyseTest.cc:32/84`

`CloneTree()` 返回的临时树未 `delete`，`new mylib::Combine()` 不释放。本次未改动。

### 🟡-21. `DetectorConstruction` 中 `fieldMgr` 未注册删除　【⬜ 待修复（用户有意为之，需评估）】

**文件**：`Opposite/src/DetectorConstruction.cc:460-470`

```cpp
G4FieldManager *fieldMgr = new G4FieldManager(threadField);
...
// G4AutoDelete::Register(fieldMgr);  // 用户注释：加会涉及主线程删磁场而子线程还在等待命令
```

每个 worker 线程泄漏一组 `fieldMgr` + `equation` + `stepper` + `chordFinder`（32 线程 = 32 组）。用户注释的理由对 `G4AutoDelete` 的理解可能有误——`G4AutoDelete::Register` 在 GEANT4 里是线程安全的（有线程局部删除机制），官方示例正是这样用的。建议重新评估，至少可改为在 worker 线程注册。

### 🟡-22. `tracing.py` 中 `rigidity = pGeV / abs(charge)` 潜在除零　【✅ 已修复】

**文件**：`Opposite/mypylib/tracing.py:168`

```python
rigidity = pGeV / abs(charge) if charge != 0 else pGeV   # ✅ 已加除零保护
```

注意：`charge==0` 时取 `pGeV` 只是兜底（物理上中性粒子 rigidity 应为无穷大），但由于 `ParticleSelect` 只选带电粒子，此分支实际不触发。可进一步改成 `float('inf')` 更严谨。

### 🟡-23. `manager.py` 注释块中第二处 `particle_dtype` 字段顺序不同　【⬜ 待修复（风险低）】

**文件**：`Opposite/manager.py:305-309`（注释块，`charge` 与 `t` 颠倒）

生效版（`:76-81`）字段顺序正确。注释块已废弃，但建议删除以免误用。

### 🟡-24. `trac2Orbit_vec` 粒子背向球面运动时取到负根　【⬜ 待修复】

**文件**：`Analysis/mypylib/ToHigherOrbit.py:60`

```python
t = (-b + np.sqrt(disc)) / (2.0 * a_safe)   # ❌ 仍只取较大根
```

本次未改动。

### 🟡-25. `FluxCalculationOn_T_P` 与 `_T_P_E` 方向符号不一致　【⬜ 待修复】

**文件**：`Analysis/mypylib/FluxCalculation.py:109`（取负）vs `:146`（不取负）

本次未改动。

### 🟡-26. `str0` 文件名依赖经纬度整数度取整　【⬜ 待修复】

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:899`

```cpp
std::string str0 = "../ctable/all_" + std::to_string(int(std::round(lon / deg))) + "_" + ...;
```

`lon/lat` 非整数度时构造的文件名不存在，`fDirectionCache.load` 返回 false 后静默返回 `(0,0,0)` 方向。

### 🟡-27. `tracing.main` 的 `n` / `thread` 参数未使用　【⬜ 待修复】

**文件**：`Opposite/mypylib/tracing.py:159`

`def main(Xinit, monento, otherpar, i, n, thread)` 中 `n`、`thread` 仍未使用（仅注释块引用过）。

### 🟡-28. 【🆕 本次新增】`Readfile` 解析中 `col` 是死变量，`col <= 8` 条件无意义

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:109-113`

见 🔴-5。`col` 从不自增，`col <= 8` 恒真，靠 `getline` 失败退出。建议改为：

```cpp
while (std::getline(ss, cell, ',')) { fields.push_back(cell); }
if (fields.size() > 8) { p.time = std::stoi(fields[8]) * s; }   // 访问前判列数
```

### 🟡-29. 【🆕 本次新增】`cell[0] == '1'` 对空字符串越界

**文件**：`Opposite/include/PrimaryGeneratorAction.hh:125`、`Opposite/src/BinaryReader.cc:72`

若某单元格为空（如行尾多一个逗号），`cell[0]` 访问空 `std::string` 的 `[0]` 是未定义行为。建议先判 `!cell.empty()`。

### 🟡-30. 【🆕 本次新增】能量阈值三处不一致

**文件**：
- `Opposite/src/ActionInitialization.cc:33`（`stacking->SetEnergyThreshold(15.0*MeV)`）
- `Opposite/src/MyStackingAction.cc:13`（构造默认 `5.0*MeV`，被上面覆盖）
- `Opposite/src/TrackerSD.cc:142 / 180`（SD1/SD2 截止 `5.*MeV`）
- `Opposite/src/SphericalSystemSDinAir.cc:192` / `SphericalSystemSDinOrbit.cc:179`（`30.*MeV`）

提交信息说"截止能量改为 5 MeV"，但实际：stacking 阈值被覆盖成 **15 MeV**、SD1/SD2 是 **5 MeV**、Air/Orbit 仍是 **30 MeV**。三处不统一。若意图是统一 5 MeV，则 `ActionInitialization.cc:33` 和 Air/Orbit 的 30 MeV 是遗漏。

### 🟡-31. 【🆕 本次新增】`fFieldType` 默认值拼写错误 `"uiform"`

**文件**：`Opposite/include/MyMagneticField.hh:49`

```cpp
G4String fFieldType = "uiform";   // ❌ 应为 "uniform"
```

影响 `FieldMessenger.cc:124` 里 `GetFieldType() != "uniform"` 的判断（未设置类型时会误报警告）。默认场类型实际由 `fFieldTypeEnum = kNone` 决定，故无功能影响。

### 🟡-32. 【🆕 本次新增】`/field/SetType` 的候选值 `quadrupole` 无对应实现

**文件**：`Opposite/src/FieldMessenger.cc:44`、`Opposite/include/MyMagneticField.hh:30-35`

命令 candidates 包含 `"quadrupole"`，但 `SetFieldType` 只处理 `uniform/dipole/globaldipole`，`FieldType` 枚举也没有 `quadrupole`。用户若设 `quadrupole`，`fFieldType` 会被写成 `"quadrupole"` 但 `fFieldTypeEnum` 保持不变（仍是旧类型），字符串与实际场类型脱节。建议移除该候选值或补实现。

### 🟡-33. 【🆕 本次新增】Hit 类多个成员未初始化

**文件**：`Opposite/include/SphericalSystemHitinAir.hh:73-75, 87, 89, 96-98, 101`、`SphericalSystemHitinOrbit.hh`、`TrackerHit.hh:87, 89`

`fLayerID`、`fEnergyDeposit`、`fPosition`、`fTime`、`fpT`、`fRadius/fTheta/fPhi`、`fGlobalTime` 等无默认值。当前 `SetTime/SetpTime` 被注释、这些成员多数未被读取，故暂不触发；但属于隐患，一旦有人启用 `GetTime()` 即读垃圾值。建议统一给默认值。

### 🟡-34. 【🆕 本次新增】`fCachedCosColat` 等 5 个缓存成员声明未使用

**文件**：`Opposite/include/PrimaryGeneratorAction.hh:150-154`

`fCachedCosColat` / `fCachedSinColat` / `fCachedCosLon` / `fCachedSinLon` / `fTrigCached` 声明了但在 `.cc` 中从未使用（`GeneratorDirectionCSV1` 里仍直接调 `VectorZenithinGTOD`）。属未完成的优化残留，建议删除或真正启用。

### 🟡-35. 【🆕 本次新增】`dtest` 死变量

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:915`

```cpp
G4double dtest = pos.unit().dot(dpp.unit());   // 计算后未使用
```

原报告末尾提过，本次正式列入。

### 🟡-36. 【🆕 本次新增】`layer` 成员被局部变量 shadow，`SetAtmLayer` 命令无效

**文件**：`Opposite/include/DetectorConstruction.hh:67`（`G4int layer = 1000;`）、`Opposite/src/DetectorConstruction.cc:208`（`int layer = 208;`）

`Construct()` 里局部 `int layer = 208` 遮蔽了成员 `layer`（默认 1000）。后果：
- 大气层数由局部 `layer=208` 固定决定，`/Geo/` 里若暴露了设置层数的命令（`SetAtmLayer`）不会生效。
- 第 87 行 `G4int layer_loop = GetAtmLayer();` 取到的是成员 `1000`，但后续根本没被使用（死变量）；而 `AtmLayer[209]` 数组只有 209 个元素，若有人按 `layer_loop=1000` 循环会越界。

建议统一：要么删除局部 `layer`、让成员 `layer` 与 `AtmLayer` 数组长度一致（208），要么删除 `SetAtmLayer` 及 `layer_loop`。

### 🟡-37. 【🆕 本次新增】Time 列语义从"全局时间"改为"local time"

**文件**：`TrackerSD.cc:109 / 165 / 203`、`SphericalSystemSDinAir.cc:151 / 209`、`SphericalSystemSDinOrbit.cc:137 / 196`

本次把 `GetGlobalTime()`（被注释）换成了 `GetLocalTime()`。物理上"粒子到达探测器的时刻"应使用**全局时间**（`GetGlobalTime`），local time 是粒子自身的局部时（对速度接近 c 的粒子，二者差异可观）。若下游用 Time 列做到达时间/飞行时间分析，此改动会引入系统偏差。请确认是刻意还是误改。

### 🟡-38. 【🆕 本次新增】`SteppingAction` 中 `dE` 除零潜在

**文件**：`Opposite/src/SteppingAction.cc:37`

```cpp
G4double dE = step->GetTotalEnergyDeposit() / step->GetTrack()->GetKineticEnergy();
```

动能为 0 的粒子（理论上静止粒子不会 stepping，但边界/数值情况下可能）会除零。建议加 `if (ke > 0)` 保护。

### 🟡-39. 【🆕 本次新增】`radii.end() - 1` 空 vector 未定义行为

**文件**：`Opposite/src/SphericalSystemSDinAir.cc:31`、`SphericalSystemSDinOrbit.cc:30`

`fRadii(radii.begin(), radii.end() - 1)` 在 `radii` 为空时 `end() - 1` 是 UB（当前 `fLayRmininAir` 有 11088 元素故不触发）。建议 `radii.empty() ? ... : ...` 或直接 `fRadii(radii)`（若不再需要去掉最后一个元素）。

### 🟡-40. 【🆕 2026-11-11】`DecayProducts` 未处理"无衰变道"的粒子

**文件**：`Opposite/mypylib/tracingDecay.py:143-147`

```python
channels = particle_data.decay_channels(pid)
weights = [ch[0] for ch in channels]
chosen_daughters = random.choices(daughter_names, weights=weights, k=1)[0]   # channels 为空时抛异常
```

若某粒子在 `PYTHIAParticleData` 中没有衰变道（`channels == []`，例如被误分类进可衰变列表的长寿命/稳定粒子），`random.choices([], weights=[])` 会抛 `IndexError`。虽然 `ParticleSelect1` 已筛掉稳定粒子，但 `dparticlelist` 里仍可能混入无衰变数据的粒子，建议加判空保护（无衰变道时按稳定粒子处理，`return` 空产物或直接走 `tracing.main`）。

### 🟡-41. 【🆕 2026-11-11】`tracingDecay.py` 调试 `print` 残留

**文件**：`Opposite/mypylib/tracingDecay.py:148, 153, 202, 214`

```python
print(f"随机选中的衰变道: eta -> {chosen_daughters}")
print(f"末态粒子质量 (GeV): {daughter_masses}")
print("D Sol function finished!")
print("Error!!! Energy is not conserved!!", ...)
```

`DecayProducts` 里的逐粒子 `print`（每个衰变事件都会打印）在 32 进程并行 + 集群批量运行时会造成大量日志刷屏，且 `"eta -> ..."` 是硬编码的示例文本（实际粒子不一定是 eta）。建议改为 `logging`（模块里已 `import logging`，但未使用）或删除；能量不守恒告警保留为 `logging.warning`。

---

## 建议的修复优先级

1. **🔴-3 + 🔴-7（最高，阻塞）**：`targetrow` 精确匹配 → 最近邻搜索；miss 时不得返回 `-1`（或 `getRandomAllowed` 内先判 `targetRow` 越界）。否则 `run_H` / `run_He` 全部崩溃。
2. **🔴-8 + 🔴-9（衰变分支，阻塞）**：`DecayProducts` 的 PID 列改用数字 PDG、衰变产物单位统一为 MeV。否则一旦 μ/π 等短寿命粒子触发衰变分支，主粒子列表 dtype/量纲被破坏。
3. **🟠-20**：`tracingDecay.py` 的 `rLow`/`rHigh` 改回 `terminal=True`（撞大气/逃逸即停），否则性能骤降 + 物理错误。
4. **🟠-18**：父粒子信息记录逻辑重设计（影响 pPID/ParentParticalName 诊断列的正确性）。
5. **🟡-30**：统一能量阈值（stacking 15 / SD 5 / Air-Orbit 30 三处不一致）。
6. **🟠-16 / 🟠-17**：Python 端解析与调用错误（虽不影响 C++ 编译，但影响 Secondary 流程与通量分析）。
7. 其余按需（🟠-9/21/22、🟡-18/20/21/23/24/25/26/27、🟡-28~41）。

> 注：🟠-19（核子选择条件）已随 `ParticleSelect1` 改为 `>1e9`，标记 ✅ 已修复。

---

## 🔧 修改方案（Fix Plans）

> 覆盖所有「待修复」「修复有误」「本次新增」的问题，按优先级排列。改动后请重新编译 `test02` 并跑一个宏文件验证（**尤其要验证 `run_H` / `run_He` 不再崩溃**）。

### 修复优先级

1. 🔴-3 + 🔴-7（`targetrow` 崩溃，**必须先改**）
2. 🔴-8 + 🔴-9（衰变产物 PID/单位）
3. 🟠-20（`rLow`/`rHigh` 终止积分）
4. 🟠-18（父粒子信息记录）
5. 🟡-30（能量阈值统一）
6. 🟠-16 / 🟠-17（Python 端）
7. 其余按需。

---

#### 🔴-3 + 🔴-7. `targetrow` 最近邻搜索 + `getRandomAllowed` 越界保护

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:853-897`、`Opposite/include/PrimaryGeneratorAction.hh:140-145`

```cpp
// ❌ 现状：miss 时 targetrow = -1，且精确匹配几乎必 miss
auto it = fProtonEnergyToRow.find(Ek);
targetrow = (it != fProtonEnergyToRow.end()) ? it->second : -1;

// ✅ 修正：最近邻 bin 搜索（不再用 unordered_map 精确匹配）
int targetrow = 0;
double mindiff = 1e300;
const auto &Earr = (Pname == "proton") ? E1 : E2;   // 若用 ion 名，需按 Z/A 再区分 E1/E2
for (int i = 0; i < 54; i++) {
    double diff = std::fabs(Earr[i] - Ek);
    if (diff < mindiff) { mindiff = diff; targetrow = i; }
}
```

> 注意：`run_H` / `run_He` 用的是 `/gun/particle ion`，`GetParticleName()` 不是 `"proton"`，当前会走 `E2` 分支。即使改用最近邻搜索，也要**按粒子种类（质子/氦）正确选择 E1/E2**，例如根据 `GetParticleDefinition()->GetPDGEncoding()` 或 `GetParticleMass()` 判断，而不是只看名字。

同时给 `getRandomAllowed` 加防御（双保险）：

```cpp
int getRandomAllowed(int targetRow) const {
    if (targetRow < 0 || targetRow >= (int)allowedIndices.size()) return 0;   // ✅ 防越界
    const auto &indices = allowedIndices.at(targetRow);
    if (indices.empty()) return 0;   // ✅ 防空行除零/越界
    int idx = static_cast<int>(G4UniformRand() * indices.size());
    return indices[idx];
}
```

---

#### 🟠-18. 父粒子信息记录逻辑重设计

**文件**：`Opposite/src/TrackingAction.cc:29-47`

```cpp
// ❌ 现状：只对 parentID > 0 的粒子存自己，primary 直接子代漏存
// ✅ 修正：存所有粒子（包括 primary），消费方区分 parentID == 0
void TrackingAction::PreUserTrackingAction(const G4Track *track) {
    auto *localMap = GetThreadLocalMap();
    G4int trackID = track->GetTrackID();
    if (localMap->find(trackID) == localMap->end()) {
        const G4ParticleDefinition *def = track->GetDefinition();
        (*localMap)[trackID] = {def->GetParticleName(), def->GetPDGEncoding()};
    }
}
```

消费方（`TrackerSD.cc:85` / `SphericalSystemSD*.cc:127`）查父时，若 `parentID == 0` 说明父是初级粒子，可自行决定 `pPID/ParentParticalName` 填什么（如填 primary 本身或保持默认）。

---

#### 🟠-19. `ParticleSelect` 核子选择条件修正

**文件**：`Opposite/manager.py:93`

```python
# ❌ indx = np.where((PID==11)|(PID==-11)|(PID==2212)|(PID==-2212)|(PID>1e10)|(PID<-1e10))
# ✅ 原子核 PDG 是 ~1e9 量级
indx = np.where((PID==11)|(PID==-11)|(PID==2212)|(PID==-2212)|(PID>1e9)|(PID<-1e9))
```

#### 🔴-8 + 🔴-9. `DecayProducts` PID 数字化 + 单位统一（含 🟡-40 判空、🟠-21 seed）

**文件**：`Opposite/mypylib/tracingDecay.py:134-172`

```python
def DecayProducts(pid, momentum):
    particle_data = PYTHIAParticleData()
    mGeV = particle_data.mass(pid)
    channels = particle_data.decay_channels(pid)
    if not channels:                      # 🟡-40：无衰变道 → 按稳定粒子处理
        return np.array([])
    weights = [ch[0] for ch in channels]
    daughter_names = [ch[1] for ch in channels]
    chosen = random.choices(daughter_names, weights=weights, k=1)[0]
    daughter_masses = [particle_data.mass(name) for name in chosen]
    daughter_pdg   = np.array([particle_data.pdgid(name) for name in chosen])  # 🔴-8：数字 PDG
    charges        = np.array([particle_data.charge(name) for name in chosen])

    parent_momentum = np.append(momentum, mGeV)
    gen = TFANG(seed=rng_seed)            # 🟠-21：rng_seed 由全局 SeedSequence 每次 spawn，勿固定 42
    gen.SetDecay(parent_momentum, np.array(daughter_masses))
    gen.Generate()
    daughters_4mom = np.array(gen.GetDecays(0))
    px, py, pz, mass = daughters_4mom.T
    energy = np.sqrt(px**2 + py**2 + pz**2 + mass**2)
    Ek = energy - mass
    # 🔴-9：px/py/pz/Ek/mass 统一 ×1000 换算成 MeV，与主流程粒子数组一致
    daughters_array = np.column_stack([px*1000, py*1000, pz*1000, Ek*1000,
                                       daughter_pdg, charges, mass*1000])
    return daughters_array
```

#### 🟠-20. `rLow`/`rHigh` 改回 `terminal=True`

**文件**：`Opposite/mypylib/tracingDecay.py:72, 87`

```python
rLow.terminal  = True    # 撞入稠密大气即停（原 max_events=1 对单次事件不生效）
rHigh.terminal = True    # 逃离地磁场即停
```

#### 🟠-22. `datasplit` 列切片修正

**文件**：`Opposite/mypylib/tracingDecay.py:104`

```python
# ❌ return (i, result[0][0:6], ...)            # 对行切片，语义错误
# ✅ 取第一个事件的前 6 分量（剥离弧长 s）
return (i, result[0][0, :6], result_t[0], result[2][:, 0:6], result_t[2])
```

---

#### 🟡-30. 能量阈值统一

**文件**：`Opposite/src/ActionInitialization.cc:33`、`Opposite/src/SphericalSystemSDinAir.cc:192`、`Opposite/src/SphericalSystemSDinOrbit.cc:179`

按"统一 5 MeV"的意图：

```cpp
// ActionInitialization.cc:33
stacking->SetEnergyThreshold(5.0*MeV);          // 或删除此行，让构造默认 5 MeV 生效

// SphericalSystemSDinAir.cc:192  /  SphericalSystemSDinOrbit.cc:179
if (kE >= 5.*MeV) { ... }                        // 原来 30.*MeV
```

---

#### 🟠-16. `manager.py` 用 `count` 切分数据

**文件**：`Opposite/manager.py:84`

```python
# ❌ raw_data = result.stderr[4:]
# ✅ raw_data = result.stderr[4:4 + count * particle_dtype.itemsize]
```

---

#### 🟠-17. `AreaFilter` 补 `model` 参数

**文件**：`Analysis/mypylib/FluxCalculation.py:61`

```python
# ❌ idxInA = AreaFilter(xyz, xyzlocal, ARange[3])
# ✅ idxInA = AreaFilter(xyz, xyzlocal, ARange[3], 0)
```

---

#### 🟡-28. `Readfile` 解析去掉死变量 `col`，访问前判列数

**文件**：`Opposite/src/PrimaryGeneratorAction.cc:109-123`

```cpp
while (std::getline(ss, cell, ',')) { fields.push_back(cell); }
p.position = G4ThreeVector(std::stod(fields[0])*km, std::stod(fields[1])*km, std::stod(fields[2])*km);
p.direction = G4ThreeVector(std::stod(fields[3]), std::stod(fields[4]), std::stod(fields[5]));
p.kineticEnergy = std::stod(fields[6]) * MeV;
p.pdgCode = std::stoi(fields[7]);
if (fields.size() > 8) p.time = std::stoi(fields[8]) * s;   // ✅ 防列数不足
```

---

#### 🟡-29. `cell[0]` 空字符串防护

**文件**：`Opposite/include/PrimaryGeneratorAction.hh:125`、`Opposite/src/BinaryReader.cc:72`

```cpp
if (!cell.empty() && cell[0] == '1') { row.set(col); indices.push_back(col); }
```

---

#### 🟡-31. `fFieldType` 默认拼写修正

**文件**：`Opposite/include/MyMagneticField.hh:49`

```cpp
G4String fFieldType = "uniform";   // ❌ 原来 "uiform"
```

---

#### 🟡-33. Hit 类成员补默认值

**文件**：`SphericalSystemHitinAir.hh` / `SphericalSystemHitinOrbit.hh` / `TrackerHit.hh`

```cpp
G4double fTime = 0.;       // 原来无初始化
G4double fpT   = 0.;
G4int    fLayerID = -1;
G4double fEnergyDeposit = 0.;
```

---

#### 🟡-36. `layer` shadow 修复

**文件**：`Opposite/src/DetectorConstruction.cc:208`

```cpp
// ❌ int layer = 208;   // 局部遮蔽成员
// ✅ 删除局部声明，改用成员；并将头文件默认值从 1000 改为 208，与 AtmLayer[209] 对齐
G4int layer = 208;        // 头文件 DetectorConstruction.hh:67 同步改
```

同时删除第 87 行死变量 `layer_loop`（或真正用于层数判断）。

---

#### 其余（🟠-9/21、🟡-18/20/21/23/24/25/26/27、🟡-32/34/35/37/38/39/40/41）

- 🟠-9：`DetectorConstruction.cc:326` 检查 `logicSphereSDlayerinAir` 而非 `...Orbit`。
- 🟡-18：`SteppingAction` 计数改成按 trackID 存，或在 `PreUserTrackingAction` 重置 `n = 0`。
- 🟡-20：`Combine.cc` 临时树 `delete`，`dataAnalyseTest.cc` 的 `new Combine()` 用智能指针/手动释放。
- 🟡-21：重新评估 `G4AutoDelete::Register(fieldMgr)`（GEANT4 线程安全，官方推荐）。
- 🟡-23：删除注释块里字段顺序颠倒的 dtype。
- 🟡-24：`ToHigherOrbit.py:60` 取非负根。
- 🟡-25：`FluxCalculation.py` 统一方向符号约定。
- 🟡-26：`str0` 构造前校验文件存在，或在 load 失败时打印错误而非静默返回 `(0,0,0)`。
- 🟡-27：删除 `tracing.main` 未用参数 `n`、`thread`。
- 🟡-32：移除 `/field/SetType` 的 `quadrupole` 候选值，或补 `FieldType::kQuadrupole` 实现。
- 🟡-34/35：删除 `fCached*`、`dtest` 死代码。
- 🟡-37：确认 Time 列应否用 `GetGlobalTime()`。
- 🟡-38：`SteppingAction.cc:37` 加动能非零判断。
- 🟡-39：`fRadii` 构造对空 vector 加保护。
- 🟠-21：`tracingDecay.py:159` 的 `TFANG(seed=42)` 改用全局 `SeedSequence` 每次 `spawn` 派生的不重复种子（已在 🔴-8/9 方案中给出）。
- 🟡-40：`DecayProducts` 开头判 `channels` 是否为空（已在 🔴-8/9 方案中给出）。
- 🟡-41：`tracingDecay.py` 的调试 `print` 改为 `logging` 或删除。
