#include "wallitem.h"

#include <QBrush>
#include <QLineF>
#include <QPen>
#include <QPolygonF>

WallItem::WallItem(const QPointF &start,
                   const QPointF &end,
                   qreal thickness,
                   qreal height,
                   QGraphicsItem *parent)
    : QGraphicsPolygonItem(parent)
    , m_start(start)
    , m_end(end)
    , m_thickness(thickness)
    , m_height(height)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
    setPen(QPen(QColor(63, 73, 84), 1.2));
    setBrush(QBrush(QColor(186, 195, 205)));
    updateGeometry();
}

void WallItem::setStartPos(const QPointF &pos)
{
    m_start = pos;
}

void WallItem::setEndPos(const QPointF &pos)
{
    m_end = pos;
}

QPointF WallItem::startPos() const
{
    return m_start;
}

QPointF WallItem::endPos() const
{
    return m_end;
}

void WallItem::setThickness(qreal thickness)
{
    m_thickness = thickness;
    updateGeometry();
}

qreal WallItem::thickness() const
{
    return m_thickness;
}

void WallItem::setHeight(qreal height)
{
    m_height = height;
}

qreal WallItem::height() const
{
    return m_height;
}

qreal WallItem::length() const
{
    return QLineF(m_start, m_end).length();
}

qreal WallItem::angleDegrees() const
{
    return QLineF(m_start, m_end).angle();
}

void WallItem::setAngleDegrees(qreal angle)
{
    QLineF line(m_start, m_end);
    if (line.length() < 0.1) {
        return;
    }

    line.setAngle(angle);
    m_end = line.p2();
    updateGeometry();
}

void WallItem::updateGeometry()
{
    QLineF centerLine(m_start, m_end);
    if (centerLine.length() < 0.1) {
        setPolygon(QPolygonF());
        return;
    }

    QLineF normal = centerLine.normalVector();
    normal.setLength(m_thickness / 2.0);
    const QPointF offset = normal.p2() - normal.p1();

    const QPointF p1 = m_start + offset;
    const QPointF p2 = m_end + offset;
    const QPointF p3 = m_end - offset;
    const QPointF p4 = m_start - offset;

    setPolygon(QPolygonF() << p1 << p2 << p3 << p4);
}
