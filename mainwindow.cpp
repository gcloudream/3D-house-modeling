#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "assetmanager.h"
#include "blueprintitem.h"
#include "componentlistwidget.h"
#include "designscene.h"
#include "furnitureitem.h"
#include "openingitem.h"
#include "view2dwidget.h"
#include "view3dwidget.h"
#include "wallitem.h"

#include <QActionGroup>
#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsItem>
#include <QIcon>
#include <QInputDialog>
#include <QLineF>
#include <QFont>
#include <QFrame>
#include <QGroupBox>
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
#include <QJsonDocument>
#include <QJsonObject>

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
    , m_furnitureList(nullptr)
    , m_furnitureCategory(nullptr)
    , m_blueprintGroup(nullptr)
    , m_wallGroup(nullptr)
    , m_openingGroup(nullptr)
    , m_furnitureGroup(nullptr)
    , m_nameValue(nullptr)
    , m_blueprintWidthSpin(nullptr)
    , m_blueprintLengthSpin(nullptr)
    , m_blueprintAngleSpin(nullptr)
    , m_startXSpin(nullptr)
    , m_startYSpin(nullptr)
    , m_endXSpin(nullptr)
    , m_endYSpin(nullptr)
    , m_lengthSpin(nullptr)
    , m_angleSpin(nullptr)
    , m_heightSpin(nullptr)
    , m_thicknessSpin(nullptr)
    , m_openingTypeValue(nullptr)
    , m_openingWidthSpin(nullptr)
    , m_openingHeightSpin(nullptr)
    , m_openingSillSpin(nullptr)
    , m_openingOffsetSpin(nullptr)
    , m_furnitureNameValue(nullptr)
    , m_furnitureWidthSpin(nullptr)
    , m_furnitureDepthSpin(nullptr)
    , m_furnitureHeightSpin(nullptr)
    , m_furnitureElevationSpin(nullptr)
    , m_furnitureRotationSpin(nullptr)
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
    // Disconnect all signals from m_scene before destruction
    // to prevent accessing destroyed objects during cleanup
    if (m_scene) {
        disconnect(m_scene, nullptr, this, nullptr);
    }

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
    m_componentList = new ComponentListWidget(libraryDock);
    m_componentList->setObjectName("componentList");
    m_componentList->setSpacing(4);
    m_componentList->setIconSize(QSize(24, 24));

    auto addHeader = [this](const QString &text) {
        auto *item = new QListWidgetItem(text, m_componentList);
        item->setFlags(Qt::ItemIsEnabled);
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);
        item->setForeground(QColor(47, 110, 143));
        item->setData(Qt::UserRole, QByteArray());
    };

    auto addOpeningItem = [this](const QString &text,
                                 const QString &kind,
                                 const QString &style,
                                 int width,
                                 int height,
                                 int sill,
                                 const QString &iconPath) {
        QJsonObject obj;
        obj["kind"] = kind;
        obj["style"] = style;
        obj["width"] = width;
        obj["height"] = height;
        obj["sill"] = sill;
        QListWidgetItem *item =
            new QListWidgetItem(QIcon(iconPath), text, m_componentList);        
        item->setData(Qt::UserRole,
                      QJsonDocument(obj).toJson(QJsonDocument::Compact));       
        item->setData(Qt::UserRole + 1, ComponentListWidget::openingMimeType());
    };

    addHeader(tr("墙体"));
    auto *wallTip = new QListWidgetItem(tr("使用工具栏绘制墙体"), m_componentList);
    wallTip->setFlags(Qt::ItemIsEnabled);
    wallTip->setForeground(QColor(121, 133, 147));
    wallTip->setData(Qt::UserRole, QByteArray());

    addHeader(tr("门"));
    addOpeningItem(tr("单开门 64x150"), "door", "single", 64, 150, 0,
                   ":/icons/door_single.svg");
    addOpeningItem(tr("双开门 114x150"), "door", "double", 114, 150, 0,
                   ":/icons/door_double.svg");
    addOpeningItem(tr("推拉门 86x150"), "door", "sliding", 86, 150, 0,
                   ":/icons/door_sliding.svg");

    addHeader(tr("窗"));
    addOpeningItem(tr("平开窗 100x100"), "window", "casement", 100, 100, 50,
                   ":/icons/window_casement.svg");
    addOpeningItem(tr("推拉窗 100x100"), "window", "sliding", 100, 100, 50,
                   ":/icons/window_sliding.svg");
    addOpeningItem(tr("飘窗 100x100"), "window", "bay", 100, 100, 50,
                   ":/icons/window_bay.svg");

    auto *libraryWidget = new QWidget(libraryDock);
    auto *libraryLayout = new QVBoxLayout(libraryWidget);
    libraryLayout->setContentsMargins(8, 8, 8, 8);
    libraryLayout->setSpacing(8);
    libraryLayout->addWidget(m_componentList);

    auto *furnitureLabel = new QLabel(tr("家具库"), libraryWidget);
    QFont headerFont = furnitureLabel->font();
    headerFont.setBold(true);
    furnitureLabel->setFont(headerFont);
    furnitureLabel->setStyleSheet("color: #2F6E8F;");
    libraryLayout->addWidget(furnitureLabel);

    m_furnitureCategory = new QComboBox(libraryWidget);
    m_furnitureCategory->setObjectName("furnitureCategory");
    libraryLayout->addWidget(m_furnitureCategory);

    m_furnitureList = new ComponentListWidget(libraryWidget);
    m_furnitureList->setObjectName("furnitureList");
    m_furnitureList->setSpacing(4);
    m_furnitureList->setIconSize(QSize(26, 26));
    libraryLayout->addWidget(m_furnitureList, 1);

    libraryDock->setWidget(libraryWidget);
    libraryDock->setMinimumWidth(200);
    addDockWidget(Qt::LeftDockWidgetArea, libraryDock);

    QString assetError;
    const QString catalogPath = AssetManager::defaultCatalogPath();
    if (!AssetManager::instance()->loadAssets(catalogPath, &assetError)) {
        QMessageBox::warning(this,
                             tr("资源加载失败"),
                             tr("无法加载家具 catalog：%1").arg(assetError));
    }

    auto categoryLabel = [](const QString &category) {
        if (category == "living_room") {
            return QObject::tr("客厅");
        }
        if (category == "bedroom") {
            return QObject::tr("卧室");
        }
        if (category == "kitchen") {
            return QObject::tr("厨房");
        }
        if (category == "bathroom") {
            return QObject::tr("卫生间");
        }
        if (category == "study") {
            return QObject::tr("书房");
        }
        return category;
    };

    if (m_furnitureCategory) {
        m_furnitureCategory->addItem(tr("全部"), QString());
        const QStringList categories = AssetManager::instance()->categories();
        for (const QString &category : categories) {
            m_furnitureCategory->addItem(categoryLabel(category), category);
        }
    }

    auto populateFurniture = [this](const QString &categoryId) {
        if (!m_furnitureList) {
            return;
        }
        m_furnitureList->clear();

        QList<AssetManager::Asset> assets;
        if (categoryId.isEmpty()) {
            const QStringList categories = AssetManager::instance()->categories();
            for (const QString &category : categories) {
                assets.append(AssetManager::instance()->getAssetsByCategory(category));
            }
        } else {
            assets = AssetManager::instance()->getAssetsByCategory(categoryId);
        }

        for (const AssetManager::Asset &asset : assets) {
            QListWidgetItem *item =
                new QListWidgetItem(QIcon(asset.svgPath), asset.name, m_furnitureList);
            QJsonObject obj;
            obj["assetId"] = asset.id;
            item->setData(Qt::UserRole,
                          QJsonDocument(obj).toJson(QJsonDocument::Compact));
            item->setData(Qt::UserRole + 1, ComponentListWidget::furnitureMimeType());
            item->setToolTip(
                tr("%1 x %2 x %3 mm")
                    .arg(asset.defaultSize.x(), 0, 'f', 0)
                    .arg(asset.defaultSize.y(), 0, 'f', 0)
                    .arg(asset.defaultSize.z(), 0, 'f', 0));
        }
    };

    if (m_furnitureCategory) {
        connect(m_furnitureCategory,
                qOverload<int>(&QComboBox::currentIndexChanged),
                this,
                [this, populateFurniture](int index) {
                    const QString categoryId =
                        m_furnitureCategory->itemData(index).toString();
                    populateFurniture(categoryId);
                });
    }

    populateFurniture(QString());

    auto *propertiesDock = new QDockWidget(tr("属性面板"), this);
    propertiesDock->setObjectName("propertiesDock");
    auto *propertiesWidget = new QWidget(propertiesDock);
    auto *propertiesLayout = new QVBoxLayout(propertiesWidget);
    propertiesLayout->setContentsMargins(10, 10, 10, 10);

    m_blueprintGroup = new QGroupBox(tr("底图属性"), propertiesWidget);
    auto *blueprintLayout = new QFormLayout(m_blueprintGroup);
    m_blueprintWidthSpin = new QDoubleSpinBox(m_blueprintGroup);
    m_blueprintLengthSpin = new QDoubleSpinBox(m_blueprintGroup);
    m_blueprintAngleSpin = new QDoubleSpinBox(m_blueprintGroup);

    for (QDoubleSpinBox *spin : {m_blueprintWidthSpin, m_blueprintLengthSpin}) {
        spin->setRange(1.0, 1000000.0);
        spin->setDecimals(0);
        spin->setSuffix(" mm");
        spin->setEnabled(false);
    }

    m_blueprintAngleSpin->setRange(0.0, 360.0);
    m_blueprintAngleSpin->setDecimals(1);
    m_blueprintAngleSpin->setSuffix(" 度");
    m_blueprintAngleSpin->setEnabled(false);

    m_blueprintOpacitySlider = new QSlider(Qt::Horizontal, m_blueprintGroup);
    m_blueprintOpacitySlider->setRange(0, 100);
    m_blueprintOpacitySlider->setValue(60);
    m_blueprintOpacitySlider->setEnabled(false);

    blueprintLayout->addRow(tr("宽度"), m_blueprintWidthSpin);
    blueprintLayout->addRow(tr("长度"), m_blueprintLengthSpin);
    blueprintLayout->addRow(tr("角度"), m_blueprintAngleSpin);
    blueprintLayout->addRow(tr("透明度"), m_blueprintOpacitySlider);

    m_wallGroup = new QGroupBox(tr("墙体属性"), propertiesWidget);
    auto *wallLayout = new QFormLayout(m_wallGroup);
    m_nameValue = new QLabel("-", m_wallGroup);
    m_lengthSpin = new QDoubleSpinBox(m_wallGroup);
    m_heightSpin = new QDoubleSpinBox(m_wallGroup);
    m_thicknessSpin = new QDoubleSpinBox(m_wallGroup);
    m_startXSpin = new QDoubleSpinBox(m_wallGroup);
    m_startYSpin = new QDoubleSpinBox(m_wallGroup);
    m_endXSpin = new QDoubleSpinBox(m_wallGroup);
    m_endYSpin = new QDoubleSpinBox(m_wallGroup);
    m_angleSpin = new QDoubleSpinBox(m_wallGroup);

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
    m_thicknessSpin->setValue(30.0);

    m_angleSpin->setRange(0.0, 360.0);
    m_angleSpin->setDecimals(1);
    m_angleSpin->setSuffix(" °");

    m_lengthSpin->setEnabled(false);
    m_heightSpin->setEnabled(false);
    m_thicknessSpin->setEnabled(false);
    m_angleSpin->setEnabled(false);

    wallLayout->addRow(tr("名称"), m_nameValue);
    wallLayout->addRow(tr("起点 X"), m_startXSpin);
    wallLayout->addRow(tr("起点 Y"), m_startYSpin);
    wallLayout->addRow(tr("终点 X"), m_endXSpin);
    wallLayout->addRow(tr("终点 Y"), m_endYSpin);
    wallLayout->addRow(tr("长度"), m_lengthSpin);
    wallLayout->addRow(tr("角度"), m_angleSpin);
    wallLayout->addRow(tr("高度"), m_heightSpin);
    wallLayout->addRow(tr("厚度"), m_thicknessSpin);

    m_openingGroup = new QGroupBox(tr("门窗属性"), propertiesWidget);
    auto *openingLayout = new QFormLayout(m_openingGroup);
    m_openingTypeValue = new QLabel("-", m_openingGroup);
    m_openingWidthSpin = new QDoubleSpinBox(m_openingGroup);
    m_openingHeightSpin = new QDoubleSpinBox(m_openingGroup);
    m_openingSillSpin = new QDoubleSpinBox(m_openingGroup);
    m_openingOffsetSpin = new QDoubleSpinBox(m_openingGroup);

    for (QDoubleSpinBox *spin : {m_openingWidthSpin, m_openingHeightSpin,
                                 m_openingSillSpin, m_openingOffsetSpin}) {
        spin->setDecimals(0);
        spin->setSuffix(" mm");
        spin->setEnabled(false);
    }

    m_openingWidthSpin->setRange(100.0, 10000.0);
    m_openingHeightSpin->setRange(100.0, 10000.0);
    m_openingSillSpin->setRange(0.0, 10000.0);
    m_openingOffsetSpin->setRange(0.0, 100000.0);

    openingLayout->addRow(tr("类型"), m_openingTypeValue);
    openingLayout->addRow(tr("宽度"), m_openingWidthSpin);
    openingLayout->addRow(tr("高度"), m_openingHeightSpin);
    openingLayout->addRow(tr("离地"), m_openingSillSpin);
    openingLayout->addRow(tr("距起点"), m_openingOffsetSpin);

    m_furnitureGroup = new QGroupBox(tr("家具属性"), propertiesWidget);
    auto *furnitureLayout = new QFormLayout(m_furnitureGroup);
    m_furnitureNameValue = new QLabel("-", m_furnitureGroup);
    m_furnitureWidthSpin = new QDoubleSpinBox(m_furnitureGroup);
    m_furnitureDepthSpin = new QDoubleSpinBox(m_furnitureGroup);
    m_furnitureHeightSpin = new QDoubleSpinBox(m_furnitureGroup);
    m_furnitureElevationSpin = new QDoubleSpinBox(m_furnitureGroup);
    m_furnitureRotationSpin = new QDoubleSpinBox(m_furnitureGroup);

    for (QDoubleSpinBox *spin : {m_furnitureWidthSpin,
                                 m_furnitureDepthSpin,
                                 m_furnitureHeightSpin,
                                 m_furnitureElevationSpin}) {
        spin->setRange(10.0, 20000.0);
        spin->setDecimals(0);
        spin->setSuffix(" mm");
        spin->setEnabled(false);
    }

    m_furnitureRotationSpin->setRange(0.0, 360.0);
    m_furnitureRotationSpin->setDecimals(1);
    m_furnitureRotationSpin->setSuffix(" 度");
    m_furnitureRotationSpin->setEnabled(false);

    furnitureLayout->addRow(tr("名称"), m_furnitureNameValue);
    furnitureLayout->addRow(tr("宽度"), m_furnitureWidthSpin);
    furnitureLayout->addRow(tr("深度"), m_furnitureDepthSpin);
    furnitureLayout->addRow(tr("高度"), m_furnitureHeightSpin);
    furnitureLayout->addRow(tr("离地"), m_furnitureElevationSpin);
    furnitureLayout->addRow(tr("角度"), m_furnitureRotationSpin);

    propertiesLayout->addWidget(m_blueprintGroup);
    propertiesLayout->addWidget(m_wallGroup);
    propertiesLayout->addWidget(m_openingGroup);
    propertiesLayout->addWidget(m_furnitureGroup);
    propertiesLayout->addStretch();
    m_blueprintGroup->setVisible(false);
    m_openingGroup->setVisible(false);
    m_furnitureGroup->setVisible(false);

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
            auto *opening = qgraphicsitem_cast<OpeningItem *>(item);
            if (opening) {
                WallItem *wall = opening->wall();
                if (wall) {
                    wall->removeOpening(opening);
                }
                m_scene->removeItem(opening);
                delete opening;
                removed = true;
                continue;
            }
            auto *furniture = qgraphicsitem_cast<FurnitureItem *>(item);
            if (furniture) {
                m_scene->removeItem(furniture);
                delete furniture;
                removed = true;
                continue;
            }
            auto *wall = qgraphicsitem_cast<WallItem *>(item);
            if (!wall) {
                continue;
            }
            const QList<OpeningItem *> openings = wall->openings();
            for (OpeningItem *child : openings) {
                if (child) {
                    wall->removeOpening(child);
                    m_scene->removeItem(child);
                    delete child;
                }
            }
            m_scene->removeItem(wall);
            delete wall;
            removed = true;
        }
        if (!removed) {
            QMessageBox::information(this, tr("提示"), tr("请先选择墙体、门窗或家具。"));
        }
        // ✅ 删除后先清除选择，再通知场景变化
        m_scene->clearSelection();
        m_scene->notifySceneChanged();
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
    auto *viewMenu = menuBar()->addMenu(tr("视图"));
    auto *previewToggleAction = m_previewDock->toggleViewAction();
    previewToggleAction->setText(tr("3D 预览"));
    viewMenu->addAction(previewToggleAction);

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

    connect(m_blueprintWidthSpin,
            qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                BlueprintItem *blueprint = selectedBlueprint();
                if (!blueprint) {
                    return;
                }
                const QPixmap pixmap = blueprint->pixmap();
                if (pixmap.isNull() || pixmap.width() <= 0) {
                    return;
                }
                blueprint->setScale(value / pixmap.width());
                m_scene->notifySceneChanged();
            });

    connect(m_blueprintLengthSpin,
            qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                BlueprintItem *blueprint = selectedBlueprint();
                if (!blueprint) {
                    return;
                }
                const QPixmap pixmap = blueprint->pixmap();
                if (pixmap.isNull() || pixmap.height() <= 0) {
                    return;
                }
                blueprint->setScale(value / pixmap.height());
                m_scene->notifySceneChanged();
            });

    connect(m_blueprintAngleSpin,
            qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                BlueprintItem *blueprint = selectedBlueprint();
                if (!blueprint) {
                    return;
                }
                blueprint->setRotation(value);
                m_scene->notifySceneChanged();
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

    connect(m_openingWidthSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
                OpeningItem *opening = selectedOpening();
                if (!opening) {
                    return;
                }
                opening->setWidth(value);
                m_scene->notifySceneChanged();
            });

    connect(m_openingHeightSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
                OpeningItem *opening = selectedOpening();
                if (!opening) {
                    return;
                }
                opening->setHeight(value);
                m_scene->notifySceneChanged();
            });

    connect(m_openingSillSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
                OpeningItem *opening = selectedOpening();
                if (!opening) {
                    return;
                }
                opening->setSillHeight(value);
                m_scene->notifySceneChanged();
            });

    connect(m_openingOffsetSpin,
            qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                OpeningItem *opening = selectedOpening();
                if (!opening) {
                    return;
                }
                opening->setDistanceFromStart(value);
                m_scene->notifySceneChanged();
            });

    connect(m_furnitureWidthSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
                FurnitureItem *furniture = selectedFurniture();
                if (!furniture) {
                    return;
                }
                QSizeF size = furniture->size2D();
                size.setWidth(value);
                furniture->setSize2D(size);
                m_scene->notifySceneChanged();
            });

    connect(m_furnitureDepthSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
                FurnitureItem *furniture = selectedFurniture();
                if (!furniture) {
                    return;
                }
                QSizeF size = furniture->size2D();
                size.setHeight(value);
                furniture->setSize2D(size);
                m_scene->notifySceneChanged();
            });

    connect(m_furnitureHeightSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
                FurnitureItem *furniture = selectedFurniture();
                if (!furniture) {
                    return;
                }
                furniture->setHeight3D(value);
                m_scene->notifySceneChanged();
            });

    connect(m_furnitureElevationSpin,
            qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                FurnitureItem *furniture = selectedFurniture();
                if (!furniture) {
                    return;
                }
                furniture->setElevation(value);
                m_scene->notifySceneChanged();
            });

    connect(m_furnitureRotationSpin,
            qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                FurnitureItem *furniture = selectedFurniture();
                if (!furniture) {
                    return;
                }
                furniture->setRotationDegrees(value);
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
    OpeningItem *opening = selectedOpening();
    FurnitureItem *furniture = selectedFurniture();
    WallItem *wall = selectedWall();
    BlueprintItem *blueprint = selectedBlueprint();

    // ✅ 优先级处理：
    // 1. 家具优先级最高（独立对象）
    // 2. 如果墙体和门窗同时选中，优先显示墙体（门窗是墙体的附属）
    if (wall && opening) {
        opening = nullptr;
    }
    if ((wall || opening || furniture) && blueprint) {
        blueprint = nullptr;
    }

    // ✅ 按优先级检查：家具 > 门窗 > 墙体
    if (furniture) {
        m_blueprintGroup->setVisible(false);
        m_wallGroup->setVisible(false);
        m_openingGroup->setVisible(false);
        m_furnitureGroup->setVisible(true);

        if (m_deleteAction) {
            m_deleteAction->setEnabled(true);
        }

        m_furnitureWidthSpin->setEnabled(true);
        m_furnitureDepthSpin->setEnabled(true);
        m_furnitureHeightSpin->setEnabled(true);
        m_furnitureElevationSpin->setEnabled(true);
        m_furnitureRotationSpin->setEnabled(true);

        QSignalBlocker blockWidth(m_furnitureWidthSpin);
        QSignalBlocker blockDepth(m_furnitureDepthSpin);
        QSignalBlocker blockHeight(m_furnitureHeightSpin);
        QSignalBlocker blockElevation(m_furnitureElevationSpin);
        QSignalBlocker blockRotation(m_furnitureRotationSpin);
        const QString displayName = furniture->asset().name.isEmpty()
            ? furniture->assetId()
            : furniture->asset().name;
        m_furnitureNameValue->setText(displayName);
        m_furnitureWidthSpin->setValue(furniture->size2D().width());
        m_furnitureDepthSpin->setValue(furniture->size2D().height());
        m_furnitureHeightSpin->setValue(furniture->height3D());
        m_furnitureElevationSpin->setValue(furniture->elevation());
        m_furnitureRotationSpin->setValue(furniture->rotationDegrees());
        return;
    }

    if (opening) {
        m_blueprintGroup->setVisible(false);
        m_wallGroup->setVisible(false);
        m_openingGroup->setVisible(true);
        m_furnitureGroup->setVisible(false);

        m_openingTypeValue->setText(
            tr("%1 / %2").arg(opening->kindLabel(), opening->styleLabel()));
        if (m_deleteAction) {
            m_deleteAction->setEnabled(true);
        }

        const bool isWindow = opening->kind() == OpeningItem::Kind::Window;
        m_openingWidthSpin->setEnabled(true);
        m_openingHeightSpin->setEnabled(true);
        m_openingSillSpin->setEnabled(isWindow);
        m_openingOffsetSpin->setEnabled(true);

        QSignalBlocker blockWidth(m_openingWidthSpin);
        QSignalBlocker blockHeight(m_openingHeightSpin);
        QSignalBlocker blockSill(m_openingSillSpin);
        QSignalBlocker blockOffset(m_openingOffsetSpin);
        m_openingWidthSpin->setValue(opening->width());
        m_openingHeightSpin->setValue(opening->height());
        m_openingSillSpin->setValue(opening->sillHeight());
        m_openingOffsetSpin->setValue(opening->distanceFromStart());
        return;
    }

    if (blueprint) {
        m_blueprintGroup->setVisible(true);
        m_wallGroup->setVisible(false);
        m_openingGroup->setVisible(false);
        m_furnitureGroup->setVisible(false);

        if (m_deleteAction) {
            m_deleteAction->setEnabled(false);
        }

        m_blueprintWidthSpin->setEnabled(true);
        m_blueprintLengthSpin->setEnabled(true);
        m_blueprintAngleSpin->setEnabled(true);
        m_blueprintOpacitySlider->setEnabled(true);

        QSignalBlocker blockWidth(m_blueprintWidthSpin);
        QSignalBlocker blockLength(m_blueprintLengthSpin);
        QSignalBlocker blockAngle(m_blueprintAngleSpin);
        QSignalBlocker blockOpacity(m_blueprintOpacitySlider);
        const QPixmap pixmap = blueprint->pixmap();
        const qreal scale = blueprint->scale();
        const qreal width = pixmap.isNull() ? 0.0 : pixmap.width() * scale;
        const qreal length = pixmap.isNull() ? 0.0 : pixmap.height() * scale;
        m_blueprintWidthSpin->setValue(width);
        m_blueprintLengthSpin->setValue(length);
        m_blueprintAngleSpin->setValue(blueprint->rotation());
        m_blueprintOpacitySlider->setValue(qRound(blueprint->opacity() * 100.0));
        return;
    }

    if (furniture) {
        m_blueprintGroup->setVisible(false);
        m_wallGroup->setVisible(false);
        m_openingGroup->setVisible(false);
        m_furnitureGroup->setVisible(true);

        if (m_deleteAction) {
            m_deleteAction->setEnabled(true);
        }

        m_furnitureWidthSpin->setEnabled(true);
        m_furnitureDepthSpin->setEnabled(true);
        m_furnitureHeightSpin->setEnabled(true);
        m_furnitureElevationSpin->setEnabled(true);
        m_furnitureRotationSpin->setEnabled(true);

        QSignalBlocker blockWidth(m_furnitureWidthSpin);
        QSignalBlocker blockDepth(m_furnitureDepthSpin);
        QSignalBlocker blockHeight(m_furnitureHeightSpin);
        QSignalBlocker blockElevation(m_furnitureElevationSpin);
        QSignalBlocker blockRotation(m_furnitureRotationSpin);
        const QString displayName = furniture->asset().name.isEmpty()
            ? furniture->assetId()
            : furniture->asset().name;
        m_furnitureNameValue->setText(displayName);
        m_furnitureWidthSpin->setValue(furniture->size2D().width());
        m_furnitureDepthSpin->setValue(furniture->size2D().height());
        m_furnitureHeightSpin->setValue(furniture->height3D());
        m_furnitureElevationSpin->setValue(furniture->elevation());
        m_furnitureRotationSpin->setValue(furniture->rotationDegrees());
        return;
    }

    m_openingGroup->setVisible(false);
    m_blueprintGroup->setVisible(false);
    m_wallGroup->setVisible(true);
    m_furnitureGroup->setVisible(false);

    if (!wall) {
        m_nameValue->setText("-");
        if (m_deleteAction) {
            m_deleteAction->setEnabled(false);
        }
        m_blueprintWidthSpin->setEnabled(false);
        m_blueprintLengthSpin->setEnabled(false);
        m_blueprintAngleSpin->setEnabled(false);
        m_blueprintOpacitySlider->setEnabled(false);
        m_startXSpin->setEnabled(false);
        m_startYSpin->setEnabled(false);
        m_endXSpin->setEnabled(false);
        m_endYSpin->setEnabled(false);
        m_lengthSpin->setEnabled(false);
        m_angleSpin->setEnabled(false);
        m_heightSpin->setEnabled(false);
        m_thicknessSpin->setEnabled(false);
        m_furnitureWidthSpin->setEnabled(false);
        m_furnitureDepthSpin->setEnabled(false);
        m_furnitureHeightSpin->setEnabled(false);
        m_furnitureElevationSpin->setEnabled(false);
        m_furnitureRotationSpin->setEnabled(false);

        QSignalBlocker blockStartX(m_startXSpin);
        QSignalBlocker blockStartY(m_startYSpin);
        QSignalBlocker blockEndX(m_endXSpin);
        QSignalBlocker blockEndY(m_endYSpin);
        QSignalBlocker blockLength(m_lengthSpin);
        QSignalBlocker blockAngle(m_angleSpin);
        QSignalBlocker blockHeight(m_heightSpin);
        QSignalBlocker blockThickness(m_thicknessSpin);
        QSignalBlocker blockBlueprintWidth(m_blueprintWidthSpin);
        QSignalBlocker blockBlueprintLength(m_blueprintLengthSpin);
        QSignalBlocker blockBlueprintAngle(m_blueprintAngleSpin);
        QSignalBlocker blockBlueprintOpacity(m_blueprintOpacitySlider);
        QSignalBlocker blockFurnitureWidth(m_furnitureWidthSpin);
        QSignalBlocker blockFurnitureDepth(m_furnitureDepthSpin);
        QSignalBlocker blockFurnitureHeight(m_furnitureHeightSpin);
        QSignalBlocker blockFurnitureElevation(m_furnitureElevationSpin);
        QSignalBlocker blockFurnitureRotation(m_furnitureRotationSpin);
        m_startXSpin->setValue(0.0);
        m_startYSpin->setValue(0.0);
        m_endXSpin->setValue(0.0);
        m_endYSpin->setValue(0.0);
        m_lengthSpin->setValue(0.0);
        m_angleSpin->setValue(0.0);
        m_heightSpin->setValue(0.0);
        m_thicknessSpin->setValue(0.0);
        m_blueprintWidthSpin->setValue(0.0);
        m_blueprintLengthSpin->setValue(0.0);
        m_blueprintAngleSpin->setValue(0.0);
        m_blueprintOpacitySlider->setValue(0);
        m_furnitureNameValue->setText("-");
        m_furnitureWidthSpin->setValue(0.0);
        m_furnitureDepthSpin->setValue(0.0);
        m_furnitureHeightSpin->setValue(0.0);
        m_furnitureElevationSpin->setValue(0.0);
        m_furnitureRotationSpin->setValue(0.0);
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
    for (QGraphicsItem *item : items) {
        WallItem *wall = qgraphicsitem_cast<WallItem *>(item);
        if (wall) {
            return wall;
        }
    }
    return nullptr;
}

OpeningItem *MainWindow::selectedOpening() const
{
    const QList<QGraphicsItem *> items = m_scene->selectedItems();
    for (QGraphicsItem *item : items) {
        OpeningItem *opening = qgraphicsitem_cast<OpeningItem *>(item);
        if (opening) {
            return opening;
        }
    }
    return nullptr;
}

FurnitureItem *MainWindow::selectedFurniture() const
{
    const QList<QGraphicsItem *> items = m_scene->selectedItems();
    for (QGraphicsItem *item : items) {
        FurnitureItem *furniture = qgraphicsitem_cast<FurnitureItem *>(item);
        if (furniture) {
            return furniture;
        }
    }
    return nullptr;
}

BlueprintItem *MainWindow::selectedBlueprint() const
{
    const QList<QGraphicsItem *> items = m_scene->selectedItems();
    for (QGraphicsItem *item : items) {
        BlueprintItem *blueprint = qgraphicsitem_cast<BlueprintItem *>(item);
        if (blueprint) {
            return blueprint;
        }
    }
    return nullptr;
}
