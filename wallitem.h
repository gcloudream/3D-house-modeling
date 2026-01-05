#ifndef WALLITEM_H
#define WALLITEM_H

#include <QBrush>
#include <QGraphicsPolygonItem>
#include <QList>
#include <QPen>

class OpeningItem;

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

    void addOpening(OpeningItem *opening);
    void removeOpening(OpeningItem *opening);
    QList<OpeningItem *> openings() const;

    void setHighlighted(bool highlighted);
    bool isHighlighted() const;

private:
    void syncOpenings();

    QPointF m_start;
    QPointF m_end;
    qreal m_thickness;
    qreal m_height;
    QList<OpeningItem *> m_openings;
    QPen m_basePen;
    QBrush m_baseBrush;
    bool m_highlighted;
};

#endif // WALLITEM_H
