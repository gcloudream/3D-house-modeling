#ifndef WINDOWITEM_H
#define WINDOWITEM_H

#include "openingitem.h"

class WindowItem : public OpeningItem
{
public:
    enum class WindowStyle {
        Casement,
        Sliding,
        Bay
    };

    explicit WindowItem(WindowStyle style = WindowStyle::Casement,
                        qreal width = 100.0,
                        qreal height = 100.0,
                        qreal sillHeight = 50.0,
                        QGraphicsItem *parent = nullptr);

    WindowStyle windowStyle() const;

private:
    WindowStyle m_windowStyle;
};

#endif // WINDOWITEM_H
