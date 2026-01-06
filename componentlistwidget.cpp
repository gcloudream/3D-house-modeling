#include "componentlistwidget.h"

#include <QDrag>
#include <QMimeData>

ComponentListWidget::ComponentListWidget(QWidget *parent)
    : QListWidget(parent)
{
    setDragEnabled(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
}

QString ComponentListWidget::openingMimeType()
{
    return QStringLiteral("application/x-qtplan-opening");
}

QString ComponentListWidget::furnitureMimeType()
{
    return QStringLiteral("application/x-qtplan-furniture");
}

void ComponentListWidget::startDrag(Qt::DropActions supportedActions)
{
    QListWidgetItem *item = currentItem();
    if (!item) {
        return;
    }

    const QByteArray payload = item->data(Qt::UserRole).toByteArray();
    if (payload.isEmpty()) {
        QListWidget::startDrag(supportedActions);
        return;
    }

    const QString mimeType = item->data(Qt::UserRole + 1).toString();
    auto *mimeData = new QMimeData();
    mimeData->setData(mimeType.isEmpty() ? openingMimeType() : mimeType, payload);

    auto *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    if (!item->icon().isNull()) {
        drag->setPixmap(item->icon().pixmap(32, 32));
    }
    drag->exec(Qt::CopyAction);
}
