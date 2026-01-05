#include "blueprintitem.h"

BlueprintItem::BlueprintItem(const QPixmap &pixmap, QGraphicsItem *parent)
    : QGraphicsPixmapItem(pixmap, parent)
{
}

void BlueprintItem::setSourcePath(const QString &path)
{
    m_sourcePath = path;
}

QString BlueprintItem::sourcePath() const
{
    return m_sourcePath;
}
