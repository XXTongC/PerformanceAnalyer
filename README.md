# Performance Analyzer Plugin - 用户文档

## 📋 目录

1. [插件概述](#1-插件概述)
2. [系统要求](#2-系统要求)
3. [安装指南](#3-安装指南)
4. [快速入门](#4-快速入门)
5. [功能模块详解](#5-功能模块详解)
   - [5.1 性能分析器](#51-性能分析器)
   - [5.2 实时性能监控](#52-实时性能监控)
   - [5.3 LOD 自动生成器](#53-lod-自动生成器)
   - [5.4 材质优化器](#54-材质优化器)
---

## 1. 插件概述

**Performance Analyzer** 是一款专为 Unreal Engine 5.7 设计的综合性能优化工具集，旨在帮助开发者快速识别和解决项目中的性能瓶颈。

### 核心特性

- ✅ **实时性能监控** - 多级数据采集系统，实时追踪 FPS、DrawCalls、GPU/CPU 时间等关键指标
- ✅ **智能场景分析** - 自动检测高多边形网格、缺失 LOD、材质热点等性能问题
- ✅ **自动 LOD 生成** - 支持批量生成、多种预设、事务回滚的 LOD 自动化工具
- ✅ **材质批量优化** - 一键将材质转换为材质实例，减少渲染开销
- ✅ **详细性能报告** - 生成可导出的性能分析报告，包含问题分级和优化建议

### 插件信息

| 项目 | 信息 |
|------|------|
| **版本** | 1.0 |
| **作者** | XXTong |
| **引擎版本** | Unreal Engine 5.7 |
| **支持平台** | Windows 64-bit |
| **模块类型** | Editor Plugin |
| **许可证** | Epic Games License |

---

## 2. 系统要求

### 最低要求

- **操作系统**: Windows 10/11 (64-bit)
- **Unreal Engine**: 5.7 或更高版本
- **内存**: 16 GB RAM
- **存储空间**: 100 MB（插件本身）

### 推荐配置

- **操作系统**: Windows 11 (64-bit)
- **Unreal Engine**: 5.7+
- **内存**: 32 GB RAM
- **GPU**: 支持 DirectX 12 的显卡

---

## 3. 安装指南

### 方法一：通过插件文件夹安装

1. **下载插件**

   - 下载 `PerformanceAnalyzer` 插件压缩包
   - 解压到本地目录

2. **复制到项目**

YourProject/
   └── Plugins/
       └── PerformanceAnalyzer/
           ├── PerformanceAnalyzer.uplugin
           ├── Source/
           ├── Resources/
           └── Content/

3. **启用插件**

- 打开 Unreal Editor
- 进入 `Edit` → `Plugins`
- 搜索 "Performance Analyzer"
- 勾选启用插件
- 重启编辑器

### 方法二：通过引擎插件目录安装

将插件复制到引擎插件目录（所有项目共享）：UE_5.7/Engine/Plugins/Marketplace/PerformanceAnalyzer/


### 验证安装

安装成功后，你应该能在以下位置看到插件菜单：

- **主菜单**: `Tools` → `Performance Analyzer`
- **工具栏**: 顶部工具栏会出现 Performance Analyzer 图标

---

## 4. 快速入门

### 第一次使用

1. **打开插件主窗口**
   - 点击 `Tools` → `Performance Analyzer` → `Open Plugin Window`
   - 或使用快捷键 `Ctrl+Shift+P`

2. **分析当前场景**
   - 点击 `Analyze Scene` 按钮
   - 等待分析完成（通常 5-30 秒）
   - 查看生成的性能报告

3. **查看实时监控**
   - 点击 `Tools` → `Performance Analyzer` → `Performance Monitor`
   - 观察实时性能曲线图

### 典型工作流程

打开场景 → 运行性能分析 → 查看报告 → 发现问题
    ↓
问题类型判断:
├── LOD缺失 → 使用LOD生成器
├── 材质过多 → 使用材质优化器
└── 其他问题 → 手动优化
    ↓
重新分析 → 导出报告 → 完成

---

## 5. 功能模块详解

### 5.1 性能分析器

#### 功能概述

性能分析器是插件的核心模块，负责扫描整个场景并生成详细的性能报告。

#### 使用方法

**启动分析**

1. 打开要分析的关卡
2. 点击 `Tools` → `Performance Analyzer` → `Analyze Scene`
3. 或使用快捷键 `Ctrl+Shift+P`

**分析内容**

分析器会检测以下内容：

| 检测项 | 说明 | 严重等级 |
|--------|------|----------|
| **高多边形网格** | 三角形数 > 50,000 | 🔴 High |
| **中等多边形网格** | 三角形数 > 20,000 | 🟡 Medium |
| **缺失 LOD** | 网格没有 LOD 层级 | 🟡 Medium |
| **高 DrawCall Actor** | 材质槽位 > 10 | 🔴 High |
| **中等 DrawCall Actor** | 材质槽位 > 5 | 🟡 Medium |

#### 报告结构

```cpp
FPerformanceReport
├── TotalActors          // 场景总 Actor 数
├── FrameTimeMS          // 帧时间（毫秒）
├── TotalStaticMeshes    // 静态网格总数
├── TotalTriangles       // 总三角形数
├── HighPolyMeshCount    // 高多边形网格数量
├── MeshesWithoutLODs    // 缺失 LOD 的网格数
├── Warnings[]           // 警告列表
└── Issues[]             // 问题详情列表
    ├── ActorName        // 问题 Actor 名称
    ├── IssueType        // 问题类型
    ├── Description      // 详细描述
    └── Severity         // 严重程度 (1-3)
```

#### 导出报告

导出为文本文件

```cpp
// 通过菜单
Tools → Performance Analyzer → Export Report

// 保存位置
YourProject/Saved/PerformanceReports/Report_YYYY-MM-DD_HH-MM-SS.txt
```

### 5.2 实时性能监控

#### 功能概述

实时性能监控器提供持续的性能数据采集和可视化，帮助开发者在编辑器中实时观察性能变化。

#### 打开监控窗口

```cpp
Tools → Performance Analyzer → Performance Monitor
```


#### 数据采集系统

监控器采用**三级数据获取策略**：

```cpp
Level 1: RHI Stats (最准确)
    ↓ 失败
Level 2: Engine Stats (较准确)
    ↓ 失败
Level 3: Scene Estimation (估算)
```

**数据源说明**

| 数据源 | 准确度 | 可用性 | 说明 |
|--------|--------|--------|------|
| **RHI** | ⭐⭐⭐⭐⭐ | 运行时 | 直接从渲染硬件接口获取 |
| **Engine** | ⭐⭐⭐⭐ | 编辑器 | 从引擎统计系统获取 |
| **Estimated** | ⭐⭐⭐ | 始终可用 | 基于场景分析估算 |

#### 监控指标

**核心指标**

FPerformanceSample
├── Timestamp            // 时间戳
├── FrameTimeMS          // 帧时间（毫秒）
├── GPUTimeMS            // GPU 时间
├── RenderThreadTimeMS   // 渲染线程时间
├── GameThreadTimeMS     // 游戏线程时间
├── DrawCalls            // DrawCall 数量
├── Primitives           // Primitive 数量
├── Triangles            // 三角形数量
└── MemoryUsageMB        // 内存使用（MB）

### 5.3 LOD 自动生成器（支持undo/redo）

#### 功能概述

LOD（Level of Detail）自动生成器可以为静态网格批量生成多级 LOD，通过在不同距离显示不同精度的模型来优化渲染性能。

距离近 → LOD 0 (高精度，100% 三角形)
距离中 → LOD 1 (中精度，50% 三角形)
距离远 → LOD 2 (低精度，25% 三角形)
距离很远 → LOD 3 (最低精度，12.5% 三角形)


**性能收益**

| 场景 | 无 LOD | 有 LOD | 性能提升 |
|------|--------|--------|----------|
| 开放世界 | 5,000,000 三角形 | 1,500,000 三角形 | ✅ 70% |
| 室内场景 | 2,000,000 三角形 | 800,000 三角形 | ✅ 60% |
| 建筑可视化 | 10,000,000 三角形 | 3,000,000 三角形 | ✅ 70% |

#### 打开 LOD 生成器

```cpp
Tools → Performance Analyzer → LOD Generator
```


#### 预设模式

插件提供 4 种预设模式，适用于不同场景：

**1. High Quality（高质量）**

```cpp
适用场景: character、重要道具、近景物体
LOD 配置:
├── LOD 数量: 4
├── LOD 0: 100% 三角形, 屏幕尺寸 1.0
├── LOD 1: 75% 三角形, 屏幕尺寸 0.5
├── LOD 2: 50% 三角形, 屏幕尺寸 0.25
└── LOD 3: 25% 三角形, 屏幕尺寸 0.125
```

**2. Balanced（平衡）**

```cpp
适用场景: 一般场景物体、建筑、环境道具
LOD 配置:
├── LOD 数量: 4
├── LOD 0: 100% 三角形, 屏幕尺寸 1.0
├── LOD 1: 50% 三角形, 屏幕尺寸 0.5
├── LOD 2: 25% 三角形, 屏幕尺寸 0.25
└── LOD 3: 12.5% 三角形, 屏幕尺寸 0.125
````

**3. Performance（性能优先）**

```cpp
适用场景: 背景物体、远景、大量重复物体
LOD 配置:
├── LOD 数量: 5
├── LOD 0: 100% 三角形, 屏幕尺寸 1.0
├── LOD 1: 40% 三角形, 屏幕尺寸 0.5
├── LOD 2: 20% 三角形, 屏幕尺寸 0.25
├── LOD 3: 10% 三角形, 屏幕尺寸 0.125
└── LOD 4: 5% 三角形, 屏幕尺寸 0.0625
````

**4. Mobile（移动端）**

```cpp
适用场景: 移动端项目、低端设备
LOD 配置:
├── LOD 数量: 3
├── LOD 0: 100% 三角形, 屏幕尺寸 1.0
├── LOD 1: 40% 三角形, 屏幕尺寸 0.5
└── LOD 2: 20% 三角形, 屏幕尺寸 0.25
```

#### 使用步骤

**步骤 1: 选择网格**

方法 1: 在内容浏览器中选择

1. 打开 Content Browser
2. 选择一个或多个静态网格资产
3. 打开 LOD Generator

方法 2: 在视口中选择

1. 在关卡视口中选择 Actor
2. 打开 LOD Generator
3. 插件会自动提取 Actor 使用的网格

方法 3: 刷新选择

1. 打开 LOD Generator
2. 在场景中选择新的网格
3. 点击 "Refresh Selection" 按钮


**步骤 2: 选择预设**

1. 点击预设按钮（High Quality / Balanced / Performance / Mobile）
2. 查看预设描述
3. 预设会自动更新所有设置


**步骤 3: 自定义设置（可选）**

```cpp
基本设置:
├── Number of LODs: 调整 LOD 数量（1-8）
├── Recalculate Normals: 重新计算法线（推荐开启）
├── Generate Lightmap UVs: 生成光照贴图 UV（推荐开启）
└── Welding Threshold: 顶点焊接阈值（0.0-10.0）
LOD 配置:
├── Reduction %: 每级 LOD 的简化百分比
└── Screen Size: LOD 切换的屏幕尺寸阈值
```

**步骤 4: 预览（推荐）**

1. 点击 "Preview" 按钮
2. 等待预览生成（不会修改原始资产）
3. 查看预览结果:

   * 每级 LOD 的三角形数量
   * 简化百分比
   * 预估性能提升

**步骤 5: 生成 LOD**

1. 确认设置无误后，点击 "Generate LODs" 按钮
2. 等待生成完成（显示进度条）
3. 查看生成结果对话框
4. 生成的 LOD 会自动保存到网格资产中


#### 批量生成流程

```cpp
选择网格 → 选择预设 → (可选)自定义设置 → 预览
    ↓
满意? 
├── 否 → 调整设置 → 重新预览
└── 是 → 生成 LOD
    ↓
显示进度 → 完成 → 查看结果
```

#### 事务支持（Undo/Redo）

LOD 生成器支持完整的撤销/重做功能：

```txt
生成 LOD 后可以撤销:
Ctrl+Z  // 撤销 LOD 生成，恢复原始网格
Ctrl+Y  // 重做 LOD 生成

事务记录内容:
FLODGenerationTransaction
├── BackupData[]              // 备份的网格数据
│   ├── OriginalNumLODs       // 原始 LOD 数量
│   └── LODSettings[]         // 每级 LOD 的设置
│       ├── BuildSettings     // 构建设置
│       ├── ReductionSettings // 简化设置
│       └── ScreenSize        // 屏幕尺寸
└── ProcessedMeshes[]         // 处理的网格列表
```

### 5.4 材质优化器

#### 功能概述

材质优化器可以扫描场景中的材质使用情况，并将材质转换为材质实例（Material Instance），从而减少渲染开销和提升性能。

#### 核心概念

**材质 vs 材质实例**

```cpp
材质 (Material)
├── 完整的着色器代码
├── 每次修改需要重新编译
├── 渲染开销较大
└── 适合作为"母版"
材质实例 (Material Instance)
├── 继承自父材质
├── 只修改参数，无需编译
├── 渲染开销小
├── 支持运行时修改
└── 适合大量使用
```

**性能收益**

| 场景 | 优化前 | 优化后 | 性能提升 |
|------|--------|--------|----------|
| 材质数量 | 50 个材质 | 5 个材质 + 45 个实例 | ✅ 减少 Shader 切换 |
| 编译时间 | 5 分钟 | 30 秒 | ✅ 90% 提升 |
| 运行时修改 | 不支持 | 支持 | ✅ 动态材质效果 |
| 内存占用 | 200 MB | 50 MB | ✅ 75% 降低 |

#### 打开材质优化器（支持undo/redo）

Tools → Performance Analyzer → Material Optimizer

#### 使用步骤

**步骤 1: 扫描场景**

1. 打开材质优化器
2. (可选) 勾选 "Only Selected Actors" 只扫描选中的 Actor
3. 点击 "Scan Scene" 按钮
4. 等待扫描完成（通常 5-30 秒）


**步骤 2: 配置优化设置**

Save Path (保存路径):

* 材质实例的保存位置
* 默认: /Game/Materials/Instances
* 建议: 按项目结构组织

Instance Prefix (前缀):

* 材质实例名称前缀
* 默认: MI_
* 示例: M_Building → MI_Building

Instance Suffix (后缀):

* 材质实例名称后缀
* 默认: 空
* 示例: M_Building → M_Building_Inst

Auto Save Assets (自动保存):
* 是否自动保存生成的资产
* 推荐: 开启

**命名示例**

原始材质: M_Building_Master

配置 1:   
Prefix: MI_   
Suffix: (空)   
结果: MI_Building_Master


配置 2:   
Prefix: (空)   
Suffix: _Inst   
结果: M_Building_Master_Inst

配置 3:    
Prefix: MI_   
Suffix: _01   
结果: MI_Building_Master_01


**步骤 3: 选择要优化的材质**

方法 1: 手动选择
* 在列表中勾选要优化的材质
* 可以逐个选择

方法 2: 全选
* 点击 "Select All" 按钮
* 选择所有可优化的材质

方法 3: 取消全选
* 点击 "Deselect All" 按钮
* 清除所有选择

**步骤 4: 执行优化**

1. 确认选择的材质
2. 点击 "Optimize Selected Materials" 按钮
3. 等待优化完成
4. 查看优化结果

#### 优化流程

```cpp
扫描场景 → 收集材质信息 → 显示列表
    ↓
选择材质 → 配置设置 → 执行优化
    ↓
创建材质实例 → 替换引用 → 保存资产
    ↓
显示结果 → 完成
```
