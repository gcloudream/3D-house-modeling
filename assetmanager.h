#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <QHash>
#include <QDir>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector3D>

class AssetManager
{
public:
    struct Asset {
        QString id;
        QString name;
        QString category;
        QString svgPath;
        QString modelPath;
        QVector3D defaultSize;
        QVector3D defaultScale;
        QString material;
    };

    static AssetManager *instance();
    static QString findAssetsRoot();
    static QString defaultCatalogPath();

    bool loadAssets(const QString &catalogPath, QString *errorMessage = nullptr);
    Asset getAsset(const QString &id) const;
    QList<Asset> getAssetsByCategory(const QString &category) const;
    QStringList categories() const;
    QString assetsRoot() const;

private:
    AssetManager() = default;
    QString resolvePath(const QString &path) const;
    void registerCategory(const QString &category);

    QHash<QString, Asset> m_assets;
    QStringList m_categories;
    QDir m_assetsRoot;
};

#endif // ASSETMANAGER_H
