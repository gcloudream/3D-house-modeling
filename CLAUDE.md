# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

**QtPlanArchitect** - 基于 Qt 6 的桌面端户型仿真设计软件

**技术栈**: Qt 6.8+, C++17, OpenGL 3.3+, MinGW 64-bit
**平台**: Windows
**架构**: Document-View 模式 + Graphics Scene Framework

## 开发命令

### 构建项目
```bash
# 使用 Qt Creator 打开项目
# File -> Open File or Project -> 选择 untitled.pro

# 或使用 qmake 命令行构建
qmake untitled.pro
mingw32-make
```

### 运行项目
```bash
# 在 Qt Creator 中按 Ctrl+R 运行
# 或在 build 目录执行生成的可执行文件
cd build/Desktop_Qt_6_8_3_MinGW_64_bit-Debug
./untitled.exe
```

### 清理构建
```bash
mingw32-make clean
# 或删除 build 目录
```

## 核心架构

### 分层设计

```
Presentation Layer (UI)
  ├── MainWindow (QMainWindow) - 主窗口
  ├── PropertiesPanel (QDockWidget) - 属性面板
  └── ComponentLibrary (QListWidget) - 组件库

Business Logic Layer
  ├── DesignScene (QGraphicsScene) - 场景管理
  ├── ToolManager (StateMachine) - 工具状态管理
  └── AssetManager (ResourcePool) - 资源管理

Graphics Items (Model)
  ├── WallItem - 墙体图元
  ├── OpeningItem - 门窗图元
  └── FurnitureItem - 家具图元

Rendering Layer
  ├── View2DWidget (QGraphicsView) - 2D 视图
  ├── View3DWidget (QOpenGLWidget) - 3D 渲染
  └── RenderEngine - 网格生成器

Data Layer
  ├── ProjectManager (Singleton) - 项目管理
  ├── JSONSerializer - 序列化
  └── QUndoStack - 撤销/重做
```

### 核心类关系

**DesignScene** 是中心枢纽:
- 管理所有图形项目 (WallItem, FurnitureItem, OpeningItem)
- 处理鼠标交互和工具模式切换
- 通过信号 `sceneContentChanged()` 触发 3D 视图同步
- 负责网格绘制、吸附系统、坐标转换

**WallItem** (墙体):
- 继承 `QGraphicsPolygonItem`
- 核心数据: startPos, endPos, thickness, height
- 关键方法: `updateGeometry()` - 根据起点终点厚度计算4个顶点
- 3D 接口: `get3DVertices()` - 生成立方体顶点供 OpenGL 使用
- 支持智能连接 (L型/T型融合算法)

**View3DWidget** (3D 渲染器):
- 继承 `QOpenGLWidget` + `QOpenGLFunctions_3_3_Core`
- 连接 DesignScene 的信号,实时同步 2D 修改
- `paintGL()` 遍历场景对象生成网格并渲染
- 要求同步延迟 <100ms

## 关键算法

### 墙体几何计算
墙体以中心线定义,需计算 Polygon 的4个顶点:
```cpp
// 1. 计算中心线的法向量
QLineF centerLine(startPos, endPos);
QLineF perpendicular = centerLine.normalVector();
perpendicular.setLength(thickness / 2.0);

// 2. 计算4个角点 (p1, p2, p3, p4)
// 3. 设置 Polygon
setPolygon(QPolygonF() << p1 << p2 << p3 << p4);
```

### 墙体智能连接
当两墙体端点接近时:
1. 判断连接类型 (L型 90度 / T型)
2. 计算融合点和新顶点
3. 更新两墙体的 Polygon 实现无缝连接
参见 `WallJoinAlgorithm::computeJoin()` (详细设计文档第3.1节)

### 吸附系统
优先级排序: 端点 (1.0) > 中点 (2.0) > 网格 (3.0)
- 收集所有候选吸附点
- 按距离+优先级排序
- 返回容差范围内 (默认10px) 的最佳点

### 坐标转换
```
Qt 2D:    X→右, Y→下 (左上角原点)
OpenGL:   X→右, Y→上, Z→外 (右手系)

转换公式:
  3D_X = 2D_X
  3D_Y = Wall_Height (或 Furniture_Elevation)
  3D_Z = -2D_Y  (注意负号,因为 Y 方向相反)
```

## 数据结构

### 项目文件格式 (.qplan)
JSON 格式,包含:
- `project_info`: 版本、创建时间
- `settings`: 底图路径、像素比率、透明度
- `walls`: 墙体数组 (id, start, end, thickness, height)
- `openings`: 门窗数组 (type, wall_id, distance_from_start, width)
- `furniture`: 家具数组 (model_id, pos, rotate, scale)

### 资源组织
```
assets/
  ├── catalog.json      # 资源索引 (id, name, category, svg, model)
  ├── icons/            # 2D SVG 图标
  │   └── *.svg
  └── models/           # 3D 模型
      └── *.obj / *.glb
```

## 设计模式应用

- **Singleton**: AssetManager, ProjectManager
- **Command**: 撤销/重做 (AddWallCommand, MoveItemCommand)
- **State**: ToolManager 工具切换
- **Observer**: Scene → View3D 信号槽同步
- **Factory**: ItemFactory 图元创建

## 性能优化要点

### 渲染优化
- **视锥剔除**: 只渲染可见对象
- **批量渲染**: 合并所有墙体为一个大 Mesh,减少 Draw Call
- **模型缓存**: 多个家具实例共享同一 3D 模型数据

### 2D/3D 同步
- DesignScene 发射 `sceneContentChanged()` 信号
- View3DWidget 槽函数标记脏标志
- 下一次 `paintGL()` 时重新生成网格
- 目标延迟: <100ms

## Qt 特性使用

### Graphics View Framework
- **Scene**: 管理图元、处理交互、信号通知
- **View**: 显示 Scene、缩放平移、视口变换
- **Item**: 自定义图元 (重写 paint, boundingRect, shape)

### OpenGL 集成
- `QOpenGLWidget::initializeGL()` - 初始化着色器、VAO/VBO
- `QOpenGLWidget::paintGL()` - 每帧渲染循环
- `QOpenGLWidget::resizeGL()` - 更新投影矩阵

### 工具类
- `QUndoStack` - 自动管理撤销/重做历史
- `QGraphicsSceneMouseEvent` - 场景坐标鼠标事件
- `QMatrix4x4`, `QVector3D` - 3D 数学库

## 开发注意事项

### 墙体开洞 (门窗)
**2D 实现**: 门窗图元层级高于墙体 + 白色背景遮挡
**3D 实现**: CSG 布尔减法或简化的分段渲染 (将墙体分成开洞前/后段)

### 比例尺校准流程
1. 用户在底图上画线标记已知长度
2. 输入实际尺寸 (如 5000mm)
3. 计算 `pixelsPerMm = 线段像素长度 / 实际长度`
4. 缩放底图使其匹配 1:1 坐标系

### 正交绘制 (Shift 键)
绘制墙体时检测 Shift 键状态,限制鼠标移动方向为水平或垂直

### 动态输入
绘制墙体过程中键盘输入数字,按回车在鼠标方向生成指定长度墙体

## 扩展开发

### 添加新图元类型
1. 继承 `QGraphicsItem` 或子类
2. 实现 `boundingRect()`, `shape()`, `paint()`
3. 定义 `toJson()` / `fromJson()` 序列化方法
4. 在 DesignScene 添加管理接口
5. (可选) 实现 `get3DVertices()` 支持 3D 渲染

### 集成新 3D 模型格式
使用 Assimp 库加载:
```cpp
Assimp::Importer importer;
const aiScene* scene = importer.ReadFile(path,
    aiProcess_Triangulate | aiProcess_GenNormals);
// 提取 vertex/index 数据到 MeshData
```

### 添加新工具模式
1. 在 `DesignScene::Mode` 枚举添加新模式
2. 在 `mousePressEvent/mouseMoveEvent` 添加处理逻辑
3. 在工具栏添加切换按钮并连接 `setMode()` 槽
