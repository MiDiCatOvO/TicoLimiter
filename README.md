# Tico Limiter

~ cute bus limiter ~

## 概述

Tico Limiter 是一款可爱风格的总线限制器，具备 True Peak 检测、Tico Magic 均衡、PikaPika 动态清理等功能。支持 AU / VST3 / Standalone 格式。

---

## 信号链

```
Input → Tico Power → Saturation → Tico Magic → Oversample → Soft Clip → Limiter → Downsample → Dry/Wet Mix → Output
```

---

## Controls 页面

### I/O 控制

| 控件 | 说明 | 范围 |
|------|------|------|
| **Tico Power** | 输入增益 | 0 ~ +30 dB |
| **Mix** | 干湿比 | 0 ~ 100% |
| **Ceiling** | 输出上限（限制器阈值） | -0.1 / -0.3 / -0.5 / -1 / -3 dB |
| **Oversampling** | 超采样倍数 | 2x / 4x / 8x / 16x / 32x / 64x |
| **Sample Rate** | 内部采样率 | 44.1k / 48k / 88.2k / 96k |
| **True Peak** | ISP 检测（强制最低 4x 超采样） | ON / OFF |

### Headroom Meter

输入增益旁的纵向彩色条：
- **绿色** = 有余量，可以继续推 input
- **黄色** = 接近极限
- **红色** = 已满幅，推更多不会增加响度

### GR Meter

显示限制器增益衰减量（dB），带分段彩色条和心形指示器。

### Saturation（饱和）

| 控件 | 说明 |
|------|------|
| **ON** | 开关 |
| **Odd/Even** | 奇偶次谐波混合（0% = 纯偶次，100% = 纯奇次） |
| **Drive** | 饱和度 |

### Soft Clip（软削波）

| 控件 | 说明 |
|------|------|
| **ON** | 开关 |
| **Ratio** | 软削波与原始信号的混合比（1:2 / 1:3 / 1:5 / 1:8） |

### Limiter（限制器）

| 控件 | 说明 | 范围 |
|------|------|------|
| **Release** | 释放时间 | 10 ~ 500 ms |
| **Look-Ahead** | 前瞻时间（attack 自动绑定） | 0 ~ 10 ms |
| **Auto Rel** | 自适应释放（快瞬态快释放，持续信号慢释放） | ON / OFF |

---

## Analysis 页面

### Tico Magic（微笑曲线 EQ）

| 控件 | 说明 |
|------|------|
| **ON** | 开关 |
| **Kirakira** | EQ 强度（0 ~ 6 dB） |

**工作原理：**
- 低频搁架提升（120Hz，斜率较陡）
- 高频搁架提升（6kHz，斜率较缓，约低频的 70%）
- 增益量由 Kirakira 滑条线性控制

### PikaPika（动态频段清理）

4 个可独立开关的频段：

| 频点 | 用途 |
|------|------|
| **200Hz** | 低中频浑浊区 |
| **280Hz** | 低中频浑浊区 |
| **370Hz** | 中低频浑浊区 |
| **500Hz** | 中频浑浊区 |

**工作原理：**
- 自适应 Threshold：追踪信号长期平均电平，Threshold = 平均 - 6dB
- 安静信号 → Threshold 低 → 几乎不触发 → 保留 punch
- 响亮信号 → Threshold 跟随 → 正常压缩，不会压死
- 压缩比 3:1，衰减量受 Kirakira 滑条控制（0 ~ 6 → 0% ~ 100%）
- 衰减量缩放系数 25%（温和处理）
- 静态曲线完全平直，纯动态处理

### 频谱分析器

- **蓝色**：输入频谱
- **粉色**：输出频谱
- 频率范围：20Hz ~ 20kHz（对数刻度）
- 刻度标注：20 / 50 / 100 / 200 / 500 / 1k / 2k / 5k / 10k / 20k
- 等响度补偿显示

### GR 历史

- **蓝色曲线**：限制器 GR
- **粉色曲线**：软削波 GR
- 横轴时间，纵轴 dB（0 ~ -12dB）
- 右侧带当前值指示器

### 输出计量

| 显示 | 说明 |
|------|------|
| **dB** | 输出峰值电平 |
| **RMS** | 输出均方根电平 |
| **TP** | 当前 / 最大 True Peak（点击重置最大值） |
| **LUFS** | 瞬时 / 短期 / 综合响度 |

---

## 技术参数

| 项目 | 说明 |
|------|------|
| 格式 | AU / VST3 / Standalone |
| 声道 | 立体声 |
| 最低系统 | macOS 11.0 |
| 构建 | JUCE 8.0.6, C++17 |

---

## 注意事项

- **Look-Ahead** 会引入延迟，宿主会自动补偿（PDC）
- **True Peak** 开启时强制最低 4x 超采样
- **64x 超采样** CPU 开销较大，建议离线渲染使用
- **Auto Release** 适合大多数场景，手动 Release 适合精细控制
- **PikaPika** 的自适应 Threshold 会根据输入电平自动调整，不需要手动设置

## 构建

```bash
cd KawaiiLimiter
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
