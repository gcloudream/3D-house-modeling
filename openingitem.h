#ifndef OPENINGITEM_H
#define OPENINGITEM_H

#include <QGraphicsItem>
#include <QPolygonF>
#include <QString>

class WallItem;

class OpeningItem : public QGraphicsItem
{
public:
    enum class Kind {
        Door,
        Window
    };

    enum class Style {
        SingleDoor,
        DoubleDoor,
        SlidingDoor,
        CasementWindow,
        SlidingWindow,
        BayWindow
    };

    OpeningItem(Kind kind,
                Style style,
                qreal width,
                qreal height,
                qreal sillHeight = 0.0,
                QGraphicsItem *parent = nullptr);

    Kind kind() const;
    Style style() const;
    QString kindLabel() const;
    QString styleLabel() const;

    void setWall(WallItem *wall);
    WallItem *wall() const;

    void setDistanceFromStart(qreal distance);
    qreal distanceFromStart() const;

    void setWidth(qreal width);
    qreal width() const;

    void setHeight(qreal height);
    qreal height() const;

    void setSillHeight(qreal height);
    qreal sillHeight() const;

    void setPreview(bool preview);
    bool isPreview() const;

    void setFlipped(bool flipped);
    bool isFlipped() const;
    void toggleFlip();

    void syncWithWall();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QRectF openingRect() const;
    QRectF doorSwingRect() const;
    QRectF flipHandleRect() const;
    QRectF bayProtrusionRect() const;
    QPolygonF bayOutline() const;
    qreal wallThickness() const;
    qreal wallLength() const;
    qreal wallAngle() const;
    QPointF startPointForDistance(qreal distance) const;
    qreal projectDistance(const QPointF &scenePos) const;
    qreal clampDistance(qreal distance) const;
    void updateFlags();

    Kind m_kind;
    Style m_style;
    qreal m_width;
    qreal m_height;
    qreal m_sillHeight;
    qreal m_distance;
    bool m_preview;
    bool m_flipped;
    bool m_ignorePositionChange;
    WallItem *m_wall;
};

#endif // OPENINGITEM_H
