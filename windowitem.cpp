#include "windowitem.h"

WindowItem::WindowItem(WindowStyle style,
                       qreal width,
                       qreal height,
                       qreal sillHeight,
                       QGraphicsItem *parent)
    : OpeningItem(Kind::Window,
                  style == WindowStyle::Casement
                      ? Style::CasementWindow
                      : (style == WindowStyle::Sliding ? Style::SlidingWindow
                                                       : Style::BayWindow),
                  width,
                  height,
                  sillHeight,
                  parent)
    , m_windowStyle(style)
{
}

WindowItem::WindowStyle WindowItem::windowStyle() const
{
    return m_windowStyle;
}
