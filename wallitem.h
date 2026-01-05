#ifndef WALLITEM_H
#define WALLITEM_H

#include <QGraphicsPolygonItem>

class WallItem : public QGraphicsPolygonItem
{
public:
    WallItem(const QPointF &start,
             const QPointF &end,
             qreal thickness = 40.0,
             qreal height = 200.0,
             QGraphicsItem *parent = nullptr);

    void setStartPos(const QPointF &pos);
    void setEndPos(const QPointF &pos);
    QPointF startPos() const;
    QPointF endPos() const;

    void setThickness(qreal thickness);
    qreal thickness() const;

    void setHeight(qreal height);
    qreal height() const;

    qreal length() const;
    qreal angleDegrees() const;
    void setAngleDegrees(qreal angle);
    void updateGeometry();

private:
    QPointF m_start;
    QPointF m_end;
    qreal m_thickness;
    qreal m_height;
};

#endif // WALLITEM_H
