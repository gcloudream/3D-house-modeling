#ifndef DOORITEM_H
#define DOORITEM_H

#include "openingitem.h"

class DoorItem : public OpeningItem
{
public:
    enum class DoorStyle {
        Single,
        Double,
        Sliding
    };

    explicit DoorItem(DoorStyle style = DoorStyle::Single,
                      qreal width = 64.0,
                      qreal height = 150.0,
                      QGraphicsItem *parent = nullptr);

    DoorStyle doorStyle() const;

private:
    DoorStyle m_doorStyle;
};

#endif // DOORITEM_H
