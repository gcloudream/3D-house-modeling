#include "designscene.h"

#include "blueprintitem.h"
#include "componentlistwidget.h"
#include "dooritem.h"
#include "furnitureitem.h"
#include "openingitem.h"
#include "wallitem.h"
#include "windowitem.h"

#include <QBrush>
#include <QDateTime>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLineF>
#include <QList>
#include <QMimeData>
#include <QPen>
#include <QRectF>
#include <QTransform>
#include <QVector2D>
#include <QtGlobal>
#include <cmath>

DesignScene::DesignScene(QObject *parent)
    : QGraphicsScene(parent)
    , m_mode(Mode_Select)
    , m_blueprintItem(nullptr)
    , m_activeWall(nullptr)
    , m_editWall(nullptr)
    , m_dragWall(nullptr)
    , m_editHandle(Handle_None)
    , m_calibrationLine(nullptr)
    , m_snapIndicator(nullptr)
    , m_lengthIndicator(nullptr)
    , m_lastCalibrationLength(0.0)
    , m_wallThickness(30.0)
    , m_wallHeight(200.0)
    , m_snapEnabled(true)
    , m_snapToGridEnabled(true)
    , m_snapTolerance(10.0)
    , m_gridSize(100.0)
    , m_lastMousePos(0.0, 0.0)
    , m_lengthInput()
    , m_lastDirection(1.0f, 0.0f)
    , m_previewOpening(nullptr)
    , m_previewFurniture(nullptr)
    , m_hoverWall(nullptr)
    , m_projectCreatedAt()
{
    setSceneRect(-50000.0, -50000.0, 100000.0, 100000.0);
    initializeHelpers();
    m_projectCreatedAt = QDateTime::currentDateTime().toString(Qt::ISODate);

    connect(this, &QGraphicsScene::changed, this, [this](const QList<QRectF> &) {
        emit sceneContentChanged();
    });
}

void DesignScene::initializeHelpers()
{
    m_snapIndicator = addEllipse(QRectF(-4.0, -4.0, 8.0, 8.0),
                                 QPen(QColor(47, 110, 143), 1.5),
                                 QBrush(QColor(47, 110, 143, 90)));
    m_snapIndicator->setZValue(1000.0);
    m_snapIndicator->setVisible(false);

    m_lengthIndicator = addSimpleText(QString());
    m_lengthIndicator->setZValue(1001.0);
    m_lengthIndicator->setVisible(false);
    m_lengthIndicator->setBrush(QBrush(QColor(47, 110, 143)));
}

void DesignScene::setMode(DesignScene::Mode mode)
{
    if (m_mode == mode) {
        return;
    }

    clearOpeningPreview();
    clearFurniturePreview();

    if (m_mode == Mode_DrawWall && mode != Mode_DrawWall) {
        finalizeWall(QPointF(), false);
    }

    if (m_mode == Mode_Calibrate && mode != Mode_Calibrate) {
        resetCalibration();
    }

    m_mode = mode;
    m_editWall = nullptr;
    m_editHandle = Handle_None;
    m_dragWall = nullptr;
    m_lengthInput.clear();
    updateLengthIndicator();
    updateSnapIndicator(QPointF(), false);
    emit modeChanged(m_mode);
}

DesignScene::Mode DesignScene::mode() const
{
    return m_mode;
}

bool DesignScene::setBlueprintPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        return false;
    }

    if (!m_blueprintItem) {
        m_blueprintItem = new BlueprintItem(pixmap);
        addItem(m_blueprintItem);
    } else {
        m_blueprintItem->setPixmap(pixmap);
    }

    m_blueprintItem->setZValue(-100.0);
    m_blueprintItem->setTransformationMode(Qt::SmoothTransformation);
    m_blueprintItem->setOpacity(0.6);
    m_blueprintItem->setOffset(-pixmap.width() / 2.0, -pixmap.height() / 2.0);
    m_blueprintItem->setTransformOriginPoint(0.0, 0.0);
    m_blueprintItem->setPos(0.0, 0.0);
    m_blueprintItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    return true;
}

void DesignScene::setBlueprintOpacity(qreal opacity)
{
    if (!m_blueprintItem) {
        return;
    }

    const qreal clamped = qMax(0.0, qMin(1.0, opacity));
    m_blueprintItem->setOpacity(clamped);
}

bool DesignScene::hasBlueprint() const
{
    return m_blueprintItem != nullptr;
}

void DesignScene::setWallDefaults(qreal thickness, qreal height)
{
    m_wallThickness = thickness;
    m_wallHeight = height;
}

void DesignScene::applyCalibration(qreal actualLengthMm)
{
    if (!m_blueprintItem || actualLengthMm <= 0.0 || m_lastCalibrationLength <= 0.0) {
        resetCalibration();
        return;
    }

    const qreal factor = actualLengthMm / m_lastCalibrationLength;
    m_blueprintItem->setScale(m_blueprintItem->scale() * factor);
    resetCalibration();
    setMode(Mode_Select);
}

void DesignScene::applyWallHeightToAllWalls(qreal height)
{
    m_wallHeight = height;
    const QList<QGraphicsItem *> sceneItems = items();
    for (QGraphicsItem *item : sceneItems) {
        auto *wall = qgraphicsitem_cast<WallItem *>(item);
        if (!wall) {
            continue;
        }
        wall->setHeight(height);
    }

    emit sceneContentChanged();
}

void DesignScene::notifySceneChanged()
{
    emit sceneContentChanged();
}

QJsonObject DesignScene::toJson() const
{
    QJsonObject root;
    QJsonObject info;
    info["version"] = QStringLiteral("1.0");
    const QString createdAt = m_projectCreatedAt.isEmpty()
        ? QDateTime::currentDateTime().toString(Qt::ISODate)
        : m_projectCreatedAt;
    info["created_at"] = createdAt;
    root["project_info"] = info;

    QJsonObject settings;
    if (m_blueprintItem) {
        settings["background_image"] = m_blueprintItem->sourcePath();
        settings["pixel_ratio"] = m_blueprintItem->scale();
        settings["opacity"] = m_blueprintItem->opacity();
        settings["rotation"] = m_blueprintItem->rotation();
    } else {
        settings["background_image"] = QString();
        settings["pixel_ratio"] = 1.0;
        settings["opacity"] = 1.0;
        settings["rotation"] = 0.0;
    }
    root["settings"] = settings;

    QJsonArray wallsArray;
    for (WallItem *wall : walls()) {
        if (wall) {
            wallsArray.append(wall->toJson());
        }
    }
    root["walls"] = wallsArray;

    QJsonArray openingsArray;
    for (OpeningItem *opening : openings()) {
        if (opening) {
            openingsArray.append(opening->toJson());
        }
    }
    root["openings"] = openingsArray;

    QJsonArray furnitureArray;
    for (FurnitureItem *item : furniture()) {
        if (item) {
            furnitureArray.append(item->toJson());
        }
    }
    root["furniture"] = furnitureArray;

    return root;
}

void DesignScene::fromJson(const QJsonObject &root)
{
    resetScene();

    const QJsonObject info = root.value("project_info").toObject();
    m_projectCreatedAt = info.value("created_at").toString();
    if (m_projectCreatedAt.isEmpty()) {
        m_projectCreatedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    }

    const QJsonObject settings = root.value("settings").toObject();
    const QString backgroundPath = settings.value("background_image").toString();
    const qreal pixelRatio = settings.value("pixel_ratio").toDouble(1.0);
    const qreal opacity = settings.value("opacity").toDouble(1.0);
    const qreal rotation = settings.value("rotation").toDouble(0.0);

    if (!backgroundPath.isEmpty()) {
        QPixmap pixmap(backgroundPath);
        if (!pixmap.isNull() && setBlueprintPixmap(pixmap)) {
            if (m_blueprintItem) {
                m_blueprintItem->setSourcePath(backgroundPath);
                m_blueprintItem->setScale(pixelRatio > 0.0 ? pixelRatio : 1.0);
                m_blueprintItem->setOpacity(qBound(0.0, opacity, 1.0));
                m_blueprintItem->setRotation(rotation);
            }
        }
    }

    QHash<QString, WallItem *> wallMap;
    const QJsonArray wallsArray = root.value("walls").toArray();
    for (const QJsonValue &value : wallsArray) {
        if (!value.isObject()) {
            continue;
        }
        WallItem *wall = WallItem::fromJson(value.toObject());
        addItem(wall);
        wallMap.insert(wall->id(), wall);
    }

    const QJsonArray openingsArray = root.value("openings").toArray();
    for (const QJsonValue &value : openingsArray) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject obj = value.toObject();
        const QString wallId = obj.value("wall_id").toString();
        WallItem *wall = wallMap.value(wallId, nullptr);
        if (!wall) {
            continue;
        }
        OpeningItem *opening = OpeningItem::fromJson(obj, wall);
        addItem(opening);
        wall->addOpening(opening);
    }

    const QJsonArray furnitureArray = root.value("furniture").toArray();
    for (const QJsonValue &value : furnitureArray) {
        if (!value.isObject()) {
            continue;
        }
        FurnitureItem *item = FurnitureItem::fromJson(value.toObject());
        addItem(item);
    }
}

void DesignScene::resetScene()
{
    clear();
    setSceneRect(-50000.0, -50000.0, 100000.0, 100000.0);
    m_blueprintItem = nullptr;
    m_activeWall = nullptr;
    m_editWall = nullptr;
    m_dragWall = nullptr;
    m_editHandle = Handle_None;
    m_calibrationLine = nullptr;
    m_snapIndicator = nullptr;
    m_lengthIndicator = nullptr;
    m_previewOpening = nullptr;
    m_previewFurniture = nullptr;
    m_hoverWall = nullptr;
    m_lengthInput.clear();
    m_lastCalibrationLength = 0.0;
    m_wallThickness = 30.0;
    m_wallHeight = 200.0;
    m_snapEnabled = true;
    m_snapToGridEnabled = true;
    m_snapTolerance = 10.0;
    m_gridSize = 100.0;
    m_lastDirection = QVector2D(1.0f, 0.0f);
    m_projectCreatedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    initializeHelpers();
}

QList<WallItem *> DesignScene::walls() const
{
    QList<WallItem *> result;
    const QList<QGraphicsItem *> sceneItems = items();
    for (QGraphicsItem *item : sceneItems) {
        auto *wall = qgraphicsitem_cast<WallItem *>(item);
        if (!wall) {
            continue;
        }
        if (wall->length() < 0.1) {
            continue;
        }
        result.append(wall);
    }
    return result;
}

QList<OpeningItem *> DesignScene::openings() const
{
    QList<OpeningItem *> result;
    const QList<WallItem *> sceneWalls = walls();
    for (WallItem *wall : sceneWalls) {
        const QList<OpeningItem *> wallOpenings = wall->openings();
        for (OpeningItem *opening : wallOpenings) {
            if (!opening || opening->isPreview()) {
                continue;
            }
            result.append(opening);
        }
    }
    return result;
}

QList<FurnitureItem *> DesignScene::furniture() const
{
    QList<FurnitureItem *> result;
    const QList<QGraphicsItem *> sceneItems = items();
    for (QGraphicsItem *item : sceneItems) {
        auto *furnitureItem = qgraphicsitem_cast<FurnitureItem *>(item);
        if (!furnitureItem || furnitureItem->isPreview()) {
            continue;
        }
        result.append(furnitureItem);
    }
    return result;
}

BlueprintItem *DesignScene::blueprintItem() const
{
    return m_blueprintItem;
}

void DesignScene::setSnapEnabled(bool enabled)
{
    m_snapEnabled = enabled;
    if (!m_snapEnabled) {
        updateSnapIndicator(QPointF(), false);
    }
}

bool DesignScene::snapEnabled() const
{
    return m_snapEnabled;
}

void DesignScene::setSnapToGridEnabled(bool enabled)
{
    m_snapToGridEnabled = enabled;
}

bool DesignScene::snapToGridEnabled() const
{
    return m_snapToGridEnabled;
}

void DesignScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_mode == Mode_Select && event->button() == Qt::LeftButton) {
        if (tryBeginEdit(event->scenePos())) {
            event->accept();
            return;
        }
        if (tryBeginDrag(event->scenePos())) {
            event->accept();
            return;
        }
    }

    if (m_mode == Mode_DrawWall) {
        if (event->button() == Qt::LeftButton) {
            if (!m_lengthInput.isEmpty()) {
                m_lengthInput.clear();
                updateLengthIndicator();
            }
            const bool useOrtho = event->modifiers().testFlag(Qt::ShiftModifier);
            if (!m_activeWall) {
                bool snapped = false;
                const QPointF startPos =
                    applyConstraints(event->scenePos(), event->scenePos(), false, &snapped);
                clearSelection();
                m_activeWall = new WallItem(startPos,
                                            startPos,
                                            m_wallThickness,
                                            m_wallHeight);
                addItem(m_activeWall);
                m_activeWall->setSelected(true);
            } else {
                bool snapped = false;
                const QPointF endPos =
                    applyConstraints(event->scenePos(), m_activeWall->startPos(), useOrtho, &snapped);
                clearSelection();
                m_activeWall->setEndPos(endPos);
                m_activeWall->updateGeometry();

                m_activeWall = new WallItem(endPos,
                                            endPos,
                                            m_wallThickness,
                                            m_wallHeight);
                addItem(m_activeWall);
                m_activeWall->setSelected(true);
            }

            event->accept();
            return;
        }

        if (event->button() == Qt::RightButton) {
            bool snapped = false;
            QPointF endPos = event->scenePos();
            if (m_activeWall) {
                const bool useOrtho = event->modifiers().testFlag(Qt::ShiftModifier);
                endPos = applyConstraints(event->scenePos(), m_activeWall->startPos(), useOrtho, &snapped);
            }
            finalizeWall(endPos, true);
            updateSnapIndicator(QPointF(), false);
            event->accept();
            return;
        }
    }

    if (m_mode == Mode_Calibrate) {
        if (event->button() == Qt::LeftButton) {
            if (!m_blueprintItem) {
                event->accept();
                return;
            }

            if (!m_calibrationLine) {
                m_calibrationStart = event->scenePos();
                m_calibrationLine = addLine(QLineF(m_calibrationStart, m_calibrationStart),
                                            QPen(QColor(47, 110, 143), 1.5, Qt::DashLine));
            } else {
                QLineF line(m_calibrationStart, event->scenePos());
                m_lastCalibrationLength = line.length();
                if (m_lastCalibrationLength < 1.0) {
                    resetCalibration();
                    event->accept();
                    return;
                }
                emit calibrationRequested(m_lastCalibrationLength);
            }

            event->accept();
            return;
        }
    }

    QGraphicsScene::mousePressEvent(event);
}

void DesignScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    m_lastMousePos = event->scenePos();
    updateLengthIndicator();

    if (m_editHandle != Handle_None && m_editWall) {
        const bool useOrtho = event->modifiers().testFlag(Qt::ShiftModifier);
        const QPointF anchor = (m_editHandle == Handle_Start)
            ? m_editWall->endPos()
            : m_editWall->startPos();
        bool snapped = false;
        const QPointF constrained = applyConstraints(event->scenePos(), anchor, useOrtho, &snapped);

        if (m_editHandle == Handle_Start) {
            m_editWall->setStartPos(constrained);
        } else {
            m_editWall->setEndPos(constrained);
        }
        m_editWall->updateGeometry();
        event->accept();
        return;
    }

    if (m_dragWall) {
        const QPointF delta = event->scenePos() - m_dragLastPos;
        if (!delta.isNull()) {
            m_dragWall->setStartPos(m_dragWall->startPos() + delta);
            m_dragWall->setEndPos(m_dragWall->endPos() + delta);
            m_dragWall->updateGeometry();
            m_dragLastPos = event->scenePos();
        }
        event->accept();
        return;
    }

    if (m_mode == Mode_DrawWall && m_activeWall) {
        const bool useOrtho = event->modifiers().testFlag(Qt::ShiftModifier);
        if (!m_lengthInput.isEmpty()) {
            bool ok = false;
            const double length = m_lengthInput.toDouble(&ok);
            if (ok && length > 0.0) {
                const QPointF anchor = m_activeWall->startPos();
                QPointF delta = event->scenePos() - anchor;
                if (qAbs(delta.x()) < 0.001 && qAbs(delta.y()) < 0.001) {
                    delta = QPointF(m_lastDirection.x(), m_lastDirection.y());
                }
                if (useOrtho) {
                    if (qAbs(delta.x()) >= qAbs(delta.y())) {
                        delta.setY(0.0);
                    } else {
                        delta.setX(0.0);
                    }
                }
                QVector2D dir(delta);
                if (dir.lengthSquared() < 0.0001f) {
                    dir = m_lastDirection;
                } else {
                    dir.normalize();
                    m_lastDirection = dir;
                }

                const QPointF end =
                    anchor + QPointF(dir.x(), dir.y()) * static_cast<qreal>(length);
                m_activeWall->setEndPos(end);
                m_activeWall->updateGeometry();
                updateSnapIndicator(QPointF(), false);
                event->accept();
                return;
            }
        }

        bool snapped = false;
        const QPointF constrained =
            applyConstraints(event->scenePos(), m_activeWall->startPos(), useOrtho, &snapped);
        m_activeWall->setEndPos(constrained);
        m_activeWall->updateGeometry();
        const QPointF delta = constrained - m_activeWall->startPos();
        if (qAbs(delta.x()) > 0.001 || qAbs(delta.y()) > 0.001) {
            QVector2D dir(delta);
            dir.normalize();
            m_lastDirection = dir;
        }
        event->accept();
        return;
    }

    if (m_mode == Mode_Calibrate && m_calibrationLine) {
        m_calibrationLine->setLine(QLineF(m_calibrationStart, event->scenePos()));
        event->accept();
        return;
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void DesignScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_editHandle != Handle_None && event->button() == Qt::LeftButton) {
        m_editHandle = Handle_None;
        m_editWall = nullptr;
        updateSnapIndicator(QPointF(), false);
        event->accept();
        return;
    }

    if (m_dragWall && event->button() == Qt::LeftButton) {
        m_dragWall = nullptr;
        event->accept();
        return;
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

void DesignScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData() &&
        (event->mimeData()->hasFormat(ComponentListWidget::openingMimeType()) ||
         event->mimeData()->hasFormat(ComponentListWidget::furnitureMimeType()))) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void DesignScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (!event->mimeData()) {
        event->ignore();
        return;
    }

    if (event->mimeData()->hasFormat(ComponentListWidget::openingMimeType())) {
        const QByteArray raw =
            event->mimeData()->data(ComponentListWidget::openingMimeType());
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &error);
    if (doc.isNull() || !doc.isObject()) {
        event->ignore();
        return;
    }

        const QJsonObject obj = doc.object();
    const QString kind = obj.value("kind").toString();
    const QString style = obj.value("style").toString();
    const qreal width = obj.value("width").toDouble(900.0);
    const qreal height = obj.value("height").toDouble(2100.0);
    const qreal sill = obj.value("sill").toDouble(0.0);

        const bool needNewPreview =
            !m_previewOpening ||
            (kind == "door" && m_previewOpening->kind() != OpeningItem::Kind::Door) ||
            (kind != "door" && m_previewOpening->kind() != OpeningItem::Kind::Window);

        if (needNewPreview) {
            clearOpeningPreview();
            if (kind == "door") {
                DoorItem::DoorStyle doorStyle = DoorItem::DoorStyle::Single;
                if (style == "double") {
                    doorStyle = DoorItem::DoorStyle::Double;
                } else if (style == "sliding") {
                    doorStyle = DoorItem::DoorStyle::Sliding;
                }
                m_previewOpening = new DoorItem(doorStyle, width, height);
            } else {
                WindowItem::WindowStyle windowStyle = WindowItem::WindowStyle::Casement;
                if (style == "sliding") {
                    windowStyle = WindowItem::WindowStyle::Sliding;
                } else if (style == "bay") {
                    windowStyle = WindowItem::WindowStyle::Bay;
                }
                m_previewOpening = new WindowItem(windowStyle, width, height, sill);
            }
            m_previewOpening->setPreview(true);
            addItem(m_previewOpening);
        }

        if (m_previewOpening) {
            m_previewOpening->setWidth(width);
            m_previewOpening->setHeight(height);
            m_previewOpening->setSillHeight(sill);
        }

        const QPointF scenePos = event->scenePos();
        qreal distanceAlong = 0.0;
        WallItem *wall = findWallNear(scenePos, 50.0, &distanceAlong);
        updateHoverWall(wall);

        if (wall) {
            m_previewOpening->setVisible(true);
            m_previewOpening->setWall(wall);

            const qreal length = QLineF(wall->startPos(), wall->endPos()).length();
            qreal centerDistance = distanceAlong;
            const qreal snapDistance = 80.0;
            if (qAbs(centerDistance) <= snapDistance) {
                centerDistance = 0.0;
            } else if (qAbs(centerDistance - length / 2.0) <= snapDistance) {
                centerDistance = length / 2.0;
            } else if (qAbs(centerDistance - length) <= snapDistance) {
                centerDistance = length;
            }
            const qreal startDistance = centerDistance - width / 2.0;
            m_previewOpening->setDistanceFromStart(startDistance);
            m_previewOpening->syncWithWall();
        } else if (m_previewOpening) {
            m_previewOpening->setVisible(false);
        }

        event->acceptProposedAction();
        return;
    }

    if (event->mimeData()->hasFormat(ComponentListWidget::furnitureMimeType())) {
        const QByteArray raw =
            event->mimeData()->data(ComponentListWidget::furnitureMimeType());
        QJsonParseError error{};
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &error);
        if (doc.isNull() || !doc.isObject()) {
            event->ignore();
            return;
        }

        const QJsonObject obj = doc.object();
        const QString assetId = obj.value("assetId").toString();
        if (assetId.isEmpty()) {
            event->ignore();
            return;
        }

        if (!m_previewFurniture || m_previewFurniture->assetId() != assetId) {
            clearFurniturePreview();
            m_previewFurniture = new FurnitureItem(assetId);
            m_previewFurniture->setPreview(true);
            addItem(m_previewFurniture);
        }

        if (m_previewFurniture) {
            m_previewFurniture->setPos(event->scenePos());
        }

        event->acceptProposedAction();
        return;
    }

    event->ignore();
}

void DesignScene::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_UNUSED(event);
    clearOpeningPreview();
    clearFurniturePreview();
}

void DesignScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    if (!event->mimeData()) {
        event->ignore();
        return;
    }

    if (event->mimeData()->hasFormat(ComponentListWidget::openingMimeType())) {
        if (m_previewOpening && m_hoverWall) {
            m_previewOpening->setPreview(false);
            m_hoverWall->addOpening(m_previewOpening);
            clearSelection();  // 清除之前的选择
            m_previewOpening->setSelected(true);
            notifySceneChanged();
            m_previewOpening = nullptr;
            updateHoverWall(nullptr);
        } else {
            clearOpeningPreview();
        }

        event->acceptProposedAction();
        return;
    }

    if (event->mimeData()->hasFormat(ComponentListWidget::furnitureMimeType())) {
        if (m_previewFurniture) {
            m_previewFurniture->setPreview(false);
            clearSelection();  // 清除之前的选择
            m_previewFurniture->setSelected(true);
            notifySceneChanged();
            m_previewFurniture = nullptr;
        }
        event->acceptProposedAction();
        return;
    }

    event->ignore();
}

void DesignScene::keyPressEvent(QKeyEvent *event)
{
    if (m_mode == Mode_DrawWall && m_activeWall) {
        const int key = event->key();
        if ((key >= Qt::Key_0 && key <= Qt::Key_9) || key == Qt::Key_Period) {
            const QChar ch = event->text().isEmpty() ? QChar() : event->text().at(0);
            if (!ch.isNull() && (ch.isDigit() || ch == '.')) {
                m_lengthInput.append(ch);
                updateLengthIndicator();
                event->accept();
                return;
            }
        }

        if (key == Qt::Key_Backspace) {
            if (!m_lengthInput.isEmpty()) {
                m_lengthInput.chop(1);
                updateLengthIndicator();
            }
            event->accept();
            return;
        }

        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            bool ok = false;
            const double length = m_lengthInput.toDouble(&ok);
            if (ok && length > 0.0) {
                const QPointF anchor = m_activeWall->startPos();
                QPointF delta = m_lastMousePos - anchor;
                if (qAbs(delta.x()) < 0.001 && qAbs(delta.y()) < 0.001) {
                    delta = QPointF(m_lastDirection.x(), m_lastDirection.y());
                }
                QVector2D dir(delta);
                if (dir.lengthSquared() < 0.0001f) {
                    dir = m_lastDirection;
                } else {
                    dir.normalize();
                    m_lastDirection = dir;
                }

                const QPointF end =
                    anchor + QPointF(dir.x(), dir.y()) * static_cast<qreal>(length);
                m_activeWall->setEndPos(end);
                m_activeWall->updateGeometry();

                const QPointF nextStart = end;
                clearSelection();
                m_activeWall = new WallItem(nextStart,
                                            nextStart,
                                            m_wallThickness,
                                            m_wallHeight);
                addItem(m_activeWall);
                m_activeWall->setSelected(true);
            }

            m_lengthInput.clear();
            updateLengthIndicator();
            updateSnapIndicator(QPointF(), false);
            event->accept();
            return;
        }

        if (key == Qt::Key_Escape) {
            m_lengthInput.clear();
            updateLengthIndicator();
            event->accept();
            return;
        }
    }

    QGraphicsScene::keyPressEvent(event);
}

QPointF DesignScene::applyConstraints(const QPointF &rawPos,
                                      const QPointF &anchor,
                                      bool orthogonal,
                                      bool *snapped)
{
    QPointF adjusted = rawPos;
    if (orthogonal) {
        const QPointF delta = rawPos - anchor;
        if (qAbs(delta.x()) >= qAbs(delta.y())) {
            adjusted.setY(anchor.y());
        } else {
            adjusted.setX(anchor.x());
        }
    }

    return snapPosition(adjusted, snapped);
}

QPointF DesignScene::snapPosition(const QPointF &pos, bool *snapped)
{
    if (snapped) {
        *snapped = false;
    }

    if (!m_snapEnabled) {
        updateSnapIndicator(QPointF(), false);
        return pos;
    }

    QPointF best = pos;
    qreal bestDist = m_snapTolerance + 1.0;

    const QList<WallItem *> walls = wallItems();
    for (WallItem *wall : walls) {
        const QPointF start = wall->startPos();
        const QPointF end = wall->endPos();
        const QPointF mid = (start + end) / 2.0;

        const qreal distStart = QLineF(pos, start).length();
        if (distStart < bestDist) {
            bestDist = distStart;
            best = start;
        }

        const qreal distEnd = QLineF(pos, end).length();
        if (distEnd < bestDist) {
            bestDist = distEnd;
            best = end;
        }

        const qreal distMid = QLineF(pos, mid).length();
        if (distMid < bestDist) {
            bestDist = distMid;
            best = mid;
        }

        const QPointF v = end - start;
        const qreal denom = v.x() * v.x() + v.y() * v.y();
        if (denom > 0.0001) {
            const QPointF w = pos - start;
            const qreal t = (w.x() * v.x() + w.y() * v.y()) / denom;
            if (t >= 0.0 && t <= 1.0) {
                const QPointF proj = start + v * t;
                const qreal distProj = QLineF(pos, proj).length();
                if (distProj < bestDist) {
                    bestDist = distProj;
                    best = proj;
                }
            }
        }
    }

    if (m_snapToGridEnabled) {
        const QPointF gridPoint(qRound(pos.x() / m_gridSize) * m_gridSize,
                                qRound(pos.y() / m_gridSize) * m_gridSize);
        const qreal distGrid = QLineF(pos, gridPoint).length();
        if (distGrid < bestDist) {
            bestDist = distGrid;
            best = gridPoint;
        }
    }

    const bool isSnapped = bestDist <= m_snapTolerance;
    updateSnapIndicator(best, isSnapped);

    if (snapped) {
        *snapped = isSnapped;
    }

    return isSnapped ? best : pos;
}

QList<WallItem *> DesignScene::wallItems() const
{
    QList<WallItem *> result;
    const QList<QGraphicsItem *> sceneItems = items();
    for (QGraphicsItem *item : sceneItems) {
        auto *wall = qgraphicsitem_cast<WallItem *>(item);
        if (!wall) {
            continue;
        }
        if (wall == m_activeWall || wall == m_editWall) {
            continue;
        }
        result.append(wall);
    }

    return result;
}

void DesignScene::updateSnapIndicator(const QPointF &pos, bool visible)
{
    if (!m_snapIndicator) {
        return;
    }

    if (!visible) {
        m_snapIndicator->setVisible(false);
        return;
    }

    const QRectF rect(pos.x() - 4.0, pos.y() - 4.0, 8.0, 8.0);
    m_snapIndicator->setRect(rect);
    m_snapIndicator->setVisible(true);
}

void DesignScene::updateLengthIndicator()
{
    if (!m_lengthIndicator) {
        return;
    }

    if (m_lengthInput.isEmpty()) {
        m_lengthIndicator->setVisible(false);
        return;
    }

    m_lengthIndicator->setText(m_lengthInput + " mm");
    m_lengthIndicator->setPos(m_lastMousePos + QPointF(12.0, -18.0));
    m_lengthIndicator->setVisible(true);
}

bool DesignScene::tryBeginEdit(const QPointF &scenePos)
{
    const qreal handleRadius = m_snapTolerance + 2.0;
    WallItem *closestWall = nullptr;
    EditHandle closestHandle = Handle_None;
    qreal bestDist = handleRadius;

    const QList<QGraphicsItem *> sceneItems = items();
    for (QGraphicsItem *item : sceneItems) {
        auto *wall = qgraphicsitem_cast<WallItem *>(item);
        if (!wall) {
            continue;
        }

        const qreal distStart = QLineF(scenePos, wall->startPos()).length();
        if (distStart < bestDist) {
            bestDist = distStart;
            closestWall = wall;
            closestHandle = Handle_Start;
        }

        const qreal distEnd = QLineF(scenePos, wall->endPos()).length();
        if (distEnd < bestDist) {
            bestDist = distEnd;
            closestWall = wall;
            closestHandle = Handle_End;
        }
    }

    if (!closestWall || closestHandle == Handle_None) {
        return false;
    }

    clearSelection();
    closestWall->setSelected(true);
    m_editWall = closestWall;
    m_editHandle = closestHandle;
    return true;
}

bool DesignScene::tryBeginDrag(const QPointF &scenePos)
{
    QGraphicsItem *item = itemAt(scenePos, QTransform());
    auto *wall = qgraphicsitem_cast<WallItem *>(item);
    if (!wall) {
        return false;
    }

    clearSelection();
    wall->setSelected(true);
    m_dragWall = wall;
    m_dragLastPos = scenePos;
    return true;
}

void DesignScene::finalizeWall(const QPointF &endPos, bool applyEndPos)
{
    if (!m_activeWall) {
        return;
    }

    if (applyEndPos) {
        m_activeWall->setEndPos(endPos);
        m_activeWall->updateGeometry();
    }

    if (m_activeWall->length() < 0.1) {
        removeItem(m_activeWall);
        delete m_activeWall;
    }

    m_activeWall = nullptr;
    m_lengthInput.clear();
    updateLengthIndicator();
    updateSnapIndicator(QPointF(), false);
}

void DesignScene::resetCalibration()
{
    if (m_calibrationLine) {
        removeItem(m_calibrationLine);
        delete m_calibrationLine;
        m_calibrationLine = nullptr;
    }

    m_lastCalibrationLength = 0.0;
    updateSnapIndicator(QPointF(), false);
}

WallItem *DesignScene::findWallNear(const QPointF &pos,
                                    qreal maxDistance,
                                    qreal *distanceAlong) const
{
    WallItem *closest = nullptr;
    qreal bestDistance = maxDistance;
    qreal bestAlong = 0.0;

    const QList<WallItem *> walls = wallItems();
    for (WallItem *wall : walls) {
        if (!wall) {
            continue;
        }
        const QPointF start = wall->startPos();
        const QPointF end = wall->endPos();
        const QPointF v = end - start;
        const qreal denom = v.x() * v.x() + v.y() * v.y();
        if (denom < 0.0001) {
            continue;
        }

        const QPointF w = pos - start;
        qreal t = (w.x() * v.x() + w.y() * v.y()) / denom;
        t = qBound(0.0, t, 1.0);
        const QPointF proj = start + v * t;
        const qreal distance = QLineF(pos, proj).length();
        if (distance < bestDistance) {
            bestDistance = distance;
            closest = wall;
            bestAlong = t * std::sqrt(denom);
        }
    }

    if (distanceAlong) {
        *distanceAlong = bestAlong;
    }
    return closest;
}

void DesignScene::updateHoverWall(WallItem *wall)
{
    if (m_hoverWall == wall) {
        return;
    }

    if (m_hoverWall) {
        m_hoverWall->setHighlighted(false);
    }
    m_hoverWall = wall;
    if (m_hoverWall) {
        m_hoverWall->setHighlighted(true);
    }
}

void DesignScene::clearOpeningPreview()
{
    updateHoverWall(nullptr);
    if (m_previewOpening) {
        removeItem(m_previewOpening);
        delete m_previewOpening;
        m_previewOpening = nullptr;
    }
}

void DesignScene::clearFurniturePreview()
{
    if (m_previewFurniture) {
        removeItem(m_previewFurniture);
        delete m_previewFurniture;
        m_previewFurniture = nullptr;
    }
}
