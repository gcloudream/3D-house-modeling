#include "furnitureitem.h"
#include "designscene.h"

#include <QGraphicsSceneMouseEvent>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QSvgRenderer>
#include <QtMath>
#include <cmath>

namespace {
constexpr qreal kMinFurnitureSize = 80.0;
constexpr qreal kHandleRadius = 4.5;
constexpr qreal kRotateHandleOffset = 16.0;
constexpr qreal kRotateSnapStep = 45.0;
constexpr qreal kRotateSnapThreshold = 6.0;
} // namespace

FurnitureItem::FurnitureItem(const QString &assetId, QGraphicsItem *parent)
    : QGraphicsSvgItem(parent)
    , m_assetId(assetId)
    , m_asset()
    , m_size2D(300.0, 300.0)
    , m_height3D(300.0)
    , m_elevation(0.0)
    , m_scale3D(1.0f, 1.0f, 1.0f)
    , m_rotation(0.0)
    , m_preview(false)
    , m_dragMode(DragMode::None)
    , m_dragStartSize(300.0, 300.0)
{
    setFlags(QGraphicsItem::ItemIsSelectable
             | QGraphicsItem::ItemIsMovable
             | QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setZValue(5.0);

    const AssetManager::Asset asset = AssetManager::instance()->getAsset(assetId);
    if (!asset.id.isEmpty()) {
        loadAsset(asset);
    } else {
        updateGeometry();
    }
}

QString FurnitureItem::assetId() const
{
    return m_assetId;
}

const AssetManager::Asset &FurnitureItem::asset() const
{
    return m_asset;
}

bool FurnitureItem::hasValidAsset() const
{
    return !m_asset.id.isEmpty();
}

void FurnitureItem::setPreview(bool preview)
{
    m_preview = preview;
    setOpacity(m_preview ? 0.5 : 1.0);
    update();
}

bool FurnitureItem::isPreview() const
{
    return m_preview;
}

void FurnitureItem::setElevation(qreal elevation)
{
    m_elevation = elevation;
}

qreal FurnitureItem::elevation() const
{
    return m_elevation;
}

void FurnitureItem::setRotationDegrees(qreal angle)
{
    m_rotation = std::fmod(angle + 360.0, 360.0);
    setRotation(m_rotation);
    update();
}

qreal FurnitureItem::rotationDegrees() const
{
    return m_rotation;
}

void FurnitureItem::setSize2D(const QSizeF &size)
{
    QSizeF clamped = size;
    clamped.setWidth(qMax(kMinFurnitureSize, clamped.width()));
    clamped.setHeight(qMax(kMinFurnitureSize, clamped.height()));
    if (qFuzzyCompare(clamped.width(), m_size2D.width())
        && qFuzzyCompare(clamped.height(), m_size2D.height())) {
        return;
    }
    prepareGeometryChange();
    m_size2D = clamped;
    updateGeometry();
}

QSizeF FurnitureItem::size2D() const
{
    return m_size2D;
}

void FurnitureItem::setHeight3D(qreal height)
{
    m_height3D = qMax(10.0, height);
}

qreal FurnitureItem::height3D() const
{
    return m_height3D;
}

void FurnitureItem::setScale3D(const QVector3D &scale)
{
    m_scale3D = scale;
}

QVector3D FurnitureItem::scale3D() const
{
    return m_scale3D;
}

QVector3D FurnitureItem::defaultSize3D() const
{
    return m_asset.defaultSize;
}

QVector3D FurnitureItem::modelScale(const QVector3D &modelSize) const
{
    QVector3D baseSize = m_asset.defaultSize;
    if (baseSize.lengthSquared() < 0.001f) {
        baseSize = QVector3D(1000.0f, 1000.0f, 1000.0f);
    }

    const float fitX = (baseSize.x() > 0.0f && modelSize.x() > 0.0f)
        ? baseSize.x() / modelSize.x()
        : 1.0f;
    const float fitZ = (baseSize.y() > 0.0f && modelSize.z() > 0.0f)
        ? baseSize.y() / modelSize.z()
        : 1.0f;
    const float fitY = (baseSize.z() > 0.0f && modelSize.y() > 0.0f)
        ? baseSize.z() / modelSize.y()
        : 1.0f;

    const float userX = baseSize.x() > 0.0f
        ? static_cast<float>(m_size2D.width() / baseSize.x())
        : 1.0f;
    const float userZ = baseSize.y() > 0.0f
        ? static_cast<float>(m_size2D.height() / baseSize.y())
        : 1.0f;
    const float userY = baseSize.z() > 0.0f
        ? static_cast<float>(m_height3D / baseSize.z())
        : 1.0f;

    return QVector3D(fitX * userX * m_asset.defaultScale.x() * m_scale3D.x(),
                     fitY * userY * m_asset.defaultScale.y() * m_scale3D.y(),
                     fitZ * userZ * m_asset.defaultScale.z() * m_scale3D.z());
}

QString FurnitureItem::material() const
{
    return m_asset.material;
}

QJsonObject FurnitureItem::toJson() const
{
    QJsonObject obj;
    obj["model_id"] = m_assetId;
    obj["pos"] = QJsonArray{pos().x(), pos().y()};
    obj["rotate"] = m_rotation;
    obj["scale"] = QJsonArray{m_scale3D.x(), m_scale3D.y(), m_scale3D.z()};
    obj["elevation"] = m_elevation;
    obj["size"] = QJsonArray{m_size2D.width(), m_size2D.height()};
    obj["height"] = m_height3D;
    return obj;
}

FurnitureItem *FurnitureItem::fromJson(const QJsonObject &json)
{
    const QString assetId = json.value("model_id").toString();
    FurnitureItem *item = new FurnitureItem(assetId);

    const QJsonArray posArray = json.value("pos").toArray();
    item->setPos(QPointF(posArray.size() > 0 ? posArray.at(0).toDouble() : 0.0,
                         posArray.size() > 1 ? posArray.at(1).toDouble() : 0.0));

    const qreal rotation = json.value("rotate").toDouble(0.0);
    item->setRotationDegrees(rotation);

    const QJsonArray scaleArray = json.value("scale").toArray();
    if (scaleArray.size() >= 3) {
        item->setScale3D(QVector3D(static_cast<float>(scaleArray.at(0).toDouble(1.0)),
                                   static_cast<float>(scaleArray.at(1).toDouble(1.0)),
                                   static_cast<float>(scaleArray.at(2).toDouble(1.0))));
    }

    item->setElevation(json.value("elevation").toDouble(0.0));

    const QJsonArray sizeArray = json.value("size").toArray();
    if (sizeArray.size() >= 2) {
        item->setSize2D(QSizeF(sizeArray.at(0).toDouble(),
                               sizeArray.at(1).toDouble()));
    }

    if (json.contains("height")) {
        item->setHeight3D(json.value("height").toDouble());
    }

    return item;
}

QMatrix4x4 FurnitureItem::transformMatrix() const
{
    QMatrix4x4 matrix;
    matrix.translate(pos().x(), static_cast<float>(m_elevation), -pos().y());
    matrix.rotate(static_cast<float>(m_rotation), 0.0f, 1.0f, 0.0f);
    return matrix;
}

QRectF FurnitureItem::boundingRect() const
{
    return QRectF(-m_size2D.width() / 2.0,
                  -m_size2D.height() / 2.0,
                  m_size2D.width(),
                  m_size2D.height());
}

void FurnitureItem::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing, true);

    if (renderer() && renderer()->isValid()) {
        renderer()->render(painter, boundingRect());
    } else {
        painter->setPen(QPen(QColor(47, 110, 143), 1.5));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }

    if (isSelected()) {
        const QColor accent(24, 144, 255);
        painter->setPen(QPen(accent, 1.5));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());

        painter->setBrush(accent);
        painter->setPen(Qt::NoPen);
        for (const QRectF &rect : scaleHandleRects()) {
            painter->drawEllipse(rect);
        }
        const QRectF rotateRect = rotationHandleRect();
        painter->drawEllipse(rotateRect);

        painter->setPen(QPen(accent, 1.0));
        painter->drawLine(QPointF(0.0, boundingRect().top()), rotateRect.center());
    }
}

QVariant FurnitureItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemSelectedHasChanged) {
        update();
    }
    return QGraphicsSvgItem::itemChange(change, value);
}

void FurnitureItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isSelected()) {
        const QPointF localPos = event->pos();
        if (rotationHandleRect().contains(localPos)) {
            m_dragMode = DragMode::Rotate;
            event->accept();
            return;
        }
        for (const QRectF &rect : scaleHandleRects()) {
            if (rect.contains(localPos)) {
                m_dragMode = DragMode::Scale;
                m_dragStartSize = m_size2D;
                event->accept();
                return;
            }
        }
    }

    m_dragMode = DragMode::Move;
    QGraphicsSvgItem::mousePressEvent(event);
}

void FurnitureItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_dragMode == DragMode::Rotate) {
        const QPointF center = itemCenter();
        const qreal rawAngle = -QLineF(center, event->pos()).angle();
        const qreal snapped = snappedAngle(rawAngle);
        setRotationDegrees(snapped);
        event->accept();
        return;
    }

    if (m_dragMode == DragMode::Scale) {
        const QPointF center = itemCenter();
        const QPointF delta = event->pos() - center;
        qreal newWidth = qMax(kMinFurnitureSize, qAbs(delta.x()) * 2.0);
        qreal newHeight = qMax(kMinFurnitureSize, qAbs(delta.y()) * 2.0);
        if (event->modifiers().testFlag(Qt::ShiftModifier)) {
            const qreal ratio = m_dragStartSize.width() / m_dragStartSize.height();
            if (ratio > 0.0) {
                if (newWidth / newHeight > ratio) {
                    newWidth = newHeight * ratio;
                } else {
                    newHeight = newWidth / ratio;
                }
            }
        }
        setSize2D(QSizeF(newWidth, newHeight));
        event->accept();
        return;
    }

    QGraphicsSvgItem::mouseMoveEvent(event);
}

void FurnitureItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_dragMode != DragMode::Move) {
        m_dragMode = DragMode::None;
        notifySceneChanged();
        event->accept();
        return;
    }

    QGraphicsSvgItem::mouseReleaseEvent(event);
    notifySceneChanged();
}

void FurnitureItem::loadAsset(const AssetManager::Asset &asset)
{
    m_asset = asset;
    m_assetId = asset.id;

    if (!asset.svgPath.isEmpty()) {
        auto *svg = new QSvgRenderer(asset.svgPath, this);
        setSharedRenderer(svg);
    }

    m_size2D = QSizeF(asset.defaultSize.x(), asset.defaultSize.y());
    m_height3D = asset.defaultSize.z();
    updateGeometry();
}

QRectF FurnitureItem::handleRect(const QPointF &center) const
{
    return QRectF(center.x() - kHandleRadius,
                  center.y() - kHandleRadius,
                  kHandleRadius * 2.0,
                  kHandleRadius * 2.0);
}

QRectF FurnitureItem::rotationHandleRect() const
{
    const QRectF rect = boundingRect();
    const QPointF center(0.0, rect.top() - kRotateHandleOffset);
    return handleRect(center);
}

QList<QRectF> FurnitureItem::scaleHandleRects() const
{
    const QRectF rect = boundingRect();
    QList<QRectF> rects;
    rects << handleRect(QPointF(rect.left(), rect.top()))
          << handleRect(QPointF(rect.right(), rect.top()))
          << handleRect(QPointF(rect.right(), rect.bottom()))
          << handleRect(QPointF(rect.left(), rect.bottom()));
    return rects;
}

QPointF FurnitureItem::itemCenter() const
{
    return QPointF(0.0, 0.0);
}

qreal FurnitureItem::snappedAngle(qreal angle) const
{
    const qreal normalized = std::fmod(angle + 360.0, 360.0);
    const qreal snapped = qRound(normalized / kRotateSnapStep) * kRotateSnapStep;
    if (qAbs(snapped - normalized) <= kRotateSnapThreshold) {
        return snapped;
    }
    return normalized;
}

void FurnitureItem::updateGeometry()
{
    setTransformOriginPoint(0.0, 0.0);
    setRotation(m_rotation);
    update();
}

void FurnitureItem::notifySceneChanged()
{
    if (m_preview) {
        return;
    }
    auto *designScene = qobject_cast<DesignScene *>(scene());
    if (designScene) {
        designScene->notifySceneChanged();
    }
}
