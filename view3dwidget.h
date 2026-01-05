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
    void appendWallMesh(const WallItem *wall, QVector<QVector3D> &vertices);
    void appendWallSegment(const WallItem *wall,
                           qreal startDistance,
                           qreal endDistance,
                           QVector<QVector3D> &vertices) const;
    void appendOpeningMesh(const WallItem *wall,
                           const OpeningItem *opening,
                           QVector<QVector3D> &solidVertices,
                           QVector<QVector3D> &glassVertices) const;
    QMatrix4x4 viewMatrix() const;

    DesignScene *m_scene;
    QOpenGLShaderProgram m_program;
    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;
    QVector<QVector3D> m_vertices;
    QVector<QVector3D> m_wallVertices;
    QVector<QVector3D> m_openingVertices;
    QVector<QVector3D> m_glassVertices;
    bool m_geometryDirty;
    int m_vertexCount;
    QMatrix4x4 m_projection;
    struct MeshRanges {
        int wallStart = 0;
        int wallCount = 0;
        int openingStart = 0;
        int openingCount = 0;
        int glassStart = 0;
        int glassCount = 0;
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
