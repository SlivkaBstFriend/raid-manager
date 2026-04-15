#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QAction>
#include <QStatusBar>
#include <QProgressDialog>

#include "MdadmBackend.h"
#include "DiskVisualWidget.h"
#include "RaidTableWidget.h"
#include "QueuePanel.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onRefreshDone();
    void onRaidSelected(const QString &dev);
    void onSegmentClicked(const QString &disk, const QString &raidDev);

    void actionCreateRaid();
    void actionStopArray();
    void actionStartArray();
    void actionDeleteArray();
    void actionAddSpare();
    void actionRemoveDisk();
    void actionDetail();
    void actionManualCommand();

    void onApplyRequested();
    void onCommandStarted(int idx, const QString &cmd);
    void onCommandFinished(int idx, bool ok, const QString &out);
    void onAllDone(bool anyError);

private:
    void setupMenuBar();
    void setupToolBar();
    void setupUi();
    void updateActions();
    void enqueue(const QStringList &cmds);
    RaidInfo *selectedRaid();
    QString raidDevById();

    MdadmBackend    *m_backend;
    DiskVisualWidget *m_diskMap;
    RaidTableWidget  *m_raidTable;
    QueuePanel       *m_queue;

    QAction *m_actStop;
    QAction *m_actStart;
    QAction *m_actDelete;
    QAction *m_actAddSpare;
    QAction *m_actRemoveDisk;
    QAction *m_actDetail;

    int m_selectedRaidId = -1;
    QString m_selectedRaidDev;
    QString m_devToDelete; // надёжнее чем id
    QProgressDialog *m_progress = nullptr;
};
