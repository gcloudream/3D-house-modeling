#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "designscene.h"
class View2DWidget;
class WallItem;
class View3DWidget;
class OpeningItem;
class FurnitureItem;
class BlueprintItem;
class ComponentListWidget;
class QLabel;
class QListWidget;
class QComboBox;
class QDoubleSpinBox;
class QSlider;
class QAction;
class QDockWidget;
class QGroupBox;
class QMenu;
class QTimer;
class QCloseEvent;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupCentralView();
    void setupDocks();
    void setupToolbar();
    void setupStatusBar();
    void setupPreview3D();
    void connectSignals();
    void importBlueprint();
    void updateWindowTitle();
    void rebuildRecentFilesMenu();
    void checkAutosaveRecovery();
    bool confirmSave();
    void newProject();
    void openProject();
    void openRecentProject(const QString &path);
    bool saveProject();
    bool saveProjectAs();
    void exportImage();
    void updateSelectionDetails();
    void handleCalibration(qreal measuredLength);
    void updateToolHint(DesignScene::Mode mode);
    WallItem *selectedWall() const;
    OpeningItem *selectedOpening() const;
    FurnitureItem *selectedFurniture() const;
    BlueprintItem *selectedBlueprint() const;

    Ui::MainWindow *ui;
    DesignScene *m_scene;
    View2DWidget *m_view2d;
    View3DWidget *m_view3d;
    QDockWidget *m_previewDock;
    QLabel *m_coordLabel;
    QLabel *m_zoomLabel;
    QLabel *m_hintLabel;
    ComponentListWidget *m_componentList;
    ComponentListWidget *m_furnitureList;
    QComboBox *m_furnitureCategory;
    QGroupBox *m_blueprintGroup;
    QGroupBox *m_wallGroup;
    QGroupBox *m_openingGroup;
    QGroupBox *m_furnitureGroup;
    QLabel *m_nameValue;
    QDoubleSpinBox *m_blueprintWidthSpin;
    QDoubleSpinBox *m_blueprintLengthSpin;
    QDoubleSpinBox *m_blueprintAngleSpin;
    QDoubleSpinBox *m_startXSpin;
    QDoubleSpinBox *m_startYSpin;
    QDoubleSpinBox *m_endXSpin;
    QDoubleSpinBox *m_endYSpin;
    QDoubleSpinBox *m_lengthSpin;
    QDoubleSpinBox *m_angleSpin;
    QDoubleSpinBox *m_heightSpin;
    QDoubleSpinBox *m_thicknessSpin;
    QLabel *m_openingTypeValue;
    QDoubleSpinBox *m_openingWidthSpin;
    QDoubleSpinBox *m_openingHeightSpin;
    QDoubleSpinBox *m_openingSillSpin;
    QDoubleSpinBox *m_openingOffsetSpin;
    QLabel *m_furnitureNameValue;
    QDoubleSpinBox *m_furnitureWidthSpin;
    QDoubleSpinBox *m_furnitureDepthSpin;
    QDoubleSpinBox *m_furnitureHeightSpin;
    QDoubleSpinBox *m_furnitureElevationSpin;
    QDoubleSpinBox *m_furnitureRotationSpin;
    QSlider *m_blueprintOpacitySlider;
    QAction *m_newAction;
    QAction *m_openAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_exportAction;
    QAction *m_selectAction;
    QAction *m_drawWallAction;
    QAction *m_calibrateAction;
    QAction *m_snapAction;
    QAction *m_deleteAction;
    QMenu *m_recentMenu;
    QTimer *m_autosaveTimer;
};
#endif // MAINWINDOW_H
