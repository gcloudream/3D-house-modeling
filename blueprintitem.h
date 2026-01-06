#ifndef BLUEPRINTITEM_H
#define BLUEPRINTITEM_H

#include <QGraphicsPixmapItem>
#include <QString>

class BlueprintItem : public QGraphicsPixmapItem
{
public:
    enum { Type = UserType + 4 };
    int type() const override { return Type; }

    explicit BlueprintItem(const QPixmap &pixmap,
                           QGraphicsItem *parent = nullptr);

    void setSourcePath(const QString &path);
    QString sourcePath() const;

private:
    QString m_sourcePath;
};

#endif // BLUEPRINTITEM_H
