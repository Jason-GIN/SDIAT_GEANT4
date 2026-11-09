# tracing.py / manager.py 修改后的内存溢出分析报告

> 范围：本次改动只针对 `Opposite/mypylib/tracing.py` 与 `Opposite/manager.py` 两处（对应 git 提交 `d9f124a`、`939ba25`）。
> 结论：**HTCondor 报"超内存"不是 128 GB 给小了，而是低刚度粒子被压出了几千万～几十亿步积分，单进程 `sol.y` 就涨到数 GB～数百 GB，再乘 32 个并发进程必然爆炸。** 下面逐条给出证据和修改建议（本文件只给建议，不改代码）。

---

## 0. 一句话结论（TL;DR）

新增的 `max_step = rigidity / (2*B)` 里，`rigidity` 单位是 **GV**，`B` 单位是 **nT**（纳特斯拉），两者直接相除得到的是 **10⁻⁶～10⁻⁸ 秒** 的超小步长（比实际回旋周期小 3～4 个量级）。对于被地磁场俘获、在镜像点之间反复反弹、又撞不到终止事件 `rLow`/`rHigh` 的粒子，积分会跑满整个 `tspan=(0,100)` 秒，步数 = 100 / maxstep 可达 **10⁸～10¹⁰ 步**。`solve_ivp` 默认把每一步状态都存进返回的 `sol.y`（7 行 × N 步 × 8 字节），于是：

| rigidity (GV) | 典型粒子 | maxstep (s) | 100 s 内步数 | 单进程 sol.y 内存 | ×32 并发 |
|---|---|---|---|---|---|
| 10 | 10 GeV 质子 | 1.67e-4 | 6.0e5 | 0.03 GB | 1.1 GB |
| 1 | 1 GeV 质子 | 1.67e-5 | 6.0e6 | 0.34 GB | 10.8 GB |
| 0.1 | 100 MeV 质子 | 1.67e-6 | 6.0e7 | **3.36 GB** | **107.5 GB** |
| 0.01 | 10 MeV 质子 | 1.67e-7 | 6.0e8 | **33.6 GB** | **1075 GB** |
| 0.001 | MeV 电子 | 1.67e-8 | 6.0e9 | **336 GB** | **10.7 TB** |

> 本地测不出来是因为：本地通常是**少粒子、高能、短时**的小样本，峰值内存压不进 128 GB；而 HTCondor 跑的是完整批次，包含大量低刚度（低能质子 / 电子 / 被俘获粒子），且 `MAX_CONCURRENT=32` 让 32 个进程同时持有各自的巨型 `sol.y`。

---

## 1. 本次改动的回顾（便于定位）

### 1.1 `tracing.py`（`main` 函数）
```python
def main(Xinit, monento, otherpar,i,n,thread):
    mMeV, charge, B = otherpar          # 新增：从 otherpar 解包磁场模长 B
    ...
    rigidity = pGeV / abs(charge) if charge != 0 else pGeV  # GV

    if rigidity > 50:      # 高能
        mt = 'DOP853'; tspan = (0, 10);  maxstep = np.inf
    elif rigidity > 20:    # 中高能
        mt = 'RK45';   tspan = (0.0,100.0); maxstep = np.inf
    elif rigidity > 10:    # 中低能
        mt = 'RK45';   tspan = (0.0,100.0); maxstep = rigidity/(2*B)
    else:                  # 低能
        mt = 'LSODA';  tspan = (0.0,100.0); maxstep = rigidity/(2*B)

    sol = integrate.solve_ivp(XVDinkm, tspan, XVinit,
                              method=mt, events=[rLow, rHigh, rDetector],
                              args=param, max_step=maxstep)
    ...
    return (i, sol.y_events[0], sol.t_events[0], sol.y_events[2], sol.t_events[2])
```

### 1.2 `manager.py`（`LoopBasic` 函数）
```python
from mypylib import myigrf14
from mypylib import *                # 新增通配符导入

def LoopBasic(particlelist, Egnum, n, mypth):
    ...
    r,t,p = RotationModel.xyz2rthetaphi(particlelist[:,0], particlelist[:,1], particlelist[:,2])
    ln = len(t)
    colat = np.asfortranarray(np.degrees(t), dtype=np.float64)
    lon   = np.asfortranarray(np.degrees(p), dtype=np.float64)
    r     = np.asfortranarray(r, dtype=np.float64)
    B     = np.empty(ln, dtype=np.float64)                       # ← 疑似死代码
    _, _, _, B = myigrf14.igrf_wrapper_array(ln, colat, lon, r, 2025.0)  # B 单位 nT
    ...
    args = (plist[0:3], plist[3:6], [plist[10], plist[9], B[i]], i, Egnum, n)  # 传 B[i]
```

---

## 2. 🔴 核心问题：`maxstep = rigidity/(2*B)` 单位不一致 → 步长被压小 3～4 个量级

### 2.1 证据
- `tracing.py` 内注释明确 `B` 单位是 **nT**（`# B in nT，地球磁场大小约3.e-5T`），即 400 km 轨道典型 `B ≈ 3e4 nT`。
- `rigidity = pGeV/abs(charge)` 单位是 **GV**（吉伏）。
- `rigidity/(2*B)` = GV / nT = (10⁹ V) / (10⁻⁹ T)，量纲上既不是时间也不是长度，纯粹是"两个不同量纲的数直接相除"，结果是 10⁻⁶～10⁻⁸ 量级的纯小数。

### 2.2 为什么它会让内存爆掉
对低刚度（低能）粒子，`maxstep` 极小 → 若粒子在 100 s 内始终没撞到终止事件（`rLow` 90 km 大气、`rHigh` 10⁵ km 逃逸），`solve_ivp` 会把 100 s 全部走完：

- 步数 N = 100 / maxstep = 200·B / R。
- `solve_ivp` 默认 `dense_output=False`，但**仍然把每个已接受步的完整状态存进 `sol.y`**（7 个状态分量 × N 步 × 8 字节）。
- 单进程峰值内存 ≈ 7·N·8 字节，见第 0 节表格：R=0.1 GV 时单进程 3.36 GB，R=0.01 GV 时 33.6 GB，电子 R≈0.001 GV 时 336 GB。
- `manager.py` 用 `mps.Pool(processes=32)` + `starmap` 一次性并发 32 个粒子，**32 个进程同时持有各自的 `sol.y`**，峰值叠加直接越过 128 GB。

> 补充：`solve_ivp` 的自适应步长只会"更小不会更大"，`max_step` 是上界。低能分支还用了刚性求解器 `LSODA`，配上这么小的 `max_step` 反而更慢、步数更多。

### 2.3 为什么本地能跑、HTCondor 超内存
本地小样本时：粒子数少、能量高、没有大量俘获粒子，且机器内存充足；HTCondor 跑完整批次时，低刚度俘获粒子（镜像反弹、长时间不触发终止事件）成批出现，32 进程叠加即 OOM。**这不是 HTCondor 的配置问题，是代码里的步长量级问题。**

---

## 3. 🟠 次级问题清单（按严重程度）

| 级别 | 位置 | 问题 | 影响 |
|---|---|---|---|
| 🟠 | `tracing.py` | `max_step` 未设上限/未控步数，低能俘获粒子跑满 100 s | 内存+时间双重爆炸（与 🔴 同源） |
| 🟠 | `tracing.py` | `main()` 只用了 `sol.y_events`/`sol.t_events`，却让 `solve_ivp` 存了整个轨迹 `sol.y` | 白白吃内存，可彻底避免 |
| 🟠 | `manager.py:160` | `pool.starmap` 一次性并发 32、且等全部返回后再统一 vstack | 峰值内存叠加、结果全量驻留 |
| 🟡 | `manager.py:147` | `B = np.empty(ln)` 随即被 `igrf_wrapper_array` 返回值覆盖 | 死代码，白分配一段数组（非泄漏，但暴露"以为 f2py 会原地写回"的误解） |
| 🟡 | `manager.py:18` | `from mypylib import *` 通配符导入 | 命名空间污染、`RotationModel` 来源不透明、易与显式导入冲突 |
| 🟡 | `tracing.py` | `maxstep=np.inf` 作为 `max_step` 参数显式传入 | 无实际意义（默认就是无穷），且个别求解器对 `np.inf` 处理不一致，建议直接省略 |
| 🟡 | `tracing.py` | 低能分支选 `LSODA` 又强加极小 `max_step` | 刚性求解器配微小步长效率极低；非刚性场一般 `RK45` 已够 |
| 🟡 | `manager.py` | 对**全部**粒子一次性 `igrf_wrapper_array` 算 B | 若 `ln` 极大（粒子数多）会一次性生成多份 `ln` 长度数组；此处通常不大，属次要 |

---

## 4. 详细修改建议（未改动你的代码，仅供参考）

### 建议 1（必改）：修正 `maxstep` 的量级与单位 —— 用回旋周期，而不是 GV/nT
`max_step` 的目的应是"每个回旋周期内取若干步以保证精度"。回旋周期（秒）的实用公式：

```
τ_g [s] ≈ 69.8 × R[GV] / ( β × B[nT] )，其中 β = v/c
```

建议把步长定为 `τ_g / 20` 左右（每周期约 20 步，精度足够、步数合理），并加一个下限防止步长过小：

```python
beta = velosity[-1] / cc          # velosity 已存的是 v；β = v/c
if beta < 1e-4:                   # 防除零 / 非物理
    beta = 1e-4
gyro_period = 69.8 * rigidity / (beta * B)   # 秒
maxstep = max(gyro_period / 20.0, 1e-3)      # 下限 1 ms，避免低能粒子步长失控
```

这样 R=0.1 GV、B=3e4 nT、β≈0.1 时：τ_g≈2.3 ms，`maxstep≈1.2e-4 s`，100 s 内约 8.7e5 步，`sol.y`≈49 MB（原来 3.36 GB），内存直接降 ~70 倍。

####这个逻辑是有个小问题的，R=0.1 GV时，对质子确实β≈0.1，但是对电子不是的。所以必须考虑上2.

### 建议 2（强烈推荐，直接根治内存）：不存整条轨迹，只留事件

#### 2.1 这是什么意思（一句话）

你的 `main()` 里 `solve_ivp` 返回的 `sol`，最后**只用了 `sol.y_events` 和 `sol.t_events`（事件），从头到尾没用到 `sol.y`（整条轨迹）**。但 `solve_ivp` 默认会把每一步的状态都存进 `sol.y`。低刚度粒子走几千万～几十亿步时，这个"没人用"的 `sol.y` 就成了 GB 级的大数组。建议 2 就是：**让 `solve_ivp` 别存整条轨迹，只在少数几个时间点存值**，内存就从"随步数线性暴涨"变成"固定几百个点"。

#### 2.2 原理：为什么 `sol.y` 会随步数暴涨（看 scipy 源码）

`scipy.integrate._ivp.ivp.py`（本机 scipy 1.15.3）里的存储逻辑是这样的：

```python
# 第 623-632 行：根据是否传 t_eval 决定怎么存
if t_eval is None:
    ts = [t0]          # ← 默认情况：ts 从初值开始逐个累加
    ys = [y0]
else:
    ts = []            # ← 传了 t_eval：只准备存 t_eval 里的点
    ys = []

# 主循环里，每走一步（第 696-723 行）：
if t_eval is None:
    ts.append(t)       # ← 每一个被接受的步都 append 一次
    ys.append(y)       #    → sol.y 的列数 == 步数
else:
    t_eval_step = t_eval[搜索区间]      # 只取落在本步区间内的 t_eval 点
    if t_eval_step.size > 0:
        ts.append(t_eval_step)          # ← 只存 t_eval 网格上的点
        ys.append(sol(t_eval_step))     #    → sol.y 的列数 == len(t_eval)
```

**关键点**：`t_eval=None` 时，`ts`/`ys` 每一步 append 一次，最终 `sol.y.shape = (状态数, 步数)`；传了 `t_eval` 后，`ts`/`ys` 只在跨过 `t_eval` 网格点时 append，最终 `sol.y.shape = (状态数, len(t_eval))`。

以你的场景为例：7 个状态分量、一个低刚度俘获粒子走 10⁸ 步：

- 不传 `t_eval`：`sol.y` = 7 × 10⁸ 个 float64 ≈ **5.6 GB**；
- 传 `t_eval`（200 点）：`sol.y` = 7 × 200 个 float64 ≈ **11 KB**。

#### 2.3 为什么事件（`y_events`/`t_events`）不受影响

事件用的是完全独立的一条路径（源码第 673-694 行）：每走一步后，求解器检查事件函数有没有变号；一旦发现变号，就**临时**构造一个 `solver.dense_output()`（第 678 行）做插值，再用 `scipy.optimize.brentq` 精确求根（第 681-687 行），把准确的 `t_events` 和 `y_events` 记下来。这个插值对象是**用完即弃**的，不累积。

所以：**事件的发生时刻和位置，是由根查找精确得到的，与 `t_eval` 取多粗完全无关**。`t_eval` 只是决定"额外保存多少轨迹采样点"，不参与事件判定。

#### 2.4 实验验证（本机 scipy 1.15.3 实测）

用 3 维衰减方程，`max_step=1e-3`、`tspan=(0,10)` 跑（共 10002 步）：

```
A 无 t_eval:        sol.y.shape = (3, 10002)    # 存了全部 10002 步
B 传 t_eval(200点):  sol.y.shape = (3, 200)      # 只存 200 个点
  压缩比 = 10002 / 200 ≈ 50 倍（步数越多、差距越大）
C 传 t_eval+事件:    t_events[0] = [0.69314718]  # = ln2，精确根，不受 t_eval 影响
```

你的场景步数是 10⁶～10⁹ 量级，压缩比会是几万～几千万倍。

#### 2.5 具体怎么改（`tracing.py` 的 `main`）

```python
    # 在调 solve_ivp 前，构造一个粗粒度采样网格（t_eval 必须升序、且在 t_span 内）
    t_eval = np.linspace(tspan[0], tspan[1], 200)   # 200 点足够；哪怕只用事件，取 2~200 点都行

    sol = integrate.solve_ivp(XVDinkm, tspan, XVinit, method=mt,
                              events=[rLow, rHigh, rDetector],
                              args=param, max_step=maxstep,
                              t_eval=t_eval)        # ← 关键：加这一行
    ...
    # 下面 return 只用 sol.y_events / sol.t_events，完全不受影响
    return (i, sol.y_events[0], sol.t_events[0], sol.y_events[2], sol.t_events[2])
```

如果连 200 个采样点都不需要（`main` 只用事件），甚至可以取 `t_eval=np.linspace(tspan[0], tspan[1], 2)`，`sol.y` 就只剩 2 列。

#### 2.6 注意事项

1. **`t_eval` 必须升序且落在 `[t0, tf]` 内**，否则 `solve_ivp` 直接抛 `ValueError`（源码第 603-608 行）。你的 `tspan` 都是正方向 `(0,10)`/`(0,100)`，`np.linspace(tspan[0], tspan[1], N)` 天然满足。
2. **终止事件（`rLow`/`rHigh` 是 `terminal=True`）触发时积分提前停**，`t_eval` 里排在终止时刻之后的点不会被填，`sol.y` 只到终止时刻为止——这对你只用事件的写法没有影响。
3. `t_eval` 本身几乎不增加计算开销（每步只做一次 `np.searchsorted` + 极少量插值），比省下的内存和时间小得多。
4. 这一步**与建议 1 相互独立、可叠加**：即使先不改 `maxstep` 的量级，单靠 `t_eval` 就能把单进程 `sol.y` 从 GB 级压到 KB 级，直接解除 OOM；再配合建议 1 把步数降下来，连计算时间也一起解决。

### 建议 3（降峰值）：降低并发 / 用 `imap` 增量回收结果
`pool.starmap` 会等全部任务结束、且把结果全量塞进列表。若粒子数很多，改成增量式并降低并发：

```python
MAX_CONCURRENT = 8           # 从 32 降到 8，峰值内存 / 4
with mps.Pool(processes=MAX_CONCURRENT) as pool:
    for result in pool.imap_unordered(tracing.main, args_list, chunksize=1):
        ...  # 逐条消费，不必等全部完成
```

或保留 32 并发但配合建议 2（单进程内存已很小），也可。

### 建议 4（清理）：去掉死代码 / 通配符导入 / `np.inf`
```python
# manager.py：删除死分配
# B = np.empty(ln, dtype=np.float64)          # 删掉
_, _, _, B = myigrf14.igrf_wrapper_array(ln, colat, lon, r, 2025.0)

# manager.py：显式导入替代通配符
from mypylib import tracing, myigrf14, RotationModel   # 别用 import *

# tracing.py：高能分支别传 np.inf，直接省略 max_step
sol = integrate.solve_ivp(..., max_step=maxstep)   # 高能分支可不传 max_step
```

### 建议 5（可选，需你判断物理）：低能分支换回 `RK45`
`LSODA` 是为刚性方程设计的，地磁场轨迹大多非刚性；配极小 `max_step` 时 `LSODA` 效率很低。若不确定刚性，`RK45` 通常更稳更快。若保留 `LSODA`，注意 `max_step` 对它同样生效、同样会被压小。

---

## 5. 建议的验证方法（改完后本地先复现）

1. **单粒子低刚度压测**：构造一个 R≈0.01 GV、初始条件为镜像俘获的粒子（撞击不到 `rLow`/`rHigh`），单独跑 `tracing.main`，用 `/usr/bin/time -v` 看峰值 RSS，确认单进程 `sol.y` 不再 GB 级。
2. **监控步数**：在 `main()` 里打印 `sol.t.size` 或 `sol.y.shape[1]`，确认步数从 10⁸ 降到 10⁵～10⁶ 量级。
3. **批量回归**：跑一小批真实 `particlelist`，对比改动前后 `Parlist`/`Hitlist` 的数值（事件坐标、时间）是否一致，确保改步长没有改变物理结果。
4. **HTCondor 复测**：先用 `MAX_CONCURRENT=8` + 少量 bin 试投，确认内存峰值显著下降后再放量。

---

## 6. 附：为什么 `velosity` 行没动但也要留意（历史遗留，非本次改动）
`tracing.py` 里 `velosity = cc*monento/1e6/EGeV` 用的是 `cc`（m/s）而非 km/s，而注释声称"距离单位 km、时间单位 s"，存在一个约 1000 倍的量纲隐患。这不属于本次修改，但会影响轨迹时间与事件时刻的物理正确性，建议后续单独核对。本次内存问题与它无关，仅作提醒。

---

*报告生成于 2026-11-05，仅分析本次 `tracing.py`/`manager.py` 改动，未修改任何代码。*
