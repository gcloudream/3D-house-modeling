#include "view3dwidget.h"

#include "designscene.h"
#include "openingitem.h"
#include "wallitem.h"

#include <algorithm>
#include <cmath>

#include <QGraphicsItem>
#include <QLineF>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QOpenGLShader>
#include <QVector2D>
#include <QWheelEvent>
#include <QtGlobal>
#include <QtMath>

namespace {
constexpr float kMinDistance = 200.0f;
constexpr float kMaxDistance = 200000.0f;
constexpr float kRotateSpeed = 0.4f;
constexpr float kPanSpeedFactor = 0.002f;
constexpr float kZoomStep = 0.9f;

void appendBoxFromQuad(const QPointF &p1,
                       const QPointF &p2,
                       const QPointF &p3,
                       const QPointF &p4,
                       qreal baseY,
                       qreal height,
                       QVector<QVector3D> &vertices)
{
    auto to3d = [baseY](const QPointF &p) {
        return QVector3D(p.x(), static_cast<float>(baseY), -p.y());
    };

    const QVector3D b1 = to3d(p1);
    const QVector3D b2 = to3d(p2);
    const QVector3D b3 = to3d(p3);
    const QVector3D b4 = to3d(p4);

    const QVector3D topOffset(0.0f, static_cast<float>(height), 0.0f);
    const QVector3D t1 = b1 + topOffset;
    const QVector3D t2 = b2 + topOffset;
    const QVector3D t3 = b3 + topOffset;
    const QVector3D t4 = b4 + topOffset;

    vertices << t1 << t2 << t3;
    vertices << t1 << t3 << t4;
    vertices << b1 << b3 << b2;
    vertices << b1 << b4 << b3;
    vertices << b1 << b2 << t2;
    vertices << b1 << t2 << t1;
    vertices << b2 << b3 << t3;
    vertices << b2 << t3 << t2;
    vertices << b3 << b4 << t4;
    vertices << b3 << t4 << t3;
    vertices << b4 << b1 << t1;
    vertices << b4 << t1 << t4;
}
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
        "uniform float u_alpha;\n"
        "void main() {\n"
        "    FragColor = vec4(u_color, u_alpha);\n"
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

    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);
    if (m_ranges.wallCount > 0) {
        m_program.setUniformValue("u_color", QVector3D(0.62f, 0.68f, 0.75f));
        m_program.setUniformValue("u_alpha", 1.0f);
        glDrawArrays(GL_TRIANGLES, m_ranges.wallStart, m_ranges.wallCount);
    }

    if (m_ranges.openingCount > 0) {
        m_program.setUniformValue("u_color", QVector3D(0.45f, 0.50f, 0.56f));
        m_program.setUniformValue("u_alpha", 1.0f);
        glDrawArrays(GL_TRIANGLES, m_ranges.openingStart, m_ranges.openingCount);
    }

    if (m_ranges.glassCount > 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_program.setUniformValue("u_color", QVector3D(0.55f, 0.74f, 0.86f));
        m_program.setUniformValue("u_alpha", 0.35f);
        glDrawArrays(GL_TRIANGLES, m_ranges.glassStart, m_ranges.glassCount);
        glDisable(GL_BLEND);
    }
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
    m_wallVertices.clear();
    m_openingVertices.clear();
    m_glassVertices.clear();

    if (!m_scene) {
        m_vertexCount = 0;
        m_ranges = {};
        m_geometryDirty = false;
        return;
    }

    const QList<QGraphicsItem *> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        auto *wall = qgraphicsitem_cast<WallItem *>(item);
        if (!wall) {
            continue;
        }
        appendWallMesh(wall, m_wallVertices);
    }

    m_ranges.wallStart = 0;
    m_ranges.wallCount = m_wallVertices.size();
    m_ranges.openingStart = m_ranges.wallCount;
    m_ranges.openingCount = m_openingVertices.size();
    m_ranges.glassStart = m_ranges.openingStart + m_ranges.openingCount;
    m_ranges.glassCount = m_glassVertices.size();

    m_vertices.reserve(m_ranges.glassStart + m_ranges.glassCount);
    m_vertices << m_wallVertices << m_openingVertices << m_glassVertices;

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
                                  QVector<QVector3D> &vertices)
{
    if (!wall) {
        return;
    }

    const QLineF line(wall->startPos(), wall->endPos());
    const qreal totalLength = line.length();
    if (totalLength < 0.1) {
        return;
    }

    QList<OpeningItem *> openings = wall->openings();
    if (openings.isEmpty()) {
        appendWallSegment(wall, 0.0, totalLength, vertices);
        return;
    }

    std::sort(openings.begin(), openings.end(),
              [](OpeningItem *a, OpeningItem *b) {
                  if (!a || !b) {
                      return a && !b;
                  }
                  return a->distanceFromStart() < b->distanceFromStart();
              });

    qreal cursor = 0.0;
    for (OpeningItem *opening : openings) {
        if (!opening) {
            continue;
        }
        qreal start = qBound(0.0, opening->distanceFromStart(), totalLength);
        qreal end = qBound(0.0, start + opening->width(), totalLength);
        if (start > cursor) {
            appendWallSegment(wall, cursor, start, vertices);
        }
        cursor = qMax(cursor, end);
        appendOpeningMesh(wall, opening, m_openingVertices, m_glassVertices);
    }

    if (cursor < totalLength) {
        appendWallSegment(wall, cursor, totalLength, vertices);
    }
}

void View3DWidget::appendWallSegment(const WallItem *wall,
                                     qreal startDistance,
                                     qreal endDistance,
                                     QVector<QVector3D> &vertices) const
{
    if (!wall || endDistance - startDistance < 0.1) {
        return;
    }

    const QLineF line(wall->startPos(), wall->endPos());
    const qreal length = line.length();
    if (length < 0.1) {
        return;
    }

    const QPointF dir = (line.p2() - line.p1()) / length;
    const QPointF segStart = line.p1() + dir * startDistance;
    const QPointF segEnd = line.p1() + dir * endDistance;

    QLineF segLine(segStart, segEnd);
    QLineF normal = segLine.normalVector();
    normal.setLength(wall->thickness() / 2.0);
    const QPointF offset = normal.p2() - normal.p1();

    const QPointF p1 = segStart + offset;
    const QPointF p2 = segEnd + offset;
    const QPointF p3 = segEnd - offset;
    const QPointF p4 = segStart - offset;

    appendBoxFromQuad(p1, p2, p3, p4, 0.0, wall->height(), vertices);
}

void View3DWidget::appendOpeningMesh(const WallItem *wall,
                                     const OpeningItem *opening,
                                     QVector<QVector3D> &solidVertices,
                                     QVector<QVector3D> &glassVertices) const
{
    if (!wall || !opening) {
        return;
    }

    const QLineF line(wall->startPos(), wall->endPos());
    const qreal length = line.length();
    if (length < 0.1) {
        return;
    }

    qreal start = qBound(0.0, opening->distanceFromStart(), length);
    qreal end = qBound(0.0, start + opening->width(), length);
    if (end - start < 0.1) {
        return;
    }

    const QPointF wallDir = (line.p2() - line.p1()) / length;
    const QPointF segStart = line.p1() + wallDir * start;
    const QPointF segEnd = line.p1() + wallDir * end;

    QLineF segLine(segStart, segEnd);
    const qreal baseHeight =
        opening->kind() == OpeningItem::Kind::Door ? 0.0 : opening->sillHeight();

    const auto buildQuad = [&segLine](const QPointF &a,
                                      const QPointF &b,
                                      qreal thickness,
                                      const QPointF &shift,
                                      QPointF &o1,
                                      QPointF &o2,
                                      QPointF &o3,
                                      QPointF &o4) {
        QLineF normal = segLine.normalVector();
        normal.setLength(thickness / 2.0);
        const QPointF offset = normal.p2() - normal.p1();
        o1 = a + offset + shift;
        o2 = b + offset + shift;
        o3 = b - offset + shift;
        o4 = a - offset + shift;
    };

    if (opening->kind() == OpeningItem::Kind::Door) {
        const qreal panelThickness = qMin(40.0, wall->thickness() * 0.6);
        QPointF p1, p2, p3, p4;
        buildQuad(segStart, segEnd, panelThickness, QPointF(), p1, p2, p3, p4);
        appendBoxFromQuad(p1, p2, p3, p4, baseHeight, opening->height(),
                          solidVertices);
        return;
    }

    const qreal frameThickness = qMin(24.0, wall->thickness() * 0.5);
    QLineF frameNormal = segLine.normalVector();
    frameNormal.setLength(frameThickness / 2.0);
    const QPointF frameOffset = frameNormal.p2() - frameNormal.p1();
    QVector2D outwardVec(frameOffset);
    if (outwardVec.lengthSquared() > 0.0001f) {
        outwardVec.normalize();
    }
    const QPointF outward(outwardVec.x(), outwardVec.y());

    QPointF f1, f2, f3, f4;
    QPointF glassShift;
    qreal glassThickness = qMax(3.0, frameThickness * 0.25);

    if (opening->style() == OpeningItem::Style::BayWindow) {
        const qreal bayDepth = qMax(12.0, wall->thickness() * 0.6);
        const qreal bayThickness = frameThickness + bayDepth;
        const QPointF bayShift = outward * (bayDepth * 0.5);
        buildQuad(segStart, segEnd, bayThickness, bayShift, f1, f2, f3, f4);
        appendBoxFromQuad(f1, f2, f3, f4, baseHeight, opening->height(),
                          solidVertices);
        glassShift = outward * (bayDepth * 0.75);
    } else {
        buildQuad(segStart, segEnd, frameThickness, QPointF(), f1, f2, f3, f4);
        appendBoxFromQuad(f1, f2, f3, f4, baseHeight, opening->height(),
                          solidVertices);
        glassShift = QPointF();
    }

    const qreal segmentLength = QLineF(segStart, segEnd).length();
    if (segmentLength < 0.1) {
        return;
    }
    const qreal insetAlong = qMax(4.0, segmentLength * 0.08);
    const QPointF segDir = (segEnd - segStart) / segmentLength;
    const qreal safeInset = qMin(insetAlong, segmentLength * 0.25);

    if (opening->style() == OpeningItem::Style::SlidingWindow) {
        const qreal panelWidth = segmentLength * 0.55;
        const QPointF leftStart = segStart;
        const QPointF leftEnd = segStart + segDir * panelWidth;
        const QPointF rightEnd = segEnd;
        const QPointF rightStart = segEnd - segDir * panelWidth;
        const QPointF layerShift = outward * (frameThickness * 0.2);

        QPointF l1, l2, l3, l4;
        buildQuad(leftStart + segDir * safeInset,
                  leftEnd - segDir * safeInset,
                  glassThickness,
                  glassShift + layerShift,
                  l1, l2, l3, l4);
        appendBoxFromQuad(l1, l2, l3, l4, baseHeight, opening->height(),
                          glassVertices);

        QPointF r1, r2, r3, r4;
        buildQuad(rightStart + segDir * safeInset,
                  rightEnd - segDir * safeInset,
                  glassThickness,
                  glassShift - layerShift,
                  r1, r2, r3, r4);
        appendBoxFromQuad(r1, r2, r3, r4, baseHeight, opening->height(),
                          glassVertices);
        return;
    }

    QPointF g1, g2, g3, g4;
    buildQuad(segStart + segDir * safeInset,
              segEnd - segDir * safeInset,
              glassThickness,
              glassShift,
              g1, g2, g3, g4);
    appendBoxFromQuad(g1, g2, g3, g4, baseHeight, opening->height(),
                      glassVertices);

    if (opening->style() == OpeningItem::Style::CasementWindow) {
        const qreal barWidth = qMax(4.0, opening->width() * 0.08);
        const qreal barStartOffset = opening->width() * 0.22;
        const QPointF barStart = segStart + segDir * barStartOffset;
        const QPointF barEnd = barStart + segDir * barWidth;
        QPointF b1, b2, b3, b4;
        buildQuad(barStart,
                  barEnd,
                  frameThickness * 0.35,
                  QPointF(),
                  b1, b2, b3, b4);
        appendBoxFromQuad(b1, b2, b3, b4, baseHeight, opening->height(),
                          solidVertices);
    }
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
