#ifndef COMPONENTLISTWIDGET_H
#define COMPONENTLISTWIDGET_H

#include <QListWidget>

class ComponentListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit ComponentListWidget(QWidget *parent = nullptr);

    static QString openingMimeType();
    static QString furnitureMimeType();

protected:
    void startDrag(Qt::DropActions supportedActions) override;
};

#endif // COMPONENTLISTWIDGET_H
