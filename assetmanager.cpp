#include "assetmanager.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QtGlobal>

AssetManager *AssetManager::instance()
{
    static AssetManager instance;
    return &instance;
}

QString AssetManager::findAssetsRoot()
{
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 6; ++i) {
        if (dir.exists("assets")) {
            return dir.filePath("assets");
        }
        dir.cdUp();
    }

    QDir cwd(QDir::currentPath());
    if (cwd.exists("assets")) {
        return cwd.filePath("assets");
    }

    return QString();
}

QString AssetManager::defaultCatalogPath()
{
    const QString assetsRoot = findAssetsRoot();
    if (assetsRoot.isEmpty()) {
        return QString();
    }
    QDir rootDir(assetsRoot);
    return rootDir.filePath("models/furniture/catalog.json");
}

bool AssetManager::loadAssets(const QString &catalogPath, QString *errorMessage)
{
    m_assets.clear();
    m_categories.clear();

    if (catalogPath.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("catalog 路径为空");
        }
        return false;
    }

    QFileInfo catalogInfo(catalogPath);
    if (!catalogInfo.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("catalog 不存在: %1").arg(catalogPath);
        }
        return false;
    }

    QDir dir = catalogInfo.absoluteDir();
    bool foundAssetsRoot = false;
    for (int i = 0; i < 6; ++i) {
        if (dir.dirName().compare("assets", Qt::CaseInsensitive) == 0) {
            foundAssetsRoot = true;
            break;
        }
        if (!dir.cdUp()) {
            break;
        }
    }

    if (foundAssetsRoot) {
        m_assetsRoot = dir;
    } else {
        const QString fallback = findAssetsRoot();
        if (!fallback.isEmpty()) {
            m_assetsRoot = QDir(fallback);
        } else {
            m_assetsRoot = catalogInfo.absoluteDir();
        }
    }

    QFile file(catalogPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法读取 catalog: %1").arg(catalogPath);
        }
        return false;
    }

    QJsonParseError parseError{};
    const QByteArray rawData = file.readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(rawData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("catalog 解析失败: %1").arg(parseError.errorString());
        }
        return false;
    }
    if (doc.isNull()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("catalog 为空或只有空白");
        }
        return false;
    }

    if (!doc.isArray()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("catalog 格式错误，需为数组");
        }
        return false;
    }

    const QJsonArray assets = doc.array();
    for (const QJsonValue &val : assets) {
        if (!val.isObject()) {
            continue;
        }
        const QJsonObject obj = val.toObject();
        Asset asset;
        asset.id = obj.value("id").toString();
        asset.name = obj.value("name").toString();
        asset.category = obj.value("category").toString();
        asset.svgPath = resolvePath(obj.value("svg").toString());
        asset.modelPath = resolvePath(obj.value("model").toString());
        asset.material = obj.value("material").toString();
        if (asset.material.isEmpty()) {
            asset.material = QStringLiteral("wood");
        }

        const QJsonArray sizeArray = obj.value("defaultSize").toArray();
        if (sizeArray.size() >= 3) {
            asset.defaultSize = QVector3D(static_cast<float>(sizeArray.at(0).toDouble()),
                                          static_cast<float>(sizeArray.at(1).toDouble()),
                                          static_cast<float>(sizeArray.at(2).toDouble()));
        } else {
            asset.defaultSize = QVector3D(1000.0f, 1000.0f, 1000.0f);
        }

        asset.defaultScale = QVector3D(1.0f, 1.0f, 1.0f);
        const QJsonValue scaleValue = obj.value("scale");
        if (scaleValue.isArray()) {
            const QJsonArray scaleArray = scaleValue.toArray();
            if (scaleArray.size() >= 3) {
                asset.defaultScale = QVector3D(static_cast<float>(scaleArray.at(0).toDouble(1.0)),
                                               static_cast<float>(scaleArray.at(1).toDouble(1.0)),
                                               static_cast<float>(scaleArray.at(2).toDouble(1.0)));
            }
        } else if (scaleValue.isDouble()) {
            const float s = static_cast<float>(scaleValue.toDouble(1.0));
            asset.defaultScale = QVector3D(s, s, s);
        }

        if (asset.id.isEmpty()) {
            continue;
        }

        m_assets.insert(asset.id, asset);
        registerCategory(asset.category);
    }

    if (m_assets.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("catalog 未包含有效资源");
        }
        return false;
    }

    return true;
}

AssetManager::Asset AssetManager::getAsset(const QString &id) const
{
    return m_assets.value(id);
}

QList<AssetManager::Asset> AssetManager::getAssetsByCategory(const QString &category) const
{
    QList<Asset> result;
    for (const Asset &asset : m_assets) {
        if (asset.category == category) {
            result.append(asset);
        }
    }
    return result;
}

QStringList AssetManager::categories() const
{
    return m_categories;
}

QString AssetManager::assetsRoot() const
{
    return m_assetsRoot.absolutePath();
}

QString AssetManager::resolvePath(const QString &path) const
{
    if (path.isEmpty()) {
        return path;
    }
    if (path.startsWith(":/")) {
        return path;
    }
    if (QDir::isAbsolutePath(path)) {
        return path;
    }
    if (m_assetsRoot.absolutePath().isEmpty()) {
        return path;
    }
    return m_assetsRoot.filePath(path);
}

void AssetManager::registerCategory(const QString &category)
{
    if (category.isEmpty()) {
        return;
    }
    if (!m_categories.contains(category)) {
        m_categories.append(category);
    }
}
