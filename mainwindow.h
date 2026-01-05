#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "designscene.h"
class View2DWidget;
class WallItem;
class View3DWidget;
class QLabel;
class QListWidget;
class QDoubleSpinBox;
class QSlider;
class QAction;
class QDockWidget;

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

private:
    void setupCentralView();
    void setupDocks();
    void setupToolbar();
    void setupStatusBar();
    void setupPreview3D();
    void connectSignals();
    void importBlueprint();
    void updateSelectionDetails();
    void handleCalibration(qreal measuredLength);
    void updateToolHint(DesignScene::Mode mode);
    WallItem *selectedWall() const;

    Ui::MainWindow *ui;
    DesignScene *m_scene;
    View2DWidget *m_view2d;
    View3DWidget *m_view3d;
    QDockWidget *m_previewDock;
    QLabel *m_coordLabel;
    QLabel *m_zoomLabel;
    QLabel *m_hintLabel;
    QListWidget *m_componentList;
    QLabel *m_nameValue;
    QDoubleSpinBox *m_startXSpin;
    QDoubleSpinBox *m_startYSpin;
    QDoubleSpinBox *m_endXSpin;
    QDoubleSpinBox *m_endYSpin;
    QDoubleSpinBox *m_lengthSpin;
    QDoubleSpinBox *m_angleSpin;
    QDoubleSpinBox *m_heightSpin;
    QDoubleSpinBox *m_thicknessSpin;
    QSlider *m_blueprintOpacitySlider;
    QAction *m_selectAction;
    QAction *m_drawWallAction;
    QAction *m_calibrateAction;
    QAction *m_snapAction;
    QAction *m_deleteAction;
};
#endif // MAINWINDOW_H
