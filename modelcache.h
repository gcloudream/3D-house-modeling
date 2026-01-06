#ifndef MODELCACHE_H
#define MODELCACHE_H

#include <QHash>
#include <QSharedPointer>
#include <QVector>
#include <QVector3D>

struct MeshData {
    QVector<QVector3D> vertices;
    QVector3D minBounds;
    QVector3D maxBounds;

    QVector3D size() const {
        return maxBounds - minBounds;
    }

    QVector3D pivotOffset() const {
        return QVector3D(-(minBounds.x() + maxBounds.x()) * 0.5f,
                         -minBounds.y(),
                         -(minBounds.z() + maxBounds.z()) * 0.5f);
    }
};

class ModelCache
{
public:
    static ModelCache *instance();

    QSharedPointer<MeshData> getModel(const QString &path,
                                      QString *errorMessage = nullptr);

private:
    ModelCache() = default;

    QSharedPointer<MeshData> loadModel(const QString &path,
                                       QString *errorMessage = nullptr);
    QSharedPointer<MeshData> placeholderModel();

    QHash<QString, QSharedPointer<MeshData>> m_cache;
    QSharedPointer<MeshData> m_placeholder;
};

#endif // MODELCACHE_H
