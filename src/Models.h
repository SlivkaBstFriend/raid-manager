#pragma once
#include <QString>
#include <QStringList>
#include <QList>

enum class SegmentType {
    Free, System, Raid0, Raid1, Raid5, Raid6, Raid10, Spare, Failed
};

struct DiskSegment {
    SegmentType type = SegmentType::Free;
    QString label;
    QString raidDev;   // e.g. "md0"
    double pct = 100.0;
};

struct DiskInfo {
    QString name;      // e.g. "sda"
    quint64 sizeGB = 0;
    QList<DiskSegment> segments;
};

enum class RaidStatus { Active, Degraded, Failed, Syncing, Stopped };

struct RaidInfo {
    int id = 0;
    QString dev;       // e.g. "md0"
    int level = 1;
    RaidStatus status = RaidStatus::Active;
    QString size;
    QStringList disks;
    QString syncProgress; // e.g. "75.3%" or "—"
    int spares = 0;
};
