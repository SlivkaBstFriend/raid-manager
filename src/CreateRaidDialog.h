#pragma once
#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QListWidget>
#include <QLabel>
#include <QPlainTextEdit>
#include "Models.h"

class CreateRaidDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CreateRaidDialog(const QList<DiskInfo> &disks,
                              const QList<RaidInfo> &raids,
                              QWidget *parent = nullptr);

    // Returns list of mdadm commands to enqueue
    QStringList buildCommands() const;

private:
    QComboBox    *m_devCombo;
    QComboBox    *m_levelCombo;
    QListWidget  *m_diskList;
    QSpinBox     *m_spareSpin;
    QComboBox    *m_chunkCombo;
    QLabel       *m_minDisksLabel;
    QPlainTextEdit *m_preview;

    QList<DiskInfo> m_disks;
    QList<RaidInfo> m_raids;

    QString nextFreeMd() const;
    int minDisks(int level) const;
    void updatePreview();
    QStringList selectedDisks() const;
};
