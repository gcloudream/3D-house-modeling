#include "projectmanager.h"

#include "designscene.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>

namespace {
const char kSettingsOrg[] = "QtPlanArchitect";
const char kSettingsApp[] = "QtPlanArchitect";
const char kRecentFilesKey[] = "recentFiles";
const char kLastProjectKey[] = "lastProjectPath";
} // namespace

ProjectManager *ProjectManager::instance()
{
    static ProjectManager instance;
    return &instance;
}

ProjectManager::ProjectManager(QObject *parent)
    : QObject(parent)
    , m_scene(nullptr)
    , m_currentPath()
    , m_lastProjectPath()
    , m_recentFiles()
    , m_dirty(false)
    , m_loading(false)
{
    loadRecentFiles();
}

void ProjectManager::setScene(DesignScene *scene)
{
    if (m_scene == scene) {
        return;
    }

    if (m_scene) {
        disconnect(m_scene, nullptr, this, nullptr);
    }

    m_scene = scene;

    if (m_scene) {
        connect(m_scene, &DesignScene::sceneContentChanged, this, [this]() {
            if (m_loading) {
                return;
            }
            setDirty(true);
        });
    }
}

DesignScene *ProjectManager::scene() const
{
    return m_scene;
}

QString ProjectManager::currentPath() const
{
    return m_currentPath;
}

QString ProjectManager::lastProjectPath() const
{
    return m_lastProjectPath;
}

bool ProjectManager::isDirty() const
{
    return m_dirty;
}

bool ProjectManager::saveCurrent(QString *errorMessage)
{
    if (m_currentPath.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("当前项目没有保存路径");
        }
        return false;
    }
    return saveToPath(m_currentPath, errorMessage);
}

bool ProjectManager::saveToPath(const QString &path, QString *errorMessage)
{
    if (!m_scene) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("场景未初始化");
        }
        return false;
    }

    QString targetPath = normalizedPath(path);
    if (!targetPath.endsWith(".qplan", Qt::CaseInsensitive)) {
        targetPath += ".qplan";
    }

    const QJsonObject root = m_scene->toJson();
    if (!writeJson(targetPath, root, errorMessage)) {
        return false;
    }

    setCurrentPath(targetPath);
    setDirty(false);
    addRecentFile(targetPath);
    removeAutosave();
    return true;
}

bool ProjectManager::loadFromPath(const QString &path, QString *errorMessage)
{
    if (!m_scene) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("场景未初始化");
        }
        return false;
    }

    QFileInfo info(path);
    if (!info.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("文件不存在: %1").arg(path);
        }
        return false;
    }

    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法读取文件: %1").arg(info.absoluteFilePath());
        }
        return false;
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("JSON 解析失败: %1").arg(parseError.errorString());
        }
        return false;
    }
    if (!doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("JSON 格式错误: 根节点需为对象");
        }
        return false;
    }

    m_loading = true;
    m_scene->fromJson(doc.object());
    m_loading = false;

    setCurrentPath(info.absoluteFilePath());
    setDirty(false);
    addRecentFile(info.absoluteFilePath());
    setLastProjectPath(info.absoluteFilePath());
    return true;
}

bool ProjectManager::loadAutosave(QString *errorMessage)
{
    const QString path = autosavePath();
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("没有可恢复的自动保存文件");
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法读取自动保存文件: %1").arg(path);
        }
        return false;
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("自动保存解析失败: %1").arg(parseError.errorString());
        }
        return false;
    }
    if (!doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("自动保存格式错误: 根节点需为对象");
        }
        return false;
    }

    m_loading = true;
    m_scene->fromJson(doc.object());
    m_loading = false;

    if (!m_lastProjectPath.isEmpty()) {
        setCurrentPath(m_lastProjectPath);
    } else {
        setCurrentPath(QString());
    }
    setDirty(true);
    return true;
}

void ProjectManager::clearCurrentProject()
{
    setCurrentPath(QString());
    setDirty(false);
    removeAutosave();
}

void ProjectManager::setDirty(bool dirty)
{
    if (m_dirty == dirty) {
        return;
    }
    m_dirty = dirty;
    emit dirtyChanged(m_dirty);
}

QString ProjectManager::autosavePath() const
{
    QString basePath = m_currentPath;
    if (basePath.isEmpty()) {
        basePath = m_lastProjectPath;
    }

    if (!basePath.isEmpty()) {
        QFileInfo info(basePath);
        return info.dir().filePath(info.completeBaseName() + ".autosave.qplan");
    }

    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dirPath.isEmpty()) {
        dirPath = QDir::tempPath();
    }
    QDir dir(dirPath);
    return dir.filePath("untitled.autosave.qplan");
}

bool ProjectManager::saveAutosave(QString *errorMessage)
{
    if (!m_scene || !m_dirty) {
        return false;
    }

    const QString path = autosavePath();
    if (path.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法确定自动保存路径");
        }
        return false;
    }

    QDir dir(QFileInfo(path).absoluteDir());
    if (!dir.exists() && !dir.mkpath(".")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法创建自动保存目录");
        }
        return false;
    }

    const QJsonObject root = m_scene->toJson();
    return writeJson(path, root, errorMessage);
}

bool ProjectManager::hasAutosave() const
{
    const QString path = autosavePath();
    return !path.isEmpty() && QFileInfo::exists(path);
}

void ProjectManager::removeAutosave()
{
    const QString path = autosavePath();
    if (!path.isEmpty()) {
        QFile::remove(path);
    }
}

QStringList ProjectManager::recentFiles() const
{
    return m_recentFiles;
}

void ProjectManager::addRecentFile(const QString &path)
{
    const QString normalized = normalizedPath(path);
    if (normalized.isEmpty()) {
        return;
    }

    QStringList updated = m_recentFiles;
    updated.removeAll(normalized);
    updated.prepend(normalized);
    while (updated.size() > 10) {
        updated.removeLast();
    }
    updateRecentFiles(updated);
}

void ProjectManager::removeRecentFile(const QString &path)
{
    const QString normalized = normalizedPath(path);
    if (normalized.isEmpty()) {
        return;
    }

    QStringList updated = m_recentFiles;
    updated.removeAll(normalized);
    updateRecentFiles(updated);
}

void ProjectManager::setCurrentPath(const QString &path)
{
    const QString normalized = normalizedPath(path);
    if (m_currentPath == normalized) {
        return;
    }
    m_currentPath = normalized;
    if (!m_currentPath.isEmpty()) {
        setLastProjectPath(m_currentPath);
    }
    emit currentPathChanged(m_currentPath);
}

void ProjectManager::updateRecentFiles(const QStringList &files)
{
    m_recentFiles = files;
    saveRecentFiles();
    emit recentFilesChanged(m_recentFiles);
}

void ProjectManager::loadRecentFiles()
{
    QSettings settings(kSettingsOrg, kSettingsApp);
    m_recentFiles = settings.value(kRecentFilesKey).toStringList();
    m_lastProjectPath = settings.value(kLastProjectKey).toString();
    emit recentFilesChanged(m_recentFiles);
}

void ProjectManager::saveRecentFiles() const
{
    QSettings settings(kSettingsOrg, kSettingsApp);
    settings.setValue(kRecentFilesKey, m_recentFiles);
}

void ProjectManager::setLastProjectPath(const QString &path)
{
    m_lastProjectPath = normalizedPath(path);
    if (!m_lastProjectPath.isEmpty()) {
        QSettings settings(kSettingsOrg, kSettingsApp);
        settings.setValue(kLastProjectKey, m_lastProjectPath);
    }
}

QString ProjectManager::normalizedPath(const QString &path) const
{
    if (path.isEmpty()) {
        return QString();
    }
    QFileInfo info(path);
    return info.absoluteFilePath();
}

bool ProjectManager::writeJson(const QString &path,
                               const QJsonObject &root,
                               QString *errorMessage) const
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法写入文件: %1").arg(path);
        }
        return false;
    }

    const QJsonDocument doc(root);
    if (file.write(doc.toJson(QJsonDocument::Indented)) == -1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("写入失败: %1").arg(path);
        }
        return false;
    }

    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("保存失败: %1").arg(path);
        }
        return false;
    }

    return true;
}
