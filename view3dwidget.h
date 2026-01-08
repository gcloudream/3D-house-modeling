#ifndef VIEW3DWIDGET_H
#define VIEW3DWIDGET_H

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPoint>
#include <QSurfaceFormat>
#include <QVector3D>

class DesignScene;
class WallItem;
class OpeningItem;

struct VertexData {
    QVector3D position;
    QVector3D normal;
};

class View3DWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit View3DWidget(QWidget *parent = nullptr);
    void setScene(DesignScene *scene);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void scheduleSync();

private:
    void rebuildGeometry();
    void appendWallMesh(const WallItem *wall, QVector<VertexData> &vertices);
    void appendWallSegment(const WallItem *wall,
                           qreal startDistance,
                           qreal endDistance,
                           qreal baseY,
                           qreal height,
                           QVector<VertexData> &vertices) const;
    void appendOpeningMesh(const WallItem *wall,
                           const OpeningItem *opening,
                           QVector<VertexData> &solidVertices,
                           QVector<VertexData> &glassVertices) const;
    QMatrix4x4 viewMatrix() const;

    DesignScene *m_scene;
    QOpenGLShaderProgram m_program;
    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;
    QVector<VertexData> m_vertices;
    QVector<VertexData> m_wallVertices;
    QVector<VertexData> m_doorSingleVertices;
    QVector<VertexData> m_doorDoubleVertices;
    QVector<VertexData> m_doorSlidingVertices;
    QVector<VertexData> m_openingVertices;
    QVector<VertexData> m_glassCasementVertices;
    QVector<VertexData> m_glassSlidingVertices;
    QVector<VertexData> m_glassBayVertices;
    bool m_geometryDirty;
    int m_vertexCount;
    QMatrix4x4 m_projection;
    struct MeshRanges {
        int wallStart = 0;
        int wallCount = 0;
        int doorSingleStart = 0;
        int doorSingleCount = 0;
        int doorDoubleStart = 0;
        int doorDoubleCount = 0;
        int doorSlidingStart = 0;
        int doorSlidingCount = 0;
        int openingStart = 0;
        int openingCount = 0;
        int glassCasementStart = 0;
        int glassCasementCount = 0;
        int glassSlidingStart = 0;
        int glassSlidingCount = 0;
        int glassBayStart = 0;
        int glassBayCount = 0;
    } m_ranges;

    float m_distance;
    float m_yaw;
    float m_pitch;
    QVector3D m_target;
    QPoint m_lastMousePos;
    bool m_leftDown;
    bool m_rightDown;
};

#endif // VIEW3DWIDGET_H
