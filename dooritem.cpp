#include "dooritem.h"

DoorItem::DoorItem(DoorStyle style,
                   qreal width,
                   qreal height,
                   QGraphicsItem *parent)
    : OpeningItem(Kind::Door,
                  style == DoorStyle::Single
                      ? Style::SingleDoor
                      : (style == DoorStyle::Double ? Style::DoubleDoor
                                                    : Style::SlidingDoor),
                  width,
                  height,
                  0.0,
                  parent)
    , m_doorStyle(style)
{
}

DoorItem::DoorStyle DoorItem::doorStyle() const
{
    return m_doorStyle;
}
