#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>

class DesignScene;
class QJsonObject;

class ProjectManager : public QObject
{
    Q_OBJECT

public:
    static ProjectManager *instance();

    void setScene(DesignScene *scene);
    DesignScene *scene() const;

    QString currentPath() const;
    QString lastProjectPath() const;
    bool isDirty() const;

    bool saveCurrent(QString *errorMessage = nullptr);
    bool saveToPath(const QString &path, QString *errorMessage = nullptr);
    bool loadFromPath(const QString &path, QString *errorMessage = nullptr);
    bool loadAutosave(QString *errorMessage = nullptr);

    void clearCurrentProject();
    void setDirty(bool dirty);

    QString autosavePath() const;
    bool saveAutosave(QString *errorMessage = nullptr);
    bool hasAutosave() const;
    void removeAutosave();

    QStringList recentFiles() const;
    void addRecentFile(const QString &path);
    void removeRecentFile(const QString &path);

signals:
    void dirtyChanged(bool dirty);
    void currentPathChanged(const QString &path);
    void recentFilesChanged(const QStringList &files);

private:
    explicit ProjectManager(QObject *parent = nullptr);
    void setCurrentPath(const QString &path);
    void updateRecentFiles(const QStringList &files);
    void loadRecentFiles();
    void saveRecentFiles() const;
    void setLastProjectPath(const QString &path);
    QString normalizedPath(const QString &path) const;
    bool writeJson(const QString &path,
                   const QJsonObject &root,
                   QString *errorMessage) const;

    DesignScene *m_scene;
    QString m_currentPath;
    QString m_lastProjectPath;
    QStringList m_recentFiles;
    bool m_dirty;
    bool m_loading;
};

#endif // PROJECTMANAGER_H
