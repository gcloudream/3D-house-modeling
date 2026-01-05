这份文档是**《桌面端户型仿真设计软件（Qt版）完整开发规格书》**。它整合了之前的所有讨论，补充了家具系统，并针对 Qt C++ 开发环境进行了深度的技术拆解。您可以直接将这份文档交给开发团队（或作为自己的开发大纲）。

------

# 项目名称：QtPlanArchitect (暂定名)

**版本**：V1.0

**平台**：Windows

**技术栈**：Qt 6 (C++), OpenGL / Qt3D

------

## 第一部分：软件需求规格说明书 (SRS)

### 1. 系统概述

本软件旨在为用户提供一个基于PC桌面的轻量级室内设计工具。核心工作流为：**导入户型底图 -> 描摹墙体 -> 拖拽门窗/家具 -> 实时3D预览 -> 导出数据**。

### 2. 详细功能模块

#### 2.1 画布与视图控制 (Canvas & Viewport)

- **无限画布**：支持 X/Y 轴无限延伸。
- **视图操作**：
  - **缩放**：鼠标滚轮（以光标为中心缩放）。
  - **平移**：按住鼠标中键或空格键+左键拖拽。
- **辅助系统**：
  - **网格 (Grid)**：背景显示 100mm x 100mm 小格，1000mm x 1000mm 大格。
  - **标尺 (Ruler)**：画布边缘显示刻度。
  - **十字光标**：全屏十字辅助线，随鼠标移动，辅助对齐。

#### 2.2 底图临摹系统 (Blueprint System)

- **导入**：支持 JPG, PNG, BMP 格式。
- **图层管理**：底图位于最底层 (`Z-Value = -100`)。
- **透明度调节**：滑动条控制底图不透明度 (0% - 100%)。
- **比例尺校准 (核心功能)**：
  - 交互：用户在底图的已知长度边上画一条线（如测量尺）。
  - 输入：弹窗输入该线的实际长度（如 "5000mm"）。
  - 计算：系统计算 `px_per_mm` 比率，并自动缩放底图以匹配 1:1 坐标系。

#### 2.3 智能墙体系统 (Smart Wall)

- **绘制模式**：
  - **连续绘制**：左键点击确定起点/转折点，右键结束。
  - **正交限制**：按住 Shift 键，仅允许水平或垂直绘制。
  - **动态输入**：绘制时直接键盘输入数字（如 "3000"）并回车，按鼠标方向生成指定长度墙体。
- **墙体属性**：
  - 厚度 (Thickness)：默认 240mm，可调。
  - 高度 (Height)：默认 2800mm，可调。
- **智能连接 (Auto-Join)**：
  - **L型连接**：墙体角点自动切角 (Miter Join)。
  - **T型连接**：新墙体端点吸附到旧墙体中间时，自动融合。
- **吸附 (Snapping)**：
  - 吸附端点、中点、垂足。

#### 2.4 门窗与开洞 (Openings)

- **依附逻辑**：门窗不能独立存在，必须“寄生”在墙体对象上。
- **自动吸附**：拖拽门窗靠近墙体时，自动旋转方向并吸附。
- **布尔运算**：
  - **2D表现**：门窗图元层级高于墙体，且带有白色背景遮挡墙体线条。
  - **3D表现**：实时切割墙体 Mesh，生成洞口。

#### 2.5 家具软装系统 (Furniture & Assets)

- **库管理**：分类展示（客厅、卧室、厨卫等）。
- **2D/3D 映射**：
  - 2D 视图显示 `.svg` 矢量符号（简洁线条）。
  - 3D 视图显示 `.obj` 或 `.glb` 模型。
- **交互操作**：
  - **移动**：拖拽。
  - **旋转**：选中后出现旋转手柄，支持 45度 吸附。
  - **缩放**：支持锁定长宽比缩放。
  - **离地高度**：可设置 Z 轴偏移（如吊灯）。

#### 2.6 3D 实时预览 (Real-time Preview)

- **画中画 (PIP)**：主界面右上角悬浮小窗口。
- **同步机制**：2D 画布的任何修改（墙体移动、加家具），3D 视图必须在 <100ms 内更新。
- **漫游控制**：
  - 左键拖拽：旋转视角 (Orbit)。
  - 右键拖拽：平移视角 (Pan)。
  - 滚轮：推拉视角 (Zoom)。

------

## 第二部分：UI 界面设计规范

采用 **Qt Widgets** 结合 **QSS (Qt Style Sheets)** 打造现代化扁平风格。

### 1. 布局结构 (Layout)

Plaintext

```
+---------------------------------------------------------------+
|  菜单栏 (MenuBar) & 工具栏 (ToolBar: 保存, 撤销, 导出)          |
+-------+---------------------------------------------+---------+
|       |                                             |         |
| 组件  |                                             |  属性   |
| 库    |              中央绘图区                      |  面板   |
| (List)|           (QGraphicsView)                   | (Dock)  |
|       |                                             |         |
| [墙体]|      +-----------------------+              | [名称]  |
| [门窗]|      |   3D 预览窗 (悬浮)     |              | [宽]    |
| [家具]|      |   (QOpenGLWidget)     |              | [高]    |
|       |      +-----------------------+              | [厚]    |
|       |                                             | [贴图]  |
|       |                                             |         |
+-------+---------------------------------------------+---------+
|  状态栏: 坐标 (X,Y) | 比例尺 | 当前操作提示                      |
+---------------------------------------------------------------+
```

### 2. 交互细节

- **选中高亮**：选中墙体或家具时，显示蓝色边框 (`#1890FF`) 和控制点（Handle）。
- **光标反馈**：
  - 移动到墙体边缘 -> 显示“移动”光标。
  - 移动到端点 -> 显示“调整大小”光标。
  - 绘制中 -> 十字准星。

------

## 第三部分：技术架构与类设计 (Qt C++)

这是开发的核心指南，定义了如何使用 Qt 框架实现上述功能。

### 1. 核心架构图

采用 **Document-View** 变体模式：

- **Scene (Model)**: 存储所有几何数据。
- **View (2D UI)**: `QGraphicsView` 负责 2D 交互。
- **Renderer (3D UI)**: `QOpenGLWidget` 负责读取 Scene 数据并渲染 3D。

### 2. 关键类定义

#### A. 基础图元类

C++

```
// 墙体类
class WallItem : public QGraphicsPolygonItem {
public:
    QPointF startPos, endPos;
    qreal thickness;
    qreal height;
    
    // 核心逻辑：根据起点终点和厚度，计算 Polygon 的4个点
    void updateGeometry(); 
    
    // 用于3D生成的接口
    QVector<QVector3D> get3DVertices(); 
};

// 家具类
class FurnitureItem : public QGraphicsSvgItem {
public:
    QString modelID; // 关联 3D 模型文件
    qreal elevation; // 离地高度
    
    // 资源加载
    void loadAsset(QString id);
};
```

#### B. 场景管理器 (Scene Manager)

C++

```
class DesignScene : public QGraphicsScene {
    Q_OBJECT
public:
    // 模式控制：Select, DrawWall, DrawDimension
    enum Mode { Mode_Select, Mode_DrawWall };
    void setMode(Mode mode);

protected:
    // 所有的鼠标交互逻辑都在这里处理
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

signals:
    // 核心信号：通知 3D 视图更新
    void sceneContentChanged(); 
};
```

#### C. 3D 渲染器 (The Bridge)

C++

```
class View3DWidget : public QOpenGLWidget, protected QOpenGLFunctions {
public:
    // 接收场景数据
    void syncFromScene(DesignScene *scene);
    
protected:
    void paintGL() override {
        // 1. 清空屏幕
        // 2. 遍历 scene 中的 WallItem -> 生成立方体 -> 绘制
        // 3. 遍历 scene 中的 FurnitureItem -> 加载外部模型 -> 变换矩阵 -> 绘制
    }
};
```

### 3. 难点算法解决方案

#### 3.1 墙体融合算法 (Wall Joining)

当两段墙体成 90度 连接时，不能简单重叠。

- **方案**：使用 **向量计算**。
  1. 计算两条墙中心线的交点。
  2. 根据墙厚，计算内角点和外角点。
  3. 更新 `QGraphicsPolygonItem` 的顶点坐标，使其在视觉上无缝连接。

#### 3.2 坐标转换 (Coordinate System)

- **Qt 2D**: X 向右，Y 向下 (0,0 在左上)。
- **OpenGL 3D**: X 向右，Y 向上，Z 向外 (右手坐标系)。
- **转换公式**：
  - 3D_X = 2D_X
  - 3D_Y = Wall_Height (或 Furniture_Elevation)
  - 3D_Z = **2D_Y** (注意：通常 2D 的 Y 对应 3D 的 Z 平面)

------

## 第四部分：数据存储设计 (JSON)

文件后缀名建议为 `.qplan`，内容为 JSON 格式，方便解析。

JSON

```
{
  "project_info": {
    "version": "1.0",
    "created_at": "2023-10-27"
  },
  "settings": {
    "background_image": "path/to/image.png",
    "pixel_ratio": 0.05,  // 1 pixel = 0.05 meters
    "opacity": 0.5
  },
  "walls": [
    {
      "id": "w_1",
      "start": [0, 0],
      "end": [5000, 0],
      "thickness": 240,
      "height": 2800
    },
    {
      "id": "w_2",
      "start": [5000, 0],
      "end": [5000, 4000],
      "thickness": 240,
      "height": 2800
    }
  ],
  "openings": [
    {
      "type": "door",
      "wall_id": "w_1",
      "distance_from_start": 1200,
      "width": 900
    }
  ],
  "furniture": [
    {
      "model_id": "sofa_01",
      "pos": [2500, 2000],
      "rotate": 90,
      "scale": [1, 1, 1]
    }
  ]
}
```

------

## 第五部分：开发分工与排期 (建议)

1. **第一周：基础框架**
   - 搭建 Qt 主界面布局。
   - 实现 `GraphicsView` 的网格绘制和缩放/平移。
2. **第二周：底图与墙体 (2D)**
   - 底图导入与比例尺逻辑。
   - 墙体绘制（暂不考虑连接算法，先画出矩形）。
3. **第三周：3D 渲染基础**
   - 引入 OpenGL 或 Qt3D。
   - 实现 `WallItem` 到 3D Mesh 的转换。
4. **第四周：进阶功能**
   - 墙体融合算法。
   - 门窗“挖洞”逻辑。
   - 家具库的 UI 与拖拽实现。
5. **第五周：完善与优化**
   - 保存/读取功能。
   - 导出图片/CAD。

这份文档覆盖了从产品设计到代码实现的全部关键点，可以直接作为软件工程的指导文件使用。