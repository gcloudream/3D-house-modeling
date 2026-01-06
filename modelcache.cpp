#include "modelcache.h"

#include <QFileInfo>
#include <QString>
#include <QtGlobal>
#include <cfloat>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

ModelCache *ModelCache::instance()
{
    static ModelCache cache;
    return &cache;
}

QSharedPointer<MeshData> ModelCache::getModel(const QString &path, QString *errorMessage)
{
    if (path.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("模型路径为空");
        }
        return placeholderModel();
    }

    if (m_cache.contains(path)) {
        return m_cache.value(path);
    }

    QSharedPointer<MeshData> model = loadModel(path, errorMessage);
    if (!model || model->vertices.isEmpty()) {
        model = placeholderModel();
    }

    m_cache.insert(path, model);
    return model;
}

QSharedPointer<MeshData> ModelCache::loadModel(const QString &path, QString *errorMessage)
{
    QFileInfo info(path);
    if (!info.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("模型文件不存在: %1").arg(path);
        }
        return QSharedPointer<MeshData>();
    }

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        path.toStdString(),
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals |
        aiProcess_ImproveCacheLocality |
        aiProcess_PreTransformVertices);

    if (!scene || !scene->HasMeshes()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("模型加载失败: %1").arg(importer.GetErrorString());
        }
        return QSharedPointer<MeshData>();
    }

    auto data = QSharedPointer<MeshData>::create();
    QVector3D minBounds(FLT_MAX, FLT_MAX, FLT_MAX);
    QVector3D maxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh *mesh = scene->mMeshes[i];
        if (!mesh || !mesh->HasPositions()) {
            continue;
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace &face = mesh->mFaces[f];
            if (face.mNumIndices != 3) {
                continue;
            }
            for (unsigned int j = 0; j < 3; ++j) {
                const unsigned int idx = face.mIndices[j];
                if (idx >= mesh->mNumVertices) {
                    continue;
                }
                const aiVector3D &v = mesh->mVertices[idx];
                const QVector3D pos(v.x, v.y, v.z);
                data->vertices.append(pos);

                minBounds.setX(qMin(minBounds.x(), pos.x()));
                minBounds.setY(qMin(minBounds.y(), pos.y()));
                minBounds.setZ(qMin(minBounds.z(), pos.z()));
                maxBounds.setX(qMax(maxBounds.x(), pos.x()));
                maxBounds.setY(qMax(maxBounds.y(), pos.y()));
                maxBounds.setZ(qMax(maxBounds.z(), pos.z()));
            }
        }
    }

    if (data->vertices.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("模型无有效三角面: %1").arg(path);
        }
        return QSharedPointer<MeshData>();
    }

    data->minBounds = minBounds;
    data->maxBounds = maxBounds;
    return data;
}

QSharedPointer<MeshData> ModelCache::placeholderModel()
{
    if (m_placeholder) {
        return m_placeholder;
    }

    auto data = QSharedPointer<MeshData>::create();
    const QVector3D p000(0.0f, 0.0f, 0.0f);
    const QVector3D p100(1.0f, 0.0f, 0.0f);
    const QVector3D p010(0.0f, 1.0f, 0.0f);
    const QVector3D p110(1.0f, 1.0f, 0.0f);
    const QVector3D p001(0.0f, 0.0f, 1.0f);
    const QVector3D p101(1.0f, 0.0f, 1.0f);
    const QVector3D p011(0.0f, 1.0f, 1.0f);
    const QVector3D p111(1.0f, 1.0f, 1.0f);

    data->vertices << p000 << p100 << p110;
    data->vertices << p000 << p110 << p010;
    data->vertices << p001 << p101 << p111;
    data->vertices << p001 << p111 << p011;
    data->vertices << p000 << p100 << p101;
    data->vertices << p000 << p101 << p001;
    data->vertices << p010 << p110 << p111;
    data->vertices << p010 << p111 << p011;
    data->vertices << p000 << p010 << p011;
    data->vertices << p000 << p011 << p001;
    data->vertices << p100 << p110 << p111;
    data->vertices << p100 << p111 << p101;

    data->minBounds = QVector3D(0.0f, 0.0f, 0.0f);
    data->maxBounds = QVector3D(1.0f, 1.0f, 1.0f);
    m_placeholder = data;
    return m_placeholder;
}
