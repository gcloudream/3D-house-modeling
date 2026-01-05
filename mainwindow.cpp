#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "designscene.h"
#include "view2dwidget.h"
#include "view3dwidget.h"
#include "wallitem.h"

#include <QActionGroup>
#include <QAction>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsItem>
#include <QInputDialog>
#include <QLineF>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointF>
#include <QPoint>
#include <QPixmap>
#include <QRect>
#include <QKeySequence>
#include <QSignalBlocker>
#include <QSlider>
#include <QSize>
#include <QStringList>
#include <QStatusBar>
#include <QToolBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_scene(nullptr)
    , m_view2d(nullptr)
    , m_view3d(nullptr)
    , m_previewDock(nullptr)
    , m_coordLabel(nullptr)
    , m_zoomLabel(nullptr)
    , m_hintLabel(nullptr)
    , m_componentList(nullptr)
    , m_nameValue(nullptr)
    , m_startXSpin(nullptr)
    , m_startYSpin(nullptr)
    , m_endXSpin(nullptr)
    , m_endYSpin(nullptr)
    , m_lengthSpin(nullptr)
    , m_angleSpin(nullptr)
    , m_heightSpin(nullptr)
    , m_thicknessSpin(nullptr)
    , m_blueprintOpacitySlider(nullptr)
    , m_selectAction(nullptr)
    , m_drawWallAction(nullptr)
    , m_calibrateAction(nullptr)
    , m_snapAction(nullptr)
    , m_deleteAction(nullptr)
{
    ui->setupUi(this);

    setWindowTitle("QtPlanArchitect");

    setupCentralView();
    setupDocks();
    setupToolbar();
    setupStatusBar();
    setupPreview3D();
    connectSignals();
    updateToolHint(m_scene->mode());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupCentralView()
{
    m_scene = new DesignScene(this);
    m_scene->applyWallHeightToAllWalls(200.0);
    m_view2d = new View2DWidget(this);
    m_view2d->setObjectName("view2d");
    m_view2d->setScene(m_scene);
    m_view2d->centerOn(0.0, 0.0);
    m_view2d->setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout(ui->centralwidget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->addWidget(m_view2d);
    ui->centralwidget->setLayout(layout);
}

void MainWindow::setupDocks()
{
    auto *libraryDock = new QDockWidget(tr("组件库"), this);
    libraryDock->setObjectName("componentLibraryDock");
    m_componentList = new QListWidget(libraryDock);
    m_componentList->setObjectName("componentList");
    m_componentList->setSpacing(4);
    m_componentList->addItems(QStringList() << tr("墙体")
                                            << tr("门窗")
                                            << tr("家具"));
    libraryDock->setWidget(m_componentList);
    libraryDock->setMinimumWidth(200);
    addDockWidget(Qt::LeftDockWidgetArea, libraryDock);

    auto *propertiesDock = new QDockWidget(tr("属性面板"), this);
    propertiesDock->setObjectName("propertiesDock");
    auto *propertiesWidget = new QWidget(propertiesDock);
    auto *formLayout = new QFormLayout(propertiesWidget);
    m_nameValue = new QLabel("-", propertiesWidget);
    m_lengthSpin = new QDoubleSpinBox(propertiesWidget);
    m_heightSpin = new QDoubleSpinBox(propertiesWidget);
    m_thicknessSpin = new QDoubleSpinBox(propertiesWidget);
    m_startXSpin = new QDoubleSpinBox(propertiesWidget);
    m_startYSpin = new QDoubleSpinBox(propertiesWidget);
    m_endXSpin = new QDoubleSpinBox(propertiesWidget);
    m_endYSpin = new QDoubleSpinBox(propertiesWidget);
    m_angleSpin = new QDoubleSpinBox(propertiesWidget);

    const double coordLimit = 100000.0;
    for (QDoubleSpinBox *spin : {m_startXSpin, m_startYSpin, m_endXSpin, m_endYSpin}) {
        spin->setRange(-coordLimit, coordLimit);
        spin->setDecimals(0);
        spin->setSuffix(" mm");
        spin->setEnabled(false);
    }

    m_lengthSpin->setRange(1.0, 100000.0);
    m_lengthSpin->setDecimals(0);
    m_lengthSpin->setSuffix(" mm");

    m_heightSpin->setRange(10.0, 10000.0);
    m_heightSpin->setDecimals(0);
    m_heightSpin->setSuffix(" mm");
    m_heightSpin->setValue(200.0);

    m_thicknessSpin->setRange(10.0, 1000.0);
    m_thicknessSpin->setDecimals(0);
    m_thicknessSpin->setSuffix(" mm");
    m_thicknessSpin->setValue(40.0);

    m_angleSpin->setRange(0.0, 360.0);
    m_angleSpin->setDecimals(1);
    m_angleSpin->setSuffix(" °");

    m_lengthSpin->setEnabled(false);
    m_heightSpin->setEnabled(false);
    m_thicknessSpin->setEnabled(false);
    m_angleSpin->setEnabled(false);

    formLayout->addRow(tr("名称"), m_nameValue);
    formLayout->addRow(tr("起点 X"), m_startXSpin);
    formLayout->addRow(tr("起点 Y"), m_startYSpin);
    formLayout->addRow(tr("终点 X"), m_endXSpin);
    formLayout->addRow(tr("终点 Y"), m_endYSpin);
    formLayout->addRow(tr("长度"), m_lengthSpin);
    formLayout->addRow(tr("角度"), m_angleSpin);
    formLayout->addRow(tr("高度"), m_heightSpin);
    formLayout->addRow(tr("厚度"), m_thicknessSpin);

    m_blueprintOpacitySlider = new QSlider(Qt::Horizontal, propertiesWidget);
    m_blueprintOpacitySlider->setRange(0, 100);
    m_blueprintOpacitySlider->setValue(60);
    m_blueprintOpacitySlider->setEnabled(false);
    formLayout->addRow(tr("底图透明度"), m_blueprintOpacitySlider);

    propertiesDock->setWidget(propertiesWidget);
    propertiesDock->setMinimumWidth(240);
    addDockWidget(Qt::RightDockWidgetArea, propertiesDock);
}

void MainWindow::setupToolbar()
{
    auto *toolbar = addToolBar(tr("主工具栏"));
    toolbar->setObjectName("mainToolBar");
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    toolbar->setIconSize(QSize(18, 18));

    auto *saveAction = new QAction(tr("保存"), this);
    auto *undoAction = new QAction(tr("撤销"), this);
    m_deleteAction = new QAction(tr("删除"), this);
    m_deleteAction->setShortcut(QKeySequence::Delete);
    m_deleteAction->setShortcutContext(Qt::WindowShortcut);
    m_deleteAction->setEnabled(false);
    auto *exportAction = new QAction(tr("导出"), this);
    auto *importBlueprintAction = new QAction(tr("导入底图"), this);
    m_snapAction = new QAction(tr("吸附"), this);
    m_snapAction->setCheckable(true);
    m_snapAction->setChecked(true);
    auto *alignHorizontalAction = new QAction(tr("水平对齐"), this);
    auto *alignVerticalAction = new QAction(tr("垂直对齐"), this);

    m_selectAction = new QAction(tr("选择"), this);
    m_selectAction->setCheckable(true);
    m_drawWallAction = new QAction(tr("墙体"), this);
    m_drawWallAction->setCheckable(true);
    m_calibrateAction = new QAction(tr("校准"), this);
    m_calibrateAction->setCheckable(true);

    auto *toolGroup = new QActionGroup(this);
    toolGroup->addAction(m_selectAction);
    toolGroup->addAction(m_drawWallAction);
    toolGroup->addAction(m_calibrateAction);
    toolGroup->setExclusive(true);
    m_selectAction->setChecked(true);

    toolbar->addAction(saveAction);
    toolbar->addAction(undoAction);
    toolbar->addAction(m_deleteAction);
    toolbar->addSeparator();
    toolbar->addAction(exportAction);
    toolbar->addSeparator();
    toolbar->addAction(m_selectAction);
    toolbar->addAction(m_drawWallAction);
    toolbar->addAction(m_calibrateAction);
    toolbar->addAction(m_snapAction);
    toolbar->addAction(alignHorizontalAction);
    toolbar->addAction(alignVerticalAction);
    toolbar->addSeparator();
    toolbar->addAction(importBlueprintAction);

    auto *fileMenu = menuBar()->addMenu(tr("文件"));
    fileMenu->addAction(saveAction);
    fileMenu->addAction(importBlueprintAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exportAction);

    auto *editMenu = menuBar()->addMenu(tr("编辑"));
    editMenu->addAction(undoAction);
    editMenu->addAction(m_deleteAction);
    editMenu->addAction(m_snapAction);
    editMenu->addAction(alignHorizontalAction);
    editMenu->addAction(alignVerticalAction);

    connect(m_selectAction, &QAction::triggered, this, [this]() {
        m_scene->setMode(DesignScene::Mode_Select);
    });
    connect(m_drawWallAction, &QAction::triggered, this, [this]() {
        m_scene->setMode(DesignScene::Mode_DrawWall);
    });
    connect(m_calibrateAction, &QAction::triggered, this, [this]() {
        if (!m_scene->hasBlueprint()) {
            QMessageBox::information(this, tr("提示"), tr("请先导入底图再进行校准。"));
            return;
        }
        m_scene->setMode(DesignScene::Mode_Calibrate);
    });
    connect(m_snapAction, &QAction::toggled, this, [this](bool checked) {
        m_scene->setSnapEnabled(checked);
    });
    connect(m_deleteAction, &QAction::triggered, this, [this]() {
        const QList<QGraphicsItem *> items = m_scene->selectedItems();
        bool removed = false;
        for (QGraphicsItem *item : items) {
            auto *wall = qgraphicsitem_cast<WallItem *>(item);
            if (!wall) {
                continue;
            }
            m_scene->removeItem(wall);
            delete wall;
            removed = true;
        }
        if (!removed) {
            QMessageBox::information(this, tr("提示"), tr("请先选择墙体。"));
        }
        updateSelectionDetails();
    });
    connect(alignHorizontalAction, &QAction::triggered, this, [this]() {
        WallItem *wall = selectedWall();
        if (!wall) {
            QMessageBox::information(this, tr("提示"), tr("请先选择墙体。"));
            return;
        }
        const qreal length = wall->length();
        if (length < 0.1) {
            return;
        }
        const QPointF center = (wall->startPos() + wall->endPos()) / 2.0;
        const QPointF start(center.x() - length / 2.0, center.y());
        const QPointF end(center.x() + length / 2.0, center.y());
        wall->setStartPos(start);
        wall->setEndPos(end);
        wall->updateGeometry();
    });
    connect(alignVerticalAction, &QAction::triggered, this, [this]() {
        WallItem *wall = selectedWall();
        if (!wall) {
            QMessageBox::information(this, tr("提示"), tr("请先选择墙体。"));
            return;
        }
        const qreal length = wall->length();
        if (length < 0.1) {
            return;
        }
        const QPointF center = (wall->startPos() + wall->endPos()) / 2.0;
        const QPointF start(center.x(), center.y() - length / 2.0);
        const QPointF end(center.x(), center.y() + length / 2.0);
        wall->setStartPos(start);
        wall->setEndPos(end);
        wall->updateGeometry();
    });
    connect(importBlueprintAction, &QAction::triggered, this, &MainWindow::importBlueprint);
}

void MainWindow::setupStatusBar()
{
    m_hintLabel = new QLabel(tr("操作: 选择"), this);
    m_hintLabel->setObjectName("hintLabel");

    m_coordLabel = new QLabel(tr("坐标: X 0  Y 0"), this);
    m_coordLabel->setObjectName("coordLabel");

    m_zoomLabel = new QLabel(tr("缩放: 100%"), this);
    m_zoomLabel->setObjectName("zoomLabel");

    QFont mono("Cascadia Mono");
    mono.setStyleHint(QFont::Monospace);
    m_coordLabel->setFont(mono);
    m_zoomLabel->setFont(mono);

    statusBar()->setSizeGripEnabled(false);
    statusBar()->addWidget(m_hintLabel, 1);
    statusBar()->addPermanentWidget(m_coordLabel);
    statusBar()->addPermanentWidget(m_zoomLabel);
}

void MainWindow::setupPreview3D()
{
    m_view3d = new View3DWidget(this);
    m_view3d->setObjectName("view3d");
    m_view3d->setScene(m_scene);

    m_previewDock = new QDockWidget(tr("3D 预览"), this);
    m_previewDock->setObjectName("preview3DDock");
    m_previewDock->setWidget(m_view3d);
    m_previewDock->setAllowedAreas(Qt::NoDockWidgetArea);
    m_previewDock->setFeatures(QDockWidget::DockWidgetFloatable
                               | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::RightDockWidgetArea, m_previewDock);
    m_previewDock->setFloating(true);
    m_previewDock->resize(360, 240);

    QTimer::singleShot(0, this, [this]() {
        if (!m_previewDock) {
            return;
        }
        const QRect frame = frameGeometry();
        const QPoint anchor = frame.topRight();
        m_previewDock->move(anchor - QPoint(m_previewDock->width() + 24, -48));
    });
}

void MainWindow::connectSignals()
{
    connect(m_view2d, &View2DWidget::mouseMovedInScene, this,
            [this](const QPointF &pos) {
                m_coordLabel->setText(
                    tr("坐标: X %1  Y %2")
                        .arg(pos.x(), 0, 'f', 0)
                        .arg(pos.y(), 0, 'f', 0));
            });

    connect(m_view2d, &View2DWidget::zoomChanged, this,
            [this](qreal scale) {
                m_zoomLabel->setText(
                    tr("缩放: %1%").arg(scale * 100.0, 0, 'f', 0));
            });

    connect(m_scene, &DesignScene::modeChanged, this, &MainWindow::updateToolHint);
    connect(m_scene, &DesignScene::calibrationRequested,
            this, &MainWindow::handleCalibration);
    connect(m_scene, &QGraphicsScene::selectionChanged,
            this, &MainWindow::updateSelectionDetails);
    connect(m_scene, &DesignScene::sceneContentChanged,
            this, &MainWindow::updateSelectionDetails);

    connect(m_blueprintOpacitySlider, &QSlider::valueChanged, this,
            [this](int value) {
                m_scene->setBlueprintOpacity(value / 100.0);
            });

    connect(m_lengthSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                WallItem *wall = selectedWall();
                if (!wall) {
                    return;
                }
                QLineF line(wall->startPos(), wall->endPos());
                if (line.length() < 0.1) {
                    return;
                }
                line.setLength(value);
                wall->setEndPos(line.p2());
                wall->updateGeometry();
                m_scene->notifySceneChanged();
            });

    connect(m_angleSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                WallItem *wall = selectedWall();
                if (!wall) {
                    return;
                }
                QLineF line(wall->startPos(), wall->endPos());
                if (line.length() < 0.1) {
                    return;
                }
                line.setAngle(value);
                wall->setEndPos(line.p2());
                wall->updateGeometry();
                m_scene->notifySceneChanged();
            });

    connect(m_startXSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                WallItem *wall = selectedWall();
                if (!wall) {
                    return;
                }
                QPointF start = wall->startPos();
                start.setX(value);
                wall->setStartPos(start);
                wall->updateGeometry();
                m_scene->notifySceneChanged();
            });

    connect(m_startYSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                WallItem *wall = selectedWall();
                if (!wall) {
                    return;
                }
                QPointF start = wall->startPos();
                start.setY(value);
                wall->setStartPos(start);
                wall->updateGeometry();
                m_scene->notifySceneChanged();
            });

    connect(m_endXSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                WallItem *wall = selectedWall();
                if (!wall) {
                    return;
                }
                QPointF end = wall->endPos();
                end.setX(value);
                wall->setEndPos(end);
                wall->updateGeometry();
                m_scene->notifySceneChanged();
            });

    connect(m_endYSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                WallItem *wall = selectedWall();
                if (!wall) {
                    return;
                }
                QPointF end = wall->endPos();
                end.setY(value);
                wall->setEndPos(end);
                wall->updateGeometry();
                m_scene->notifySceneChanged();
            });

    connect(m_heightSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                WallItem *wall = selectedWall();
                if (!wall) {
                    return;
                }
                wall->setHeight(value);
                m_scene->notifySceneChanged();
            });

    connect(m_thicknessSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                WallItem *wall = selectedWall();
                if (!wall) {
                    return;
                }
                wall->setThickness(value);
                m_scene->notifySceneChanged();
            });
}

void MainWindow::importBlueprint()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("导入底图"),
        QString(),
        tr("图像文件 (*.png *.jpg *.jpeg *.bmp)"));

    if (fileName.isEmpty()) {
        return;
    }

    QPixmap pixmap(fileName);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, tr("导入失败"), tr("无法加载选中的图片。"));
        return;
    }

    if (!m_scene->setBlueprintPixmap(pixmap)) {
        QMessageBox::warning(this, tr("导入失败"), tr("底图初始化失败。"));
        return;
    }

    m_blueprintOpacitySlider->setEnabled(true);
    m_blueprintOpacitySlider->setValue(60);
}

void MainWindow::updateSelectionDetails()
{
    WallItem *wall = selectedWall();
    if (!wall) {
        m_nameValue->setText("-");
        if (m_deleteAction) {
            m_deleteAction->setEnabled(false);
        }
        m_startXSpin->setEnabled(false);
        m_startYSpin->setEnabled(false);
        m_endXSpin->setEnabled(false);
        m_endYSpin->setEnabled(false);
        m_lengthSpin->setEnabled(false);
        m_angleSpin->setEnabled(false);
        m_heightSpin->setEnabled(false);
        m_thicknessSpin->setEnabled(false);

        QSignalBlocker blockStartX(m_startXSpin);
        QSignalBlocker blockStartY(m_startYSpin);
        QSignalBlocker blockEndX(m_endXSpin);
        QSignalBlocker blockEndY(m_endYSpin);
        QSignalBlocker blockLength(m_lengthSpin);
        QSignalBlocker blockAngle(m_angleSpin);
        QSignalBlocker blockHeight(m_heightSpin);
        QSignalBlocker blockThickness(m_thicknessSpin);
        m_startXSpin->setValue(0.0);
        m_startYSpin->setValue(0.0);
        m_endXSpin->setValue(0.0);
        m_endYSpin->setValue(0.0);
        m_lengthSpin->setValue(0.0);
        m_angleSpin->setValue(0.0);
        m_heightSpin->setValue(0.0);
        m_thicknessSpin->setValue(0.0);
        return;
    }

    m_nameValue->setText(tr("墙体"));
    if (m_deleteAction) {
        m_deleteAction->setEnabled(true);
    }
    m_startXSpin->setEnabled(true);
    m_startYSpin->setEnabled(true);
    m_endXSpin->setEnabled(true);
    m_endYSpin->setEnabled(true);
    m_lengthSpin->setEnabled(true);
    m_angleSpin->setEnabled(true);
    m_heightSpin->setEnabled(true);
    m_thicknessSpin->setEnabled(true);

    QSignalBlocker blockStartX(m_startXSpin);
    QSignalBlocker blockStartY(m_startYSpin);
    QSignalBlocker blockEndX(m_endXSpin);
    QSignalBlocker blockEndY(m_endYSpin);
    QSignalBlocker blockLength(m_lengthSpin);
    QSignalBlocker blockAngle(m_angleSpin);
    QSignalBlocker blockHeight(m_heightSpin);
    QSignalBlocker blockThickness(m_thicknessSpin);
    m_startXSpin->setValue(wall->startPos().x());
    m_startYSpin->setValue(wall->startPos().y());
    m_endXSpin->setValue(wall->endPos().x());
    m_endYSpin->setValue(wall->endPos().y());
    m_lengthSpin->setValue(wall->length());
    m_angleSpin->setValue(wall->angleDegrees());
    m_heightSpin->setValue(wall->height());
    m_thicknessSpin->setValue(wall->thickness());
}

void MainWindow::handleCalibration(qreal measuredLength)
{
    bool ok = false;
    const double actualLength = QInputDialog::getDouble(
        this,
        tr("比例尺校准"),
        tr("输入实际长度 (mm)"),
        measuredLength,
        1.0,
        1000000.0,
        1,
        &ok);

    if (!ok) {
        m_scene->setMode(DesignScene::Mode_Select);
        return;
    }

    m_scene->applyCalibration(actualLength);
}

void MainWindow::updateToolHint(DesignScene::Mode mode)
{
    switch (mode) {
    case DesignScene::Mode_Select:
        m_hintLabel->setText(tr("操作: 选择/编辑 (拖动墙体或端点)"));
        if (m_selectAction) {
            m_selectAction->setChecked(true);
        }
        break;
    case DesignScene::Mode_DrawWall:
        m_hintLabel->setText(tr("操作: 墙体绘制 (左键连点, 右键结束, Shift 正交, 输入数字+回车定长)"));
        if (m_drawWallAction) {
            m_drawWallAction->setChecked(true);
        }
        break;
    case DesignScene::Mode_Calibrate:
        m_hintLabel->setText(tr("操作: 底图校准 (点击两点)"));
        if (m_calibrateAction) {
            m_calibrateAction->setChecked(true);
        }
        break;
    }
}

WallItem *MainWindow::selectedWall() const
{
    const QList<QGraphicsItem *> items = m_scene->selectedItems();
    if (items.isEmpty()) {
        return nullptr;
    }

    return qgraphicsitem_cast<WallItem *>(items.first());
}
