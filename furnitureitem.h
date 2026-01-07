#ifndef FURNITUREITEM_H
#define FURNITUREITEM_H

#include <QGraphicsSvgItem>
#include <QMatrix4x4>
#include <QSizeF>
#include <QVector3D>

#include "assetmanager.h"

class QJsonObject;

class FurnitureItem : public QGraphicsSvgItem
{
public:
    enum { Type = UserType + 3 };
    int type() const override { return Type; }

    explicit FurnitureItem(const QString &assetId, QGraphicsItem *parent = nullptr);

    QString assetId() const;
    const AssetManager::Asset &asset() const;
    bool hasValidAsset() const;

    void setPreview(bool preview);
    bool isPreview() const;

    void setElevation(qreal elevation);
    qreal elevation() const;

    void setRotationDegrees(qreal angle);
    qreal rotationDegrees() const;

    void setSize2D(const QSizeF &size);
    QSizeF size2D() const;

    void setHeight3D(qreal height);
    qreal height3D() const;

    void setScale3D(const QVector3D &scale);
    QVector3D scale3D() const;

    QVector3D defaultSize3D() const;
    QVector3D modelScale(const QVector3D &modelSize) const;

    QString material() const;

    QJsonObject toJson() const;
    static FurnitureItem *fromJson(const QJsonObject &json);

    QMatrix4x4 transformMatrix() const;

    QRectF boundingRect() const override;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    enum class DragMode {
        None,
        Rotate,
        Scale,
        Move
    };

    void loadAsset(const AssetManager::Asset &asset);
    QRectF handleRect(const QPointF &center) const;
    QRectF rotationHandleRect() const;
    QList<QRectF> scaleHandleRects() const;
    QPointF itemCenter() const;
    qreal snappedAngle(qreal angle) const;
    void updateGeometry();
    void notifySceneChanged();

    QString m_assetId;
    AssetManager::Asset m_asset;
    QSizeF m_size2D;
    qreal m_height3D;
    qreal m_elevation;
    QVector3D m_scale3D;
    qreal m_rotation;
    bool m_preview;
    DragMode m_dragMode;
    QSizeF m_dragStartSize;
};

#endif // FURNITUREITEM_H
