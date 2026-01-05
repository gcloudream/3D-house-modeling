#ifndef DESIGNSCENE_H
#define DESIGNSCENE_H

#include <QGraphicsScene>
#include <QList>
#include <QPixmap>
#include <QString>
#include <QVector2D>

class BlueprintItem;
class WallItem;
class OpeningItem;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QGraphicsSimpleTextItem;

class DesignScene : public QGraphicsScene
{
    Q_OBJECT

public:
    enum Mode {
        Mode_Select,
        Mode_DrawWall,
        Mode_Calibrate
    };

    explicit DesignScene(QObject *parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const;

    bool setBlueprintPixmap(const QPixmap &pixmap);
    void setBlueprintOpacity(qreal opacity);
    bool hasBlueprint() const;

    void setWallDefaults(qreal thickness, qreal height);
    void applyCalibration(qreal actualLengthMm);
    void applyWallHeightToAllWalls(qreal height);
    void notifySceneChanged();

    void setSnapEnabled(bool enabled);
    bool snapEnabled() const;
    void setSnapToGridEnabled(bool enabled);
    bool snapToGridEnabled() const;

signals:
    void sceneContentChanged();
    void modeChanged(Mode mode);
    void calibrationRequested(qreal measuredLength);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

private:
    enum EditHandle {
        Handle_None,
        Handle_Start,
        Handle_End
    };

    QPointF applyConstraints(const QPointF &rawPos,
                             const QPointF &anchor,
                             bool orthogonal,
                             bool *snapped);
    QPointF snapPosition(const QPointF &pos, bool *snapped);
    QList<WallItem *> wallItems() const;
    void updateSnapIndicator(const QPointF &pos, bool visible);
    void updateLengthIndicator();
    bool tryBeginEdit(const QPointF &scenePos);
    bool tryBeginDrag(const QPointF &scenePos);

    void finalizeWall(const QPointF &endPos, bool applyEndPos);
    void resetCalibration();
    WallItem *findWallNear(const QPointF &pos,
                           qreal maxDistance,
                           qreal *distanceAlong = nullptr) const;
    void updateHoverWall(WallItem *wall);
    void clearOpeningPreview();

    Mode m_mode;
    BlueprintItem *m_blueprintItem;
    WallItem *m_activeWall;
    WallItem *m_editWall;
    WallItem *m_dragWall;
    EditHandle m_editHandle;
    QGraphicsLineItem *m_calibrationLine;
    QGraphicsEllipseItem *m_snapIndicator;
    QPointF m_calibrationStart;
    QPointF m_dragLastPos;
    qreal m_lastCalibrationLength;
    qreal m_wallThickness;
    qreal m_wallHeight;
    bool m_snapEnabled;
    bool m_snapToGridEnabled;
    qreal m_snapTolerance;
    qreal m_gridSize;
    QPointF m_lastMousePos;
    QString m_lengthInput;
    QGraphicsSimpleTextItem *m_lengthIndicator;
    QVector2D m_lastDirection;
    OpeningItem *m_previewOpening;
    WallItem *m_hoverWall;
};

#endif // DESIGNSCENE_H
