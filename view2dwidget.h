#ifndef VIEW2DWIDGET_H
#define VIEW2DWIDGET_H

#include <QGraphicsView>

class View2DWidget : public QGraphicsView
{
    Q_OBJECT

public:
    explicit View2DWidget(QWidget *parent = nullptr);

signals:
    void mouseMovedInScene(const QPointF &pos);
    void zoomChanged(qreal scale);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void updateCursor();

    bool m_panning;
    bool m_spacePressed;
    QPoint m_lastMousePos;
    qreal m_smallGrid;
    qreal m_largeGrid;
};

#endif // VIEW2DWIDGET_H
