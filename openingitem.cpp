#include "openingitem.h"

#include "wallitem.h"

#include <QPainter>
#include <QPainterPath>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QLineF>
#include <QtMath>
#include <QVector2D>
#include <cmath>

namespace {
constexpr qreal kHandleRadius = 5.0;
constexpr qreal kHandleOffset = 12.0;
constexpr qreal kDefaultThickness = 30.0;
}

OpeningItem::OpeningItem(Kind kind,
                         Style style,
                         qreal width,
                         qreal height,
                         qreal sillHeight,
                         QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_kind(kind)
    , m_style(style)
    , m_width(width)
    , m_height(height)
    , m_sillHeight(sillHeight)
    , m_distance(0.0)
    , m_preview(false)
    , m_flipped(false)
    , m_ignorePositionChange(false)
    , m_wall(nullptr)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setZValue(1.0);  // 略高于墙体(0.0)，但低于家具(5.0)
}

OpeningItem::Kind OpeningItem::kind() const
{
    return m_kind;
}

OpeningItem::Style OpeningItem::style() const
{
    return m_style;
}

QString OpeningItem::kindLabel() const
{
    return m_kind == Kind::Door ? QStringLiteral("门") : QStringLiteral("窗");
}

QString OpeningItem::styleLabel() const
{
    switch (m_style) {
    case Style::SingleDoor:
        return QStringLiteral("单开");
    case Style::DoubleDoor:
        return QStringLiteral("双开");
    case Style::SlidingDoor:
        return QStringLiteral("推拉");
    case Style::CasementWindow:
        return QStringLiteral("平开");
    case Style::SlidingWindow:
        return QStringLiteral("推拉");
    case Style::BayWindow:
        return QStringLiteral("飘窗");
    default:
        return QString();
    }
}

void OpeningItem::setWall(WallItem *wall)
{
    if (m_wall == wall) {
        return;
    }
    m_wall = wall;
    syncWithWall();
}

WallItem *OpeningItem::wall() const
{
    return m_wall;
}

void OpeningItem::setDistanceFromStart(qreal distance)
{
    const qreal clamped = clampDistance(distance);
    if (qFuzzyCompare(clamped, m_distance)) {
        return;
    }
    m_distance = clamped;
    syncWithWall();
}

qreal OpeningItem::distanceFromStart() const
{
    return m_distance;
}

void OpeningItem::setWidth(qreal width)
{
    if (width <= 0.0) {
        return;
    }
    prepareGeometryChange();
    m_width = width;
    m_distance = clampDistance(m_distance);
    syncWithWall();
}

qreal OpeningItem::width() const
{
    return m_width;
}

void OpeningItem::setHeight(qreal height)
{
    if (height <= 0.0) {
        return;
    }
    m_height = height;
    update();
}

qreal OpeningItem::height() const
{
    return m_height;
}

void OpeningItem::setSillHeight(qreal height)
{
    m_sillHeight = qMax(0.0, height);
    update();
}

qreal OpeningItem::sillHeight() const
{
    return m_sillHeight;
}

void OpeningItem::setPreview(bool preview)
{
    if (m_preview == preview) {
        return;
    }
    m_preview = preview;
    updateFlags();
}

bool OpeningItem::isPreview() const
{
    return m_preview;
}

void OpeningItem::setFlipped(bool flipped)
{
    if (m_flipped == flipped) {
        return;
    }
    m_flipped = flipped;
    update();
}

bool OpeningItem::isFlipped() const
{
    return m_flipped;
}

void OpeningItem::toggleFlip()
{
    setFlipped(!m_flipped);
}

void OpeningItem::syncWithWall()
{
    if (!m_wall) {
        return;
    }

    const qreal length = wallLength();
    if (length < 0.1) {
        return;
    }

    m_distance = clampDistance(m_distance);
    m_ignorePositionChange = true;
    prepareGeometryChange();
    setRotation(-wallAngle());
    setPos(startPointForDistance(m_distance));
    m_ignorePositionChange = false;
    update();
}

QRectF OpeningItem::boundingRect() const
{
    QRectF rect = openingRect();
    if (m_kind == Kind::Door) {
        rect = rect.united(doorSwingRect());
        rect = rect.united(flipHandleRect());
    } else if (m_kind == Kind::Window && m_style == Style::BayWindow) {
        rect = rect.united(bayProtrusionRect());
    }
    rect = rect.adjusted(-2.0, -2.0, 2.0, 2.0);
    return rect;
}

QPainterPath OpeningItem::shape() const
{
    QPainterPath path;
    path.addRect(openingRect());
    if (m_kind == Kind::Door) {
        const QRectF handle = flipHandleRect();
        if (!handle.isNull()) {
            path.addEllipse(handle);
        }
    } else if (m_kind == Kind::Window && m_style == Style::BayWindow) {
        path.addPolygon(bayOutline());
    }
    return path;
}

void OpeningItem::paint(QPainter *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF maskRect = openingRect();
    painter->setBrush(Qt::white);
    painter->setPen(Qt::NoPen);
    painter->drawRect(maskRect);

    QPen symbolPen(QColor(45, 53, 63), 1.6);
    painter->setPen(symbolPen);
    painter->setBrush(Qt::NoBrush);

    if (m_kind == Kind::Door) {
        painter->drawRect(maskRect);

        const qreal swingDir = m_flipped ? -1.0 : 1.0;
        const int span = m_flipped ? 90 : -90;

        if (m_style == Style::SingleDoor) {
            QPainterPath swingPath;
            swingPath.moveTo(QPointF(0.0, 0.0));
            const QRectF arcRect = doorSwingRect();
            swingPath.arcTo(arcRect, 0.0, span);
            painter->drawPath(swingPath);

            const QPointF leafEnd(0.0, swingDir * m_width);
            painter->drawLine(QPointF(0.0, 0.0), leafEnd);

            const QVector2D dirVec(leafEnd);
            if (dirVec.lengthSquared() > 0.001f) {
                const QVector2D unit = dirVec.normalized();
                const QPointF tip = leafEnd;
                const QPointF back = tip - QPointF(unit.x(), unit.y()) * 8.0;
                const QPointF left = back + QPointF(-unit.y(), unit.x()) * 4.0;
                const QPointF right = back + QPointF(unit.y(), -unit.x()) * 4.0;
                painter->drawLine(tip, left);
                painter->drawLine(tip, right);
            }
        } else if (m_style == Style::DoubleDoor) {
            const qreal half = m_width * 0.5;
            const QPointF hinge(half, 0.0);

            painter->drawLine(QPointF(half, maskRect.top()),
                              QPointF(half, maskRect.bottom()));

            const QRectF arcRect(hinge.x() - half, -half, half * 2.0, half * 2.0);
            QPainterPath leftArc;
            leftArc.moveTo(hinge);
            leftArc.arcTo(arcRect, 180.0, span);
            painter->drawPath(leftArc);

            QPainterPath rightArc;
            rightArc.moveTo(hinge);
            rightArc.arcTo(arcRect, 0.0, span);
            painter->drawPath(rightArc);
        } else if (m_style == Style::SlidingDoor) {
            const qreal inset = qMax(2.0, maskRect.height() * 0.15);
            const qreal panelHeight = maskRect.height() - inset * 2.0;
            const qreal overlap = maskRect.width() * 0.2;
            const QRectF panelLeft(maskRect.left() + inset,
                                   maskRect.top() + inset,
                                   maskRect.width() * 0.55,
                                   panelHeight);
            const QRectF panelRight(maskRect.left() + maskRect.width() * 0.45 - overlap,
                                    maskRect.top() + inset,
                                    maskRect.width() * 0.55,
                                    panelHeight);
            painter->drawRect(panelLeft);
            painter->drawRect(panelRight);

            painter->drawLine(QPointF(maskRect.left() + inset, maskRect.top() + inset),
                              QPointF(maskRect.right() - inset, maskRect.top() + inset));
            painter->drawLine(QPointF(maskRect.left() + inset, maskRect.bottom() - inset),
                              QPointF(maskRect.right() - inset, maskRect.bottom() - inset));

            const QPointF mid(maskRect.center().x(), maskRect.center().y());
            const qreal arrowLen = maskRect.width() * 0.25;
            painter->drawLine(QPointF(mid.x() - arrowLen, mid.y()),
                              QPointF(mid.x() + arrowLen, mid.y()));

            const QPointF leftArrow(mid.x() - arrowLen, mid.y());
            const QPointF rightArrow(mid.x() + arrowLen, mid.y());
            painter->drawLine(leftArrow,
                              QPointF(leftArrow.x() + 3.0, leftArrow.y() - 2.0));
            painter->drawLine(leftArrow,
                              QPointF(leftArrow.x() + 3.0, leftArrow.y() + 2.0));
            painter->drawLine(rightArrow,
                              QPointF(rightArrow.x() - 3.0, rightArrow.y() - 2.0));
            painter->drawLine(rightArrow,
                              QPointF(rightArrow.x() - 3.0, rightArrow.y() + 2.0));
        }

        if (isSelected()) {
            painter->setBrush(QColor(47, 110, 143));
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(flipHandleRect());
        }
    } else {
        painter->drawRect(maskRect);
        const qreal inset = qMax(2.0, maskRect.height() * 0.2);

        switch (m_style) {
        case Style::CasementWindow: {
            const QRectF sash = maskRect.adjusted(2.0, inset, -2.0, -inset);
            painter->drawRect(sash);
            painter->drawLine(sash.topLeft(), sash.bottomRight());
            break;
        }
        case Style::SlidingWindow: {
            const QRectF leftPanel =
                maskRect.adjusted(2.0, inset, -maskRect.width() * 0.45, -inset);
            const QRectF rightPanel =
                maskRect.adjusted(maskRect.width() * 0.45, inset, -2.0, -inset);
            painter->drawRect(leftPanel);
            painter->drawRect(rightPanel);
            painter->drawLine(QPointF(maskRect.center().x(), maskRect.top()),
                              QPointF(maskRect.center().x(), maskRect.bottom()));
            break;
        }
        case Style::BayWindow: {
            const QPolygonF bay = bayOutline();
            painter->setBrush(Qt::white);
            painter->setPen(Qt::NoPen);
            painter->drawPolygon(bay);
            painter->setPen(symbolPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawPolygon(bay);
            const QPointF baseCenter(maskRect.center().x(), maskRect.bottom());
            painter->drawLine(baseCenter,
                              QPointF(baseCenter.x(),
                                      baseCenter.y() + bayProtrusionRect().height()));
            break;
        }
        default: {
            QRectF inner = maskRect.adjusted(0.0, inset, 0.0, -inset);
            painter->drawRect(inner);
            painter->drawLine(QPointF(maskRect.center().x(), maskRect.top()),
                              QPointF(maskRect.center().x(), maskRect.bottom()));
            break;
        }
        }
    }

    if (isSelected()) {
        QPen highlightPen(QColor(47, 110, 143), 1.3, Qt::DashLine);
        painter->setPen(highlightPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(maskRect.adjusted(-2.0, -2.0, 2.0, 2.0));
    }

    painter->restore();
}

QVariant OpeningItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && m_wall && !m_ignorePositionChange) {
        const QPointF proposed = value.toPointF();
        qreal distance = projectDistance(proposed);
        distance = clampDistance(distance);
        m_distance = distance;
        return startPointForDistance(distance);
    }
    if (change == ItemSelectedHasChanged || change == ItemPositionHasChanged) {
        update();
    }
    return QGraphicsItem::itemChange(change, value);
}

void OpeningItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_kind == Kind::Door && isSelected() &&
        flipHandleRect().contains(event->pos())) {
        toggleFlip();
        event->accept();
        return;
    }
    QGraphicsItem::mousePressEvent(event);
}

QRectF OpeningItem::openingRect() const
{
    const qreal thickness = wallThickness();
    return QRectF(0.0, -thickness / 2.0, m_width, thickness);
}

QRectF OpeningItem::doorSwingRect() const
{
    return QRectF(-m_width, -m_width, m_width * 2.0, m_width * 2.0);
}

QRectF OpeningItem::flipHandleRect() const
{
    if (m_kind != Kind::Door) {
        return QRectF();
    }
    const qreal thickness = wallThickness();
    const qreal direction = m_flipped ? -1.0 : 1.0;
    const QPointF center(m_width * 0.5, direction * (thickness * 0.6 + kHandleOffset));
    return QRectF(center.x() - kHandleRadius,
                  center.y() - kHandleRadius,
                  kHandleRadius * 2.0,
                  kHandleRadius * 2.0);
}

QRectF OpeningItem::bayProtrusionRect() const
{
    const QRectF rect = openingRect();
    const qreal depth = qMax(6.0, wallThickness() * 0.6);
    return QRectF(rect.left(), rect.bottom(), rect.width(), depth);
}

QPolygonF OpeningItem::bayOutline() const
{
    const QRectF rect = openingRect();
    const qreal depth = qMax(6.0, wallThickness() * 0.6);
    const qreal inset = rect.width() * 0.18;
    const QPointF a(rect.left() + inset, rect.bottom());
    const QPointF b(rect.right() - inset, rect.bottom());
    const QPointF c(rect.right() - inset * 1.4, rect.bottom() + depth);
    const QPointF d(rect.left() + inset * 1.4, rect.bottom() + depth);
    return QPolygonF() << a << b << c << d;
}

qreal OpeningItem::wallThickness() const
{
    return m_wall ? m_wall->thickness() : kDefaultThickness;
}

qreal OpeningItem::wallLength() const
{
    if (!m_wall) {
        return 0.0;
    }
    return QLineF(m_wall->startPos(), m_wall->endPos()).length();
}

qreal OpeningItem::wallAngle() const
{
    if (!m_wall) {
        return 0.0;
    }
    return QLineF(m_wall->startPos(), m_wall->endPos()).angle();
}

QPointF OpeningItem::startPointForDistance(qreal distance) const
{
    if (!m_wall) {
        return QPointF();
    }
    QLineF line(m_wall->startPos(), m_wall->endPos());
    if (line.length() < 0.1) {
        return m_wall->startPos();
    }
    line.setLength(distance);
    return line.p2();
}

qreal OpeningItem::projectDistance(const QPointF &scenePos) const
{
    if (!m_wall) {
        return 0.0;
    }
    const QPointF start = m_wall->startPos();
    const QPointF end = m_wall->endPos();
    const QPointF v = end - start;
    const qreal denom = v.x() * v.x() + v.y() * v.y();
    if (denom < 0.0001) {
        return 0.0;
    }
    const QPointF w = scenePos - start;
    const qreal t = (w.x() * v.x() + w.y() * v.y()) / denom;
    const qreal length = std::sqrt(denom);
    return qBound(0.0, t, 1.0) * length;
}

qreal OpeningItem::clampDistance(qreal distance) const
{
    const qreal length = wallLength();
    if (length < 0.1) {
        return 0.0;
    }
    const qreal maxStart = qMax(0.0, length - m_width);
    return qBound(0.0, distance, maxStart);
}

void OpeningItem::updateFlags()
{
    if (m_preview) {
        setOpacity(0.55);
        setAcceptedMouseButtons(Qt::NoButton);
        setFlag(QGraphicsItem::ItemIsMovable, false);
        setFlag(QGraphicsItem::ItemIsSelectable, false);
    } else {
        setOpacity(1.0);
        setAcceptedMouseButtons(Qt::LeftButton);
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
    }
}
