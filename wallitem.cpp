#include "wallitem.h"

#include "openingitem.h"

#include <QBrush>
#include <QGraphicsScene>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineF>
#include <QPen>
#include <QPolygonF>
#include <QSet>
#include <QUuid>
#include <QtGlobal>

namespace {
constexpr qreal kJointTolerance = 0.5;
constexpr qreal kMinWallLength = 0.1;
constexpr qreal kMiterLimitRatio = 4.0;

struct NeighborInfo {
  WallItem *wall = nullptr;
  QPointF dirAlong;
  qreal thickness = 0.0;
};

struct JoinPoints {
  QPointF left;
  QPointF right;
};

qreal crossProduct(const QPointF &a, const QPointF &b) {
  return a.x() * b.y() - a.y() * b.x();
}

bool lineIntersection(const QPointF &p1, const QPointF &d1, const QPointF &p2,
                      const QPointF &d2, QPointF *intersection) {
  const qreal denom = crossProduct(d1, d2);
  if (qAbs(denom) < 1e-6) {
    return false;
  }

  const QPointF delta = p2 - p1;
  const qreal t = crossProduct(delta, d2) / denom;
  if (intersection) {
    *intersection = p1 + d1 * t;
  }
  return true;
}

class UpdateGuard {
public:
  UpdateGuard(QSet<WallItem *> &set, WallItem *item)
      : m_set(set), m_item(item) {
    m_set.insert(item);
  }

  ~UpdateGuard() { m_set.remove(m_item); }

private:
  QSet<WallItem *> &m_set;
  WallItem *m_item;
};
} // namespace

WallItem::WallItem(const QPointF &start, const QPointF &end, qreal thickness,
                   qreal height, QGraphicsItem *parent)
    : QGraphicsPolygonItem(parent), m_start(start), m_end(end), m_id(),
      m_thickness(thickness), m_height(height), m_openings(),
      m_basePen(QColor(63, 73, 84), 1.2), m_baseBrush(QColor(186, 195, 205)),
      m_highlighted(false) {
  ensureId();
  setFlags(QGraphicsItem::ItemIsSelectable);
  setPen(m_basePen);
  setBrush(m_baseBrush);
  updateGeometry();
}

void WallItem::setStartPos(const QPointF &pos) { m_start = pos; }

void WallItem::setEndPos(const QPointF &pos) { m_end = pos; }

QPointF WallItem::startPos() const { return m_start; }

QPointF WallItem::endPos() const { return m_end; }

void WallItem::setThickness(qreal thickness) {
  m_thickness = thickness;
  updateGeometry();
}

qreal WallItem::thickness() const { return m_thickness; }

void WallItem::setHeight(qreal height) { m_height = height; }

qreal WallItem::height() const { return m_height; }

qreal WallItem::length() const { return QLineF(m_start, m_end).length(); }

qreal WallItem::angleDegrees() const { return QLineF(m_start, m_end).angle(); }

void WallItem::setAngleDegrees(qreal angle) {
  QLineF line(m_start, m_end);
  if (line.length() < 0.1) {
    return;
  }

  line.setAngle(angle);
  m_end = line.p2();
  updateGeometry();
}

void WallItem::updateGeometry() {
  static QSet<WallItem *> s_updating;
  if (s_updating.contains(this)) {
    return;
  }
  UpdateGuard guard(s_updating, this);

  QLineF centerLine(m_start, m_end);
  if (centerLine.length() < kMinWallLength) {
    setPolygon(QPolygonF());
    return;
  }

  const QPointF forward = (m_end - m_start) / centerLine.length();
  const QPointF backward(-forward.x(), -forward.y());

  auto findNeighbor = [this](const QPointF &joint) -> NeighborInfo {
    NeighborInfo info;
    if (!scene()) {
      return info;
    }

    qreal bestDist = kJointTolerance + 1.0;
    const QList<QGraphicsItem *> sceneItems = scene()->items();
    for (QGraphicsItem *item : sceneItems) {
      auto *wall = qgraphicsitem_cast<WallItem *>(item);
      if (!wall || wall == this) {
        continue;
      }

      const QPointF otherStart = wall->startPos();
      const QPointF otherEnd = wall->endPos();
      const qreal distStart = QLineF(joint, otherStart).length();
      const qreal distEnd = QLineF(joint, otherEnd).length();
      const qreal dist = qMin(distStart, distEnd);
      if (dist > kJointTolerance || dist >= bestDist) {
        continue;
      }

      const QPointF other = (distStart <= distEnd) ? otherEnd : otherStart;
      QLineF neighborLine(joint, other);
      if (neighborLine.length() < kMinWallLength) {
        continue;
      }

      info.wall = wall;
      info.dirAlong = (other - joint) / neighborLine.length();
      info.thickness = wall->thickness();
      bestDist = dist;
    }

    return info;
  };

  auto computeJoinPoints = [](const QPointF &joint, const QPointF &selfDir,
                              qreal selfHalf,
                              const NeighborInfo &neighbor) -> JoinPoints {
    const QPointF selfNormal(-selfDir.y(), selfDir.x());
    JoinPoints points;
    points.left = joint + selfNormal * selfHalf;
    points.right = joint - selfNormal * selfHalf;

    if (!neighbor.wall) {
      return points;
    }

    const QPointF otherDir = neighbor.dirAlong;
    const QPointF otherNormal(-otherDir.y(), otherDir.x());
    const qreal otherHalf = neighbor.thickness / 2.0;
    const qreal miterLimit = qMax(selfHalf, otherHalf) * kMiterLimitRatio;

    // 计算叉积来判断转向方向
    const qreal cross = crossProduct(selfDir, otherDir);
    if (qAbs(cross) < 1e-6) {
      // 平行或反平行，不需要斜接
      return points;
    }

    // 计算两侧的斜接交点
    // 对于外角：需要延伸边线到交点
    // 对于内角：需要缩短边线到交点

    QPointF leftIntersection, rightIntersection;

    // 左侧边线交点：self的左边与other的左边
    bool hasLeftIntersection =
        lineIntersection(points.left, selfDir, joint + otherNormal * otherHalf,
                         otherDir, &leftIntersection);

    // 右侧边线交点：self的右边与other的右边
    bool hasRightIntersection =
        lineIntersection(points.right, selfDir, joint - otherNormal * otherHalf,
                         otherDir, &rightIntersection);

    // 应用斜接（无论是外角延伸还是内角缩短）
    if (hasLeftIntersection) {
      qreal distToJoint = QLineF(joint, leftIntersection).length();
      if (distToJoint <= miterLimit) {
        points.left = leftIntersection;
      }
    }

    if (hasRightIntersection) {
      qreal distToJoint = QLineF(joint, rightIntersection).length();
      if (distToJoint <= miterLimit) {
        points.right = rightIntersection;
      }
    }

    // 外角处理：当同侧边线不相交时，尝试交叉计算
    // 这处理了外角需要延伸的情况
    if (cross > 0) {
      // 顺时针转向：右侧是外角
      QPointF outerIntersection;
      if (lineIntersection(points.right, selfDir,
                           joint + otherNormal * otherHalf, otherDir,
                           &outerIntersection)) {
        qreal dist = QLineF(joint, outerIntersection).length();
        if (dist <= miterLimit && dist > QLineF(joint, points.right).length()) {
          points.right = outerIntersection;
        }
      }
    } else {
      // 逆时针转向：左侧是外角
      QPointF outerIntersection;
      if (lineIntersection(points.left, selfDir,
                           joint - otherNormal * otherHalf, otherDir,
                           &outerIntersection)) {
        qreal dist = QLineF(joint, outerIntersection).length();
        if (dist <= miterLimit && dist > QLineF(joint, points.left).length()) {
          points.left = outerIntersection;
        }
      }
    }

    return points;
  };

  // Build a mitered polygon by intersecting offset lines at connected joints.
  const NeighborInfo startNeighbor = findNeighbor(m_start);
  const NeighborInfo endNeighbor = findNeighbor(m_end);

  const qreal half = m_thickness / 2.0;
  const JoinPoints startJoin =
      computeJoinPoints(m_start, forward, half, startNeighbor);
  const JoinPoints endJoin =
      computeJoinPoints(m_end, backward, half, endNeighbor);

  const QPointF startLeft = startJoin.left;
  const QPointF startRight = startJoin.right;
  const QPointF endLeft = endJoin.right;
  const QPointF endRight = endJoin.left;

  setPolygon(QPolygonF() << startLeft << endLeft << endRight << startRight);
  syncOpenings();

  if (startNeighbor.wall && !s_updating.contains(startNeighbor.wall)) {
    startNeighbor.wall->updateGeometry();
  }
  if (endNeighbor.wall && endNeighbor.wall != startNeighbor.wall &&
      !s_updating.contains(endNeighbor.wall)) {
    endNeighbor.wall->updateGeometry();
  }
}

void WallItem::addOpening(OpeningItem *opening) {
  if (!opening || m_openings.contains(opening)) {
    return;
  }
  m_openings.append(opening);
  opening->setWall(this);
  opening->syncWithWall();
}

void WallItem::removeOpening(OpeningItem *opening) {
  if (!opening) {
    return;
  }
  m_openings.removeAll(opening);
  if (opening->wall() == this) {
    opening->setWall(nullptr);
  }
}

QList<OpeningItem *> WallItem::openings() const { return m_openings; }

QString WallItem::id() const { return m_id; }

void WallItem::setId(const QString &id) {
  m_id = id;
  ensureId();
}

QJsonObject WallItem::toJson() const {
  QJsonObject obj;
  obj["id"] = m_id;
  obj["start"] = QJsonArray{m_start.x(), m_start.y()};
  obj["end"] = QJsonArray{m_end.x(), m_end.y()};
  obj["thickness"] = m_thickness;
  obj["height"] = m_height;
  return obj;
}

WallItem *WallItem::fromJson(const QJsonObject &json) {
  const QJsonArray startArray = json.value("start").toArray();
  const QJsonArray endArray = json.value("end").toArray();
  const QPointF start(startArray.size() > 0 ? startArray.at(0).toDouble() : 0.0,
                      startArray.size() > 1 ? startArray.at(1).toDouble()
                                            : 0.0);
  const QPointF end(endArray.size() > 0 ? endArray.at(0).toDouble() : 0.0,
                    endArray.size() > 1 ? endArray.at(1).toDouble() : 0.0);
  const qreal thickness = json.value("thickness").toDouble(30.0);
  const qreal height = json.value("height").toDouble(200.0);

  WallItem *wall = new WallItem(start, end, thickness, height);
  wall->setId(json.value("id").toString());
  return wall;
}

void WallItem::setHighlighted(bool highlighted) {
  if (m_highlighted == highlighted) {
    return;
  }
  m_highlighted = highlighted;
  if (m_highlighted) {
    setPen(QPen(QColor(47, 110, 143), 2.0));
    setBrush(QBrush(QColor(200, 214, 226)));
  } else {
    setPen(m_basePen);
    setBrush(m_baseBrush);
  }
  update();
}

bool WallItem::isHighlighted() const { return m_highlighted; }

void WallItem::syncOpenings() {
  for (OpeningItem *opening : m_openings) {
    if (opening) {
      opening->syncWithWall();
    }
  }
}

void WallItem::ensureId() {
  if (m_id.isEmpty()) {
    m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  }
}
