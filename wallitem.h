#ifndef WALLITEM_H
#define WALLITEM_H

#include <QBrush>
#include <QGraphicsPolygonItem>
#include <QList>
#include <QPen>
#include <QString>

class OpeningItem;
class QJsonObject;

class WallItem : public QGraphicsPolygonItem
{
public:
    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    WallItem(const QPointF &start,
             const QPointF &end,
             qreal thickness = 30.0,
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

    QString id() const;
    void setId(const QString &id);
    QJsonObject toJson() const;
    static WallItem *fromJson(const QJsonObject &json);

    void setHighlighted(bool highlighted);
    bool isHighlighted() const;

private:
    void syncOpenings();
    void ensureId();

    QPointF m_start;
    QPointF m_end;
    QString m_id;
    qreal m_thickness;
    qreal m_height;
    QList<OpeningItem *> m_openings;
    QPen m_basePen;
    QBrush m_baseBrush;
    bool m_highlighted;
};

#endif // WALLITEM_H
