#include "view3dwidget.h"

#include "designscene.h"
#include "wallitem.h"

#include <cmath>

#include <QGraphicsItem>
#include <QLineF>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QOpenGLShader>
#include <QWheelEvent>
#include <QtGlobal>
#include <QtMath>

namespace {
constexpr float kMinDistance = 200.0f;
constexpr float kMaxDistance = 200000.0f;
constexpr float kRotateSpeed = 0.4f;
constexpr float kPanSpeedFactor = 0.002f;
constexpr float kZoomStep = 0.9f;
}

View3DWidget::View3DWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_scene(nullptr)
    , m_vbo(QOpenGLBuffer::VertexBuffer)
    , m_geometryDirty(true)
    , m_vertexCount(0)
    , m_distance(8000.0f)
    , m_yaw(45.0f)
    , m_pitch(-20.0f)
    , m_target(0.0f, 0.0f, 0.0f)
    , m_leftDown(false)
    , m_rightDown(false)
{
    QSurfaceFormat fmt = format();
    fmt.setSamples(4);
    fmt.setDepthBufferSize(24);
    setFormat(fmt);
    setFocusPolicy(Qt::StrongFocus);
}

void View3DWidget::setScene(DesignScene *scene)
{
    if (m_scene == scene) {
        return;
    }

    if (m_scene) {
        disconnect(m_scene, nullptr, this, nullptr);
    }

    m_scene = scene;

    if (m_scene) {
        connect(m_scene, &DesignScene::sceneContentChanged,
                this, &View3DWidget::scheduleSync);
        connect(m_scene, &QObject::destroyed, this, [this]() {
            m_scene = nullptr;
            scheduleSync();
        });
    }

    scheduleSync();
}

void View3DWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glDisable(GL_CULL_FACE);
    glClearColor(0.12f, 0.15f, 0.18f, 1.0f);

    m_program.addShaderFromSourceCode(
        QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout(location = 0) in vec3 a_pos;\n"
        "uniform mat4 u_mvp;\n"
        "void main() {\n"
        "    gl_Position = u_mvp * vec4(a_pos, 1.0);\n"
        "}\n");

    m_program.addShaderFromSourceCode(
        QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 u_color;\n"
        "void main() {\n"
        "    FragColor = vec4(u_color, 1.0);\n"
        "}\n");

    m_program.link();

    m_vao.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

    m_vbo.create();
    m_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    m_vbo.bind();
    m_vbo.allocate(nullptr, 0);

    m_program.bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), nullptr);
    m_program.release();
    m_vbo.release();
}

void View3DWidget::resizeGL(int w, int h)
{
    const float aspect = h == 0 ? 1.0f : static_cast<float>(w) / static_cast<float>(h);
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, aspect, 10.0f, 200000.0f);
}

void View3DWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_geometryDirty) {
        rebuildGeometry();
    }

    if (m_vertexCount == 0) {
        return;
    }

    m_program.bind();
    const QMatrix4x4 mvp = m_projection * viewMatrix();
    m_program.setUniformValue("u_mvp", mvp);
    m_program.setUniformValue("u_color", QVector3D(0.62f, 0.68f, 0.75f));

    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    m_program.release();
}

void View3DWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_leftDown = true;
    } else if (event->button() == Qt::RightButton) {
        m_rightDown = true;
    }

    m_lastMousePos = event->pos();
    event->accept();
}

void View3DWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();

    if (m_leftDown) {
        m_yaw += delta.x() * kRotateSpeed;
        m_pitch += delta.y() * kRotateSpeed;
        m_pitch = qBound(-80.0f, m_pitch, 80.0f);
        update();
        event->accept();
        return;
    }

    if (m_rightDown) {
        const float panScale = m_distance * kPanSpeedFactor;
        m_target.setX(m_target.x() - delta.x() * panScale);
        m_target.setZ(m_target.z() + delta.y() * panScale);
        update();
        event->accept();
        return;
    }

    QOpenGLWidget::mouseMoveEvent(event);
}

void View3DWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_leftDown = false;
    } else if (event->button() == Qt::RightButton) {
        m_rightDown = false;
    }
    event->accept();
}

void View3DWidget::wheelEvent(QWheelEvent *event)
{
    const int delta = event->angleDelta().y();
    if (delta > 0) {
        m_distance = qMax(kMinDistance, m_distance * kZoomStep);
    } else if (delta < 0) {
        m_distance = qMin(kMaxDistance, m_distance / kZoomStep);
    }

    update();
    event->accept();
}

void View3DWidget::scheduleSync()
{
    m_geometryDirty = true;
    update();
}

void View3DWidget::rebuildGeometry()
{
    m_vertices.clear();

    if (!m_scene) {
        m_vertexCount = 0;
        m_geometryDirty = false;
        return;
    }

    const QList<QGraphicsItem *> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        auto *wall = qgraphicsitem_cast<WallItem *>(item);
        if (!wall) {
            continue;
        }
        appendWallMesh(wall, m_vertices);
    }

    m_vertexCount = m_vertices.size();

    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);
    m_vbo.bind();
    if (m_vertexCount > 0) {
        m_vbo.allocate(m_vertices.constData(),
                       m_vertexCount * static_cast<int>(sizeof(QVector3D)));
    } else {
        m_vbo.allocate(nullptr, 0);
    }
    m_vbo.release();
    m_geometryDirty = false;
}

void View3DWidget::appendWallMesh(const WallItem *wall,
                                  QVector<QVector3D> &vertices) const
{
    if (!wall) {
        return;
    }

    const QLineF line(wall->startPos(), wall->endPos());
    if (line.length() < 0.1) {
        return;
    }

    QLineF normal = line.normalVector();
    normal.setLength(wall->thickness() / 2.0);
    const QPointF offset = normal.p2() - normal.p1();

    const QPointF p1 = wall->startPos() + offset;
    const QPointF p2 = wall->endPos() + offset;
    const QPointF p3 = wall->endPos() - offset;
    const QPointF p4 = wall->startPos() - offset;

    auto to3d = [](const QPointF &p) {
        return QVector3D(p.x(), 0.0f, -p.y());
    };

    const QVector3D b1 = to3d(p1);
    const QVector3D b2 = to3d(p2);
    const QVector3D b3 = to3d(p3);
    const QVector3D b4 = to3d(p4);

    const QVector3D topOffset(0.0f, static_cast<float>(wall->height()), 0.0f);
    const QVector3D t1 = b1 + topOffset;
    const QVector3D t2 = b2 + topOffset;
    const QVector3D t3 = b3 + topOffset;
    const QVector3D t4 = b4 + topOffset;

    // Top
    vertices << t1 << t2 << t3;
    vertices << t1 << t3 << t4;
    // Bottom
    vertices << b1 << b3 << b2;
    vertices << b1 << b4 << b3;
    // Sides
    vertices << b1 << b2 << t2;
    vertices << b1 << t2 << t1;
    vertices << b2 << b3 << t3;
    vertices << b2 << t3 << t2;
    vertices << b3 << b4 << t4;
    vertices << b3 << t4 << t3;
    vertices << b4 << b1 << t1;
    vertices << b4 << t1 << t4;
}

QMatrix4x4 View3DWidget::viewMatrix() const
{
    const float yawRad = qDegreesToRadians(m_yaw);
    const float pitchRad = qDegreesToRadians(m_pitch);

    const QVector3D eye(
        m_target.x() + m_distance * std::cos(pitchRad) * std::cos(yawRad),
        m_target.y() + m_distance * std::sin(pitchRad),
        m_target.z() + m_distance * std::cos(pitchRad) * std::sin(yawRad));

    QMatrix4x4 view;
    view.lookAt(eye, m_target, QVector3D(0.0f, 1.0f, 0.0f));
    return view;
}
