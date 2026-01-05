#include "view2dwidget.h"

#include <cmath>

#include <QColor>
#include <QKeyEvent>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QVector>
#include <QWheelEvent>

namespace {
constexpr qreal kMinScale = 0.05;
constexpr qreal kMaxScale = 50.0;
constexpr qreal kScaleStep = 1.15;
}

View2DWidget::View2DWidget(QWidget *parent)
    : QGraphicsView(parent)
    , m_panning(false)
    , m_spacePressed(false)
    , m_smallGrid(100.0)
    , m_largeGrid(1000.0)
{
    setRenderHint(QPainter::Antialiasing, true);
    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::NoDrag);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
}

void View2DWidget::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);

    painter->fillRect(rect, QColor(233, 238, 242));

    const qreal left = std::floor(rect.left() / m_smallGrid) * m_smallGrid;
    const qreal right = std::ceil(rect.right() / m_smallGrid) * m_smallGrid;
    const qreal top = std::floor(rect.top() / m_smallGrid) * m_smallGrid;
    const qreal bottom = std::ceil(rect.bottom() / m_smallGrid) * m_smallGrid;

    QVector<QLineF> minorLines;
    QVector<QLineF> majorLines;

    for (qreal x = left; x <= right; x += m_smallGrid) {
        if (std::fmod(std::abs(x), m_largeGrid) < 0.001) {
            majorLines.append(QLineF(x, top, x, bottom));
        } else {
            minorLines.append(QLineF(x, top, x, bottom));
        }
    }

    for (qreal y = top; y <= bottom; y += m_smallGrid) {
        if (std::fmod(std::abs(y), m_largeGrid) < 0.001) {
            majorLines.append(QLineF(left, y, right, y));
        } else {
            minorLines.append(QLineF(left, y, right, y));
        }
    }

    painter->setPen(QPen(QColor(212, 220, 228), 0));
    painter->drawLines(minorLines);

    painter->setPen(QPen(QColor(184, 195, 206), 0));
    painter->drawLines(majorLines);

    painter->restore();
}

void View2DWidget::wheelEvent(QWheelEvent *event)
{
    const qreal direction = event->angleDelta().y();
    if (direction == 0.0) {
        event->ignore();
        return;
    }

    const qreal currentScale = transform().m11();
    const qreal factor = direction > 0 ? kScaleStep : 1.0 / kScaleStep;
    const qreal nextScale = currentScale * factor;

    if (nextScale < kMinScale || nextScale > kMaxScale) {
        event->accept();
        return;
    }

    scale(factor, factor);
    emit zoomChanged(nextScale);
    event->accept();
}

void View2DWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && m_spacePressed)) {
        m_panning = true;
        m_lastMousePos = event->pos();
        updateCursor();
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void View2DWidget::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMovedInScene(mapToScene(event->pos()));

    if (m_panning) {
        const QPointF delta = mapToScene(m_lastMousePos) - mapToScene(event->pos());
        translate(delta.x(), delta.y());
        m_lastMousePos = event->pos();
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void View2DWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_panning && (event->button() == Qt::MiddleButton ||
                      event->button() == Qt::LeftButton)) {
        m_panning = false;
        updateCursor();
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void View2DWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && !m_spacePressed) {
        m_spacePressed = true;
        updateCursor();
        event->accept();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

void View2DWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && m_spacePressed) {
        m_spacePressed = false;
        updateCursor();
        event->accept();
        return;
    }

    QGraphicsView::keyReleaseEvent(event);
}

void View2DWidget::updateCursor()
{
    if (m_panning) {
        setCursor(Qt::ClosedHandCursor);
    } else if (m_spacePressed) {
        setCursor(Qt::OpenHandCursor);
    } else {
        unsetCursor();
    }
}
