#pragma once
#include <QObject>
#include <QProcess>
#include <QStringList>
#include "Models.h"

class MdadmBackend : public QObject
{
    Q_OBJECT
public:
    explicit MdadmBackend(QObject *parent = nullptr);

    void refresh();
    void applyQueue(const QStringList &commands);
    void removeStoppedRaid(const QString &dev);

    const QList<DiskInfo> &disks() const { return m_disks; }
    const QList<RaidInfo> &raids() const { return m_raids; }

signals:
    void refreshDone();
    void commandStarted(int index, const QString &cmd);
    void commandFinished(int index, bool ok, const QString &output);
    void allCommandsDone(bool anyError);
    void errorOccurred(const QString &msg);

private:
    void parseMdstat();
    void parseLsblk();
    void generateDemoData();
    void markDiskSegment(const QString &diskName, const QString &raidDev,
                         int level, bool failed);
    QString getArraySize(const QString &dev);
    int expectedDisks(int level, int actual);
    RaidStatus statusFromString(const QString &s);
    void runNextInQueue();

    QList<DiskInfo> m_disks;
    QList<RaidInfo> m_stoppedRaids; // массивы остановленные но не удалённые
    QList<RaidInfo> m_raids;

    QProcess   *m_proc       = nullptr;
    QStringList m_queue;
    int         m_queueIdx   = 0;
    bool        m_queueError = false;
};
