#include "MdadmBackend.h"
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>

MdadmBackend::MdadmBackend(QObject *parent) : QObject(parent) {}

void MdadmBackend::refresh()
{
    // Запоминаем текущие активные массивы перед обновлением
    QList<RaidInfo> previousRaids = m_raids;

    m_disks.clear();
    m_raids.clear();
    parseLsblk();
    parseMdstat();

    // Массивы которые были активны, но исчезли из mdstat — помечаем как Stopped
    for (const RaidInfo &prev : previousRaids) {
        bool stillExists = false;
        for (const RaidInfo &cur : m_raids)
            if (cur.dev == prev.dev) { stillExists = true; break; }

        if (!stillExists && prev.status != RaidStatus::Stopped) {
            RaidInfo stopped = prev;
            stopped.status = RaidStatus::Stopped;
            stopped.size = "—";
            stopped.syncProgress = "—";
            m_raids.append(stopped);
        }
    }

    // Также восстанавливаем ранее остановленные если они не появились снова
    for (const RaidInfo &stopped : m_stoppedRaids) {
        bool exists = false;
        for (const RaidInfo &cur : m_raids)
            if (cur.dev == stopped.dev) { exists = true; break; }
        if (!exists) m_raids.append(stopped);
    }

    // Обновляем список остановленных
    m_stoppedRaids.clear();
    for (const RaidInfo &r : m_raids)
        if (r.status == RaidStatus::Stopped)
            m_stoppedRaids.append(r);

    // Сортируем массивы по номеру устройства: md0, md1, md2, ...
    std::sort(m_raids.begin(), m_raids.end(), [](const RaidInfo &a, const RaidInfo &b) {
        // Извлекаем число из имени: "md0" -> 0, "md10" -> 10
        auto num = [](const QString &dev) {
            QString digits;
            for (QChar c : dev)
                if (c.isDigit()) digits += c;
            return digits.isEmpty() ? 9999 : digits.toInt();
        };
        return num(a.dev) < num(b.dev);
    });

    emit refreshDone();
}

void MdadmBackend::parseLsblk()
{
    QProcess p;
    p.start("lsblk", QStringList() << "-Pbo" << "NAME,SIZE,TYPE,FSTYPE,MOUNTPOINT");
    p.waitForFinished(5000);
    QString out = p.readAllStandardOutput();

    // Regex без raw string — экранируем вручную
    QRegularExpression lineRe(
        "NAME=\"([^\"]+)\"\\s+SIZE=\"([^\"]+)\"\\s+TYPE=\"([^\"]+)\""
        "\\s+FSTYPE=\"([^\"]*)\"\\s+MOUNTPOINT=\"([^\"]*)\"");

    QStringList lines = out.split('\n');

    for (const QString &line : lines) {
        QRegularExpressionMatch m = lineRe.match(line);
        if (!m.hasMatch()) continue;
        QString name  = m.captured(1);
        quint64 bytes = m.captured(2).toULongLong();
        QString type  = m.captured(3);
        if (type != "disk") continue;

        DiskInfo di;
        di.name   = name;
        di.sizeGB = bytes / (1024ULL * 1024 * 1024);
        DiskSegment seg;
        seg.type  = SegmentType::Free;
        seg.label = QString::fromUtf8("Свободно");
        seg.pct   = 100.0;
        di.segments.append(seg);
        m_disks.append(di);
    }

    for (const QString &line : lines) {
        QRegularExpressionMatch m = lineRe.match(line);
        if (!m.hasMatch()) continue;
        QString name  = m.captured(1);
        QString type  = m.captured(3);
        QString mount = m.captured(5);
        if (type != "part") continue;

        QString parent = name;
        while (!parent.isEmpty() && parent.at(parent.size()-1).isDigit())
            parent.chop(1);

        for (int i = 0; i < m_disks.size(); i++) {
            if (m_disks[i].name == parent && (mount == "/" || mount == "/boot")) {
                m_disks[i].segments.clear();
                DiskSegment sys;
                sys.type  = SegmentType::System;
                sys.label = mount + " (" + name + ")";
                sys.pct   = 30.0;
                DiskSegment fr;
                fr.type   = SegmentType::Free;
                fr.label  = QString::fromUtf8("Свободно");
                fr.pct    = 70.0;
                m_disks[i].segments.append(sys);
                m_disks[i].segments.append(fr);
                break;
            }
        }
    }
}

void MdadmBackend::parseMdstat()
{
    QFile f("/proc/mdstat");
    if (!f.open(QIODevice::ReadOnly)) {
        generateDemoData();
        return;
    }
    QTextStream ts(&f);
    QString content = ts.readAll();
    f.close();

    QRegularExpression arrayRe(
        "(md\\d+)\\s*:\\s*(active|inactive)\\s*(raid\\d+|linear)?\\s*([^\\n]+)",
        QRegularExpression::MultilineOption);
    QRegularExpression diskRe("(\\w+)\\[\\d+\\](\\(F\\))?");
    QRegularExpression syncRe("\\[(\\d+/\\d+)\\]\\s*=+>\\s*([0-9.]+)%");

    int id = 0;
    QRegularExpressionMatchIterator it = arrayRe.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        RaidInfo ri;
        ri.id  = id++;
        ri.dev = m.captured(1);

        QString activeStr = m.captured(2);
        QString levelStr  = m.captured(3);
        QString diskStr   = m.captured(4);

        ri.level = levelStr.startsWith("raid") ? levelStr.mid(4).toInt() : 1;

        QRegularExpressionMatchIterator di = diskRe.globalMatch(diskStr);
        while (di.hasNext()) {
            QRegularExpressionMatch dm = di.next();
            QString dname  = dm.captured(1);
            bool    failed = dm.captured(2) == "(F)";
            ri.disks.append("/dev/" + dname);
            markDiskSegment(dname, ri.dev, ri.level, failed);
        }

        ri.status = (activeStr == "inactive") ? RaidStatus::Failed : RaidStatus::Active;

        QRegularExpressionMatch sm = syncRe.match(content.mid(m.capturedStart()));
        if (sm.hasMatch()) {
            ri.status = RaidStatus::Syncing;
            ri.syncProgress = sm.captured(2) + "%";
        } else {
            ri.syncProgress = QString::fromUtf8("—");
        }

        ri.size = getArraySize(ri.dev);
        m_raids.append(ri);
    }

}

void MdadmBackend::markDiskSegment(const QString &diskName, const QString &raidDev,
                                    int level, bool failed)
{
    SegmentType st;
    switch (level) {
        case 0:  st = SegmentType::Raid0; break;
        case 1:  st = SegmentType::Raid1; break;
        case 5:  st = SegmentType::Raid5; break;
        case 6:  st = SegmentType::Raid6; break;
        default: st = SegmentType::Raid1; break;
    }
    if (failed) st = SegmentType::Failed;

    for (int i = 0; i < m_disks.size(); i++) {
        if (m_disks[i].name == diskName ||
            m_disks[i].name == diskName.left(diskName.size()-1)) {
            m_disks[i].segments.clear();
            DiskSegment seg;
            seg.type    = st;
            seg.label   = raidDev + (failed ? QString::fromUtf8(" (сбой)") : "");
            seg.raidDev = raidDev;
            seg.pct     = 100.0;
            m_disks[i].segments.append(seg);
            return;
        }
    }
}

QString MdadmBackend::getArraySize(const QString &dev)
{
    // Читаем размер из /proc/mdstat — не требует root
    QFile f("/proc/mdstat");
    if (!f.open(QIODevice::ReadOnly)) return "?";
    QString content = QTextStream(&f).readAll();
    f.close();

    // Ищем блок нашего устройства и строку с размером
    // Формат: "      1048512 blocks super 1.2 [2/2] [UU]"
    QRegularExpression blockRe(dev + "\\s*:.*?\\n(.*?)\\n",
        QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch bm = blockRe.match(content);
    if (bm.hasMatch()) {
        QRegularExpression sizeRe("(\\d+) blocks");
        QRegularExpressionMatch sm = sizeRe.match(bm.captured(1));
        if (sm.hasMatch()) {
            qint64 kb = sm.captured(1).toLongLong();
            if (kb > 1024*1024) return QString("%1 GiB").arg(kb/1024/1024);
            if (kb > 1024)      return QString("%1 MiB").arg(kb/1024);
            return QString("%1 KiB").arg(kb);
        }
    }
    return "?";
}

int MdadmBackend::expectedDisks(int level, int actual)
{
    Q_UNUSED(level);
    return actual;
}

void MdadmBackend::generateDemoData()
{
    struct D { const char *name; int gb; };
    static const D dd[] = {
        {"sda",500},{"sdb",500},{"sdc",1000},{"sdd",1000},
        {"sde",2000},{"sdf",2000},{"sdg",2000},{"sdh",2000},{"sdi",2000}
    };
    for (int i = 0; i < 9; i++) {
        DiskInfo di;
        di.name   = dd[i].name;
        di.sizeGB = dd[i].gb;
        DiskSegment s;
        s.type  = SegmentType::Free;
        s.label = QString::fromUtf8("Свободно");
        s.pct   = 100;
        di.segments.append(s);
        m_disks.append(di);
    }

    m_disks[0].segments.clear();
    { DiskSegment s; s.type=SegmentType::System;
      s.label="/ (sda1)"; s.pct=30; m_disks[0].segments.append(s); }
    { DiskSegment s; s.type=SegmentType::Raid0;
      s.label="md0"; s.raidDev="md0"; s.pct=70; m_disks[0].segments.append(s); }

    markDiskSegment("sdb","md0",0,false);
    markDiskSegment("sdc","md1",1,false);
    markDiskSegment("sdd","md1",1,false);
    markDiskSegment("sdf","md2",5,false);
    markDiskSegment("sdg","md2",5,false);
    markDiskSegment("sdh","md2",5,true);

    RaidInfo r0; r0.id=0; r0.dev="md0"; r0.level=0;
    r0.status=RaidStatus::Active; r0.size="350G";
    r0.disks<<"/dev/sda2"<<"/dev/sdb";
    r0.syncProgress=QString::fromUtf8("—");

    RaidInfo r1; r1.id=1; r1.dev="md1"; r1.level=1;
    r1.status=RaidStatus::Active; r1.size="1T";
    r1.disks<<"/dev/sdc"<<"/dev/sdd";
    r1.syncProgress=QString::fromUtf8("—");

    RaidInfo r2; r2.id=2; r2.dev="md2"; r2.level=5;
    r2.status=RaidStatus::Degraded; r2.size="3.6T";
    r2.disks<<"/dev/sdf"<<"/dev/sdg"<<"/dev/sdh";
    r2.syncProgress=QString::fromUtf8("—");

    m_raids << r0 << r1 << r2;
}

void MdadmBackend::removeStoppedRaid(const QString &dev)
{
    m_stoppedRaids.erase(
        std::remove_if(m_stoppedRaids.begin(), m_stoppedRaids.end(),
            [&dev](const RaidInfo &r){ return r.dev == dev; }),
        m_stoppedRaids.end());
    // Также убираем из основного списка если там ещё есть
    m_raids.erase(
        std::remove_if(m_raids.begin(), m_raids.end(),
            [&dev](const RaidInfo &r){ return r.dev == dev; }),
        m_raids.end());
    emit refreshDone();
}

void MdadmBackend::applyQueue(const QStringList &commands)
{
    if (commands.isEmpty()) return;
    m_queue      = commands;
    m_queueIdx   = 0;
    m_queueError = false;
    runNextInQueue();
}

void MdadmBackend::runNextInQueue()
{
    if (m_queueIdx >= m_queue.size()) {
        emit allCommandsDone(m_queueError);
        refresh();
        return;
    }

    const QString cmd = m_queue.at(m_queueIdx);
    emit commandStarted(m_queueIdx, cmd);

    if (m_proc) { m_proc->deleteLater(); m_proc = nullptr; }
    m_proc = new QProcess(this);

    // Если команда содержит shell-операторы (|, >, >>, &&) — запускаем через sudo sh -c
    // иначе напрямую через pkexec
    QStringList pkArgs;
    bool needsShell = cmd.contains('|') || cmd.contains('>') || cmd.contains("&&");
    if (needsShell) {
        pkArgs << "/bin/sh" << "-c" << cmd;
    } else {
        QStringList parts = cmd.split(' ', Qt::SkipEmptyParts);
        QString program = parts.takeFirst();
        pkArgs << program << parts;
    }

    connect(m_proc,
            static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus) {
        QString out = m_proc->readAllStandardOutput()
                    + m_proc->readAllStandardError();
        bool ok = (code == 0);
        if (!ok) m_queueError = true;
        emit commandFinished(m_queueIdx, ok, out.trimmed());
        m_queueIdx++;
        runNextInQueue();
    });

    m_proc->start("pkexec", pkArgs);
}

RaidStatus MdadmBackend::statusFromString(const QString &s)
{
    if (s == "active")   return RaidStatus::Active;
    if (s == "degraded") return RaidStatus::Degraded;
    if (s == "failed")   return RaidStatus::Failed;
    return RaidStatus::Active;
}
