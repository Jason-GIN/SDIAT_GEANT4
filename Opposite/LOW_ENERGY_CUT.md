# 低能粒子截断（< 15 MeV）处理方法与注意事项

> 目标：在模拟中，一旦粒子动能低于 15 MeV 就将其停止输运，避免低能粒子浪费计算并污染输出。
> 本文档不修改任何代码，只给出方案、代码片段、效率分析与注意事项，供作者（GIN）参考后自行实施。
> 日期：2026-11-23

---

## 1. 问题现象

`MyStackingAction` 里设置了「动能 < 15 MeV 的次级粒子全部杀死」，但输出 ROOT 文件里**仍然出现大量 < 15 MeV 的粒子**，看起来像「截断没生效」。

## 2. 根因分析

### 2.1 Stacking action 只在粒子「出生时」判断一次

`G4UserStackingAction::ClassifyNewTrack()` 只在粒子**产生、入栈那一刻**被调用**一次**。它拿到的 `track` 是出生时的快照，动能是**出生时**的动能。

GEANT4 的调用流程：

```
粒子产生 → ClassifyNewTrack(只调一次) → fUrgent(立即输运) / fKill(杀死)
  └─ fUrgent 的粒子进入 tracking loop，之后每一步调用的是：
       SteppingAction::UserSteppingAction()   ← 每一步都调用
```

因此 stacking 只能杀「出生时 < 15 MeV」的次级粒子；**粒子出生后减速到 < 15 MeV，它管不着**。

### 2.2 输出文件记录的是「到达探测器时」的动能

`TrackerSD.cc` / `SphericalSystemSDinAir.cc` 里记录的都是：

```cpp
kE = newHit->GetKineticEnergy();   // = aStep->GetTrack()->GetKineticEnergy()，当前动能
```

也就是粒子**踩到探测器那一刻**的动能，不是出生时的动能。一个出生时 > 15 MeV 的粒子，输运中因电离损失减速，到达 SD1/SD2 时可能已 < 15 MeV，于是被记录。

### 2.3 实测验证：绝大多数 < 15 MeV 粒子是「被主动保留的中微子」

对 `build/protondata_0_all.root` 的实测结果：

| ntuple | < 15 MeV 行数 | ParentId==0 | 成分 |
|--------|:---:|:---:|------|
| DetectionInfo（SD2 地面）| 84 | 0 | **100% 中微子**（PID 12/14/-12/-14）|
| BoundaryInfo（SD1 130 km）| 88 | 0 | 67 个中微子 + 21 个非中微子（e±/γ/中子）|

结论：

- 所有 < 15 MeV 的粒子 **ParentId 都 > 0**（次级，无初级）——说明 stacking 对非中微子确实生效了。
- **绝大多数是中微子**：来自 `MyStackingAction.cc` 里的这行「保留中微子」分支（`apdg==12/14/16 → return fUrgent`）。这些 < 15 MeV 的中微子一路输运到 SD1/SD2，而 SD2 记录无能量阈值，于是全记了下来。
- 少部分非中微子（21 个，仅 SD1）：出生时 > 15 MeV，减速到 5–15 MeV 后，被 SD1 的 `kE >= 5 MeV` 记录。

---

## 3. 处理方法（三种，按推荐度排序）

### 方案 A —— 在 `SteppingAction::UserSteppingAction` 里每步判断（推荐，可控）

这才是「每一步都调用」的地方。在 `src/SteppingAction.cc` 的 `UserSteppingAction` **最开头**加判断：

```cpp
void SteppingAction::UserSteppingAction(const G4Step* step)
{
  G4Track* track = step->GetTrack();

  // 动能一旦低于 15 MeV 立即停止（次级粒子，含中微子）
  if (track->GetParentID() > 0 && track->GetKineticEnergy() < 15.0*MeV) {
    track->SetTrackStatus(fStopAndKill);
    return;
  }

  // ... 原有 fScoringVolume / flytime / process 等逻辑保持不变
}
```

变体（按需选择）：

```cpp
// 变体 1：保留中微子（与 stacking 的 "but keep neutrino" 一致）
G4int apdg = std::abs(track->GetDefinition()->GetPDGEncoding());
bool isNeutrino = (apdg == 12 || apdg == 14 || apdg == 16);
if (track->GetParentID() > 0 && !isNeutrino && track->GetKineticEnergy() < 15.0*MeV) {
  track->SetTrackStatus(fStopAndKill);
  return;
}

// 变体 2：连 Secondary 模式注入的 < 15 MeV 初级粒子也停（去掉 parentID 条件）
if (track->GetKineticEnergy() < 15.0*MeV) {
  track->SetTrackStatus(fStopAndKill);
  return;
}
```

**优点**：完全可控（可精确处理中微子 / 初级 / 次级）；逻辑直观。
**缺点**：每步多一次虚函数内的判断（开销见第 4 节，可忽略）。

### 方案 B —— 用 GEANT4 内置 `G4UserLimits::SetUserMinEkine`（更高效，但对中微子不可控）

在 `DetectorConstruction::Construct()` 里给最外层逻辑体积设置最小动能截止：

```cpp
#include "G4UserLimits.hh"
// ...
auto* limits = new G4UserLimits();
limits->SetUserMinEkine(15.0*MeV);   // 动能 < 15 MeV 自动停止
logicEnv->SetUserLimits(limits);      // 或 logicWorld
```

**优点**：GEANT4 内部的声明式截止，检查在 `G4SteppingManager` 里完成（早于 `UserSteppingAction`），比手动每步判断更省，是「最标准」的追踪截止手段。
**缺点**：
1. `SetUserMinEkine` 会作用于**所有粒子（含中微子）**，无法像方案 A 那样单独保留中微子；不同 GEANT4 版本对中微子 `minEkine` 的处理可能有差异，使用前需实测确认。
2. 只对**设置了该 limits 的逻辑体积**生效，需确认粒子输运路径上的体积都已覆盖（通常设在最外层 envelope/world 即可）。
3. 逻辑体积在 `Construct()` 里一次性构建，`SetUserLimits` 后需重建几何才生效。

### 方案 C —— 只在「记录侧」加能量阈值（不改输运，只改输出）

如果目的只是「让输出文件里别出现 < 15 MeV」，可以在各 SD 记录处加阈值，而不动粒子输运：

| 探测器 | 文件位置 | 当前阈值 | 建议 |
|--------|---------|---------|------|
| SD2（地面）| `TrackerSD.cc:142` | 无（`kE>=5MeV` 被注释）| 补 `kE >= 15.*MeV` |
| SD1（130 km）| `TrackerSD.cc:180` | `kE >= 5 MeV` | 改为 `15.*MeV` |
| SDinAir（大气）| `SphericalSystemSDinAir.cc:199` | `kE >= 5 MeV` | 按需 |
| SDinOrbit（轨道）| `SphericalSystemSDinOrbit.cc:179` | `kE >= 30 MeV` | 按需 |

**优点**：不改变输运、不增加每步开销。
**缺点**：只过滤「记录」，不减少「输运开销」——低能粒子仍然会跑完所有步，只是不写进输出。若目标是**提速**，此方案不解决输运开销。

---

## 4. 效率影响分析（重点）

### 4.1 每步判断的开销

方案 A 每步多做的只有：两次缓存 getter + 一次比较 + 一次分支，合计 **~5 ns**。
对比每步真正的物理计算（磁场积分 + 能量损失 + 多散射等，**μs 级**），占比 **< 0.1%**。

### 4.2 命中后是「净赚」

一旦动能 < 15 MeV，`fStopAndKill` 立即终止该粒子，省掉它后续**所有步**。低能粒子是步数最多的（小步长 + 回旋/漂移），一个俘获的低能粒子可能跑数百万步。杀掉它们 → 总步数显著下降 → **整体提速**。

### 4.3 结论

> **不会降低效率，通常反而提高。** 唯一要避免的「效率陷阱」是把判断写重（例如每步调用 `GetDefinition()->GetParticleName()` 取字符串、写 `G4cout`、做字符串比较等）。只要只用 `GetKineticEnergy()` / `GetParentID()` 这类缓存 getter，开销可忽略。

### 4.4 若追求极致

方案 B（`G4UserLimits`）比方案 A 更省（内部检查、早于 UserSteppingAction、声明式），但失去对中微子的灵活控制。若物理上允许「中微子也一起截断」，方案 B 是效率最优解；若必须精确区分中微子/初级/次级，方案 A 是正确性最优解，且其开销同样可忽略。

---

## 5. 注意事项

1. **中微子处理是核心决策**：你现在的 stacking 代码注释 "but keep neutrino" 保留了中微子。但实测表明 < 15 MeV 的噪声**几乎全是中微子**。若目标是「清空 < 15 MeV」，需连中微子一起截断（方案 A 主版本 / 方案 B）；若确实需要保留中微子，输出里将**永远**存在一堆 < 15 MeV 的中微子（这是物理上被放行的，不是 bug）。

2. **初级 vs 次级**：Primary 模式初级是 GeV 级宇宙线核，不会 < 15 MeV，`parentID > 0` 条件无影响；但 Secondary 模式注入的 `Parlist` 粒子是 `parentID == 0` 的 primary，若其中有 < 15 MeV 的粒子，会被 `parentID > 0` 条件放过。是否要停它们，取决于你希望 Secondary 注入的最低能量。

3. **物理正确性（最重要）**：15 MeV 截断会砍掉低能段的电子/γ/中子等次级粒子。如果你关心的是 **400 km 轨道通量**，这些 < 15 MeV 的粒子几乎无法逃逸出大气，截断影响可忽略；如果你也关心 **地面（SD2）或大气内部低能通量**，则需评估阈值对能谱低能端的影响，必要时调低阈值或做收敛性测试（阈值 5 / 10 / 15 / 30 MeV 对比结果是否稳定）。

4. **记录侧阈值不统一**：即使输运侧截断了 < 15 MeV，SD1 记录阈值仍是 5 MeV、SD2 无阈值。若希望「输出里也绝对没有 < 15 MeV」，需同时按方案 C 统一各 SD 的记录阈值，否则仍会看到 5–15 MeV 的减速粒子。

5. **与 stacking 的关系**：stacking 管「出生时」截断，stepping 管「输运中」截断，两者可并存。加上方案 A 后，stacking 里的 < 15 MeV 截止变得冗余（但无害——它在更早的入栈阶段就杀掉，还能省掉入栈开销）。**注意不要重复维护两处阈值**，建议统一为一个常量或宏。

6. **单位**：`GetKineticEnergy()` 返回单位是 MeV，阈值用 `15.0*MeV` 写，勿混用 GeV。

7. **多线程安全**：`SteppingAction` 每个 worker 线程各有一个实例，`UserSteppingAction` 内的判断是无状态局部变量，线程安全，无需加锁。

8. **`return` 提前返回**：方案 A 里 `return` 是为了跳过后面现有的 `flytime`/`process` 逻辑。若把判断放在别处，注意保持原逻辑不受影响。

---

## 6. 推荐实施步骤

1. 明确两个决策：**中微子是否一起截断**、**Secondary 注入的 < 15 MeV 初级是否截断**。
2. 选方案 A（可控，默认推荐）或方案 B（极致效率，且允许中微子一起截断）。
3. 若采用方案 A，在 `SteppingAction::UserSteppingAction` 最开头加对应判断（见第 3 节代码）。
4. 若希望输出也绝对干净，按方案 C 统一各 SD 记录阈值。
5. 重新编译（`cd build && make -j`），用同一宏跑一遍，验证 < 15 MeV 粒子是否消失。
6. 做一次阈值收敛性测试（5/10/15/30 MeV）确认物理结果对阈值不敏感，再决定最终阈值。
